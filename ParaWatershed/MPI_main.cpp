#include "tool.h"
#include "ParaWatershed/ParaWatershed.h"
#include <Grid/grid.h>
#include <Grid/io_gdal.h>
#include <FastWatershed/WatershedFlowPathTraversal.h>
#include <Grid/tool.h>
#include <mpi.h>
#include "MPI_ParaWatershed.h"
#include <Grid/util.h>
using namespace std::filesystem;
using namespace std;
int mpi_main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cout << "Too few arguments for MPI program." << std::endl;
        std::cout << "This program delineates watersheds using our proposed parallel MPI algorithm" << std::endl;
        std::cout << "mpiexec -np <PROCESSES_NUMBER> ParaWatershed MPI <INPUT Tiled Flow Dir Folder> <OUTPUT Tiled Watershed Folder>" << std::endl;
        std::cout << "This command determines flow directions in the given DEM " << std::endl;
        std::cout << "Example usage: mpiexec -np 3 ParaWatershed MPI ./tiledflowdir ./tiledwatershed" << std::endl;
        return -1;
    }

    path dirTileFolder = argv[2];
    path wsTileFolder = argv[3];

    MPI_Init(&argc, &argv);
    try
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        g_rank = rank;

        int size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        if (size < 2) {
            std::cout << "Specify at least 2 processes, e.g. 'mpiexec -np 2 ParaWatershed MPI path2DirTileFolder path2WatershedTileFolder'" << std::endl;
            return -1;
        }

        if (rank == 0) {

            std::cout << "Dir Folder" << dirTileFolder << std::endl;
            std::cout << "wsTileFolder" << wsTileFolder << std::endl;

            if (!exists(dirTileFolder)) {
                std::cout << "Input tiled flow dir folder does not exist:" << dirTileFolder << std::endl;
                MPI_Abort(MPI_COMM_WORLD, -1);
                return 0;
            }

            if (!exists(dirTileFolder / "outlets.txt")) {
                std::cout << "\nMissing outlet file:" << dirTileFolder / "outlets.txt" << std::endl;
                MPI_Abort(MPI_COMM_WORLD, -1);
                return 0;
            }
            
            std::cout << "Input flow dir :" << dirTileFolder << std::endl;

            //clear output folder 
            if (exists(wsTileFolder))
            {
                std::cout << "wsTileFolder exists." << wsTileFolder << std::endl;
                remove_all(wsTileFolder);
            }

            create_directories(wsTileFolder);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        GridInfo gridInfo;
        gridInfo.read(dirTileFolder / "gridInfo.txt");
        
        std::vector<path> allTileFiles = readAllTileFiles(dirTileFolder / "tiles.txt");

        if (rank == 0) {
            std::cout << "main process begins" << endl;
            mainProcess(size, dirTileFolder, allTileFiles.size(), gridInfo);
        }
        else
        {
            std::map<Cell, int> globalOutlets = loadOutletLocations(dirTileFolder / "outlets.txt");
            if (globalOutlets.size() == 0) {
                std::cout << "No outlets are provided in the file:" << dirTileFolder / "outlets.txt" << std::endl;
            }
            else
            {
                std::cout << "total number of outlets:" << globalOutlets.size() << std::endl;
                computeProcess(rank, size, dirTileFolder, wsTileFolder, allTileFiles, gridInfo, globalOutlets);
            }
        }
    }
    catch (exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    MPI_Finalize();
    return 0;
}
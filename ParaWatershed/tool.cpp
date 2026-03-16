#include <gdal_priv.h>
#include <string>
#include <stdexcept>
#include <cmath>
#include <fstream>
#include <iostream>
#include "grid_info.h"
#include <Grid/flowDir.h>
#include <Grid/grid.h>
#include <Grid/io_gdal.h>
#include "tool.h"




void ProcessMemUsage(long& vmpeak, long& vmhwm) {
#if defined( __linux__ ) || defined( __linux ) || defined( linux ) || defined( __gnu_linux__ )
    vmpeak = 0;
    vmhwm = 0;

    std::ifstream fin("/proc/self/status");
    if (!fin.good())
        return;

    while (!vmpeak || !vmhwm) {
        std::string line;

        if (!getline(fin, line)) {  // Check if we could still read file
            return;
        }

        if (line.compare(0, 7, "VmPeak:") == 0) {  // Peak virtual memory size
            vmpeak = std::stoi(line.substr(7));
            // } else if(line.compare(0,7,"VmSize:")==0){ //Virtual memory size
            //   std::cerr<<"T: "<<line.substr(7,10)<<std::endl;
            //   vmsize = std::stoi(line.substr(7,10));
            // } else if(line.compare(0,6,"VmRSS:")==0){  //Resident set size
            //   std::cerr<<"T: "<<line.substr(7,10)<<std::endl;
            //   vmrss = std::stoi(line.substr(7,10));
        }
        else if (line.compare(0, 6, "VmHWM:") == 0) {  // Peak resident set size
            vmhwm = std::stoi(line.substr(6));
        }
    }
#else
#pragma message( "Cannot check memory statistics for this OS." )
#endif
}

std::vector<std::filesystem::path> readAllTileFiles(const std::filesystem::path& filePath)
{
    std::vector<std::filesystem::path> lines;    
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to read file:" + filePath.string());
    }

    for (std::string line; std::getline(file, line); ) {
        lines.push_back(filePath.parent_path() / line);
    }

    file.close();

    return lines;
}

void writeAllTiles(std::vector<std::string>& lines, const std::filesystem::path& filePath)
{
    std::ofstream fout;
    fout.open(filePath);
    if (fout.fail()) {
        throw std::runtime_error("Failed to open file:" + filePath.string());
        return;
    }

    for (auto& line : lines) {
        fout << line << "\n";
    }
    fout.close();
}

template<typename T>
void writeTile(GDALRasterBand* band, GDALDataType type, std::array< double, 6 >& geotransform, double no_data,
    int tileHeight, int tileWidth, int tileRow, int tileCol, int height, int width,
    std::filesystem::path& outputPath)
{
    Grid<T> tile(height, width);
    tile.setNoDataValue(no_data);
    tile.allocate();

    auto returnValue = band->RasterIO(GF_Read, tileWidth * tileCol, tileHeight * tileRow,
        tile.width(), tile.height(), (void*)tile.getDataPtr(), tile.width(), tile.height(), type, 0, 0);
    if (returnValue != CE_None) {
        throw std::runtime_error("An error occured while trying to read tile.");
    }
    std::array< double, 6 > tileGeotransform(geotransform);
    tileGeotransform[0] = geotransform[0] + tileWidth * tileCol * geotransform[1] + tileHeight * tileRow * geotransform[2];
    tileGeotransform[3] = geotransform[3] + tileHeight * tileRow * geotransform[5] + tileWidth * tileCol * geotransform[4];
    tile.GeoTransform() = tileGeotransform;
    writeRaster(tile, outputPath);
}

//read the input geotiff file and create tiles from them directly
void generateTiles(const std::filesystem::path& filePath, int tileHeight, int tileWidth, const std::filesystem::path& outputFolder) {
    std::filesystem::path inputFilePath = filePath;
    std::filesystem::path output = outputFolder;

    if (exists(output))
        remove_all(output);
    create_directories(output);


    GDALAllRegister();
    GDALDataset* fin = (GDALDataset*)GDALOpen(inputFilePath.string().c_str(), GA_ReadOnly);
    if (fin == nullptr)
        throw std::runtime_error("Could not open file '" + inputFilePath.string() + "' to get dimensions.");

    GridInfo gridInfo;
    GDALRasterBand* band = fin->GetRasterBand(1);
    GDALDataType type = band->GetRasterDataType();
    int grandHeight = band->GetYSize();
    int grandWidth = band->GetXSize();
    std::array< double, 6 > geotransform;
    if (fin->GetGeoTransform(geotransform.data()) != CE_None) {
        geotransform = { 0,1,0,0,0,-1 };
    }

    int height, width;

    int gridHeight = std::ceil((double)grandHeight / tileHeight);
    int gridWidth = std::ceil((double)grandWidth / tileWidth);

    gridInfo.gridHeight = gridHeight;
    gridInfo.gridWidth = gridWidth;
    gridInfo.tileHeight = tileHeight;
    gridInfo.tileWidth = tileWidth;
    gridInfo.grandHeight = grandHeight;
    gridInfo.grandWidth = grandWidth;
    gridInfo.no_data_value = band->GetNoDataValue();

    vector<string> fileNames;

    for (int tileRow = 0; tileRow < gridHeight; tileRow++) {
        for (int tileCol = 0; tileCol < gridWidth; tileCol++) {
            auto outputFileName = Cell(tileRow, tileCol).to_string() + ".tif";
            fileNames.push_back(outputFileName);
            auto path = output / outputFileName;
            //rows and cols to be read 
            height = (grandHeight - tileHeight * tileRow >= tileHeight) ? tileHeight : (grandHeight - tileHeight * tileRow);
            width = (grandWidth - tileWidth * tileCol >= tileWidth) ? tileWidth : (grandWidth - tileWidth * tileCol);
            if (height <= 1 || width <= 1) {
                throw std::runtime_error("Both the height and widht of the generated tiles must be greater than 1.");
            }

            if (type == GDALDataType::GDT_Byte) {
                writeTile<FlowDir>(band, type, geotransform, band->GetNoDataValue(), tileHeight, tileWidth, tileRow, tileCol, height, width, path);
            }
            else if (type == GDALDataType::GDT_Int32)
            {
                writeTile<std::int32_t>(band, type, geotransform, band->GetNoDataValue(), tileHeight, tileWidth, tileRow, tileCol, height, width, path);
            }
            else
            {
                throw std::runtime_error("Currently, only Flow Direction grid and watershed grid (int32) are supported for splitting");
            }

        }
    }
    GDALClose((GDALDatasetH)fin);

    writeAllTiles(fileNames, outputFolder / "tiles.txt");
    gridInfo.write(outputFolder / "gridInfo.txt");
}


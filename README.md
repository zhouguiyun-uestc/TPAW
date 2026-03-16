# TPAW
**Manuscript Title:**
TPAW: a tile-based parallel watershed delineation algorithm for massive digital elevation models across distributed and shared-memory architectures

**Authors:**
Guiyun Zhou, Tao Zhou, Maoqiang Jing, Suhua Fu, Jiayun Lin, Jie Wen, Yi Yuan

**Corresponding author:**
Guiyun Zhou (zhouguiyun@uestc.edu.cn)

This directory contains the source code for the TPAW algorithm proposed in the manuscript above. The algorithm is designed for efficient tile-based parallel watershed delineation, with a focus on performance in distributed-memory architectures and adaptability to shared-memory environments. All experiments and evaluations presented in the manuscript were conducted using this code.

The program can be compiled and executed on both Windows and Linux platforms.

## Dependencies
* **MPI**: Required for distributed-memory parallel computing across multiple nodes.
* **OpenMP**: Required for shared-memory parallel computing.
* **GDAL**: Used for reading and processing raster Digital Elevation Models.
* **cereal**: A C++11 header-only library used for efficient data serialization. 
* **zstd**: A fast lossless compression algorithm used to compress serialized data.

## Usage:
The program provides a set of different methods. The general command format is:
```bash
ParaWatershed [Method] [other_parameters]
```
### The supported methods include:

**"split"**: Divide a large flow direction grid into smaller tiles.
```bash
ParaWatershed split flowdir.tif 100 100 pathToOutputFolder
```
**"merge"**: Combine watershed tiles to a single raster file.
```bash
ParaWatershed merge pathToInputFlowDirFolder pathToInputWatershedFolder output.tif
```
**"outlets"**: Automatically compute the outlets.The output outlet file contains 1-based row, col and label of each outlet.
```bash
ParaWatershed outlets input_flowdir.tif output_outlets.txt 255
```
**"compare"**: Compare two grids to check for identity.
```bash
ParaWatershed compare  grid1.tif grid2.tif
```
**"diff"**: Calculate the pixel-wise difference between two grids.
```bash
ParaWatershed diff grid1.tif grid2.tif diff.tif
```
**"OpenMP"**: Shared-memory parallel computing using multiple CPU cores.
```bash
ParaWatershed OpenMP 8 pathToInputFlowDirectionFolder pathToOutputWatershedFolder
```
**"MPI"**: Distributed-memory parallel computing for clusters.
```bash
mpiexec -np 3 ParaWatershed MPI pathToInputFlowDirectionFolder pathToOutputWatershedFolder
```
### Important Notice:

By default, the processed grids should contain cells no more than the maximum value hold in an unsigned int32 (which is 4294967295). 
If larger grids need to processed, please define the macro _MASSIVE_DATASET_ in grid.h to enable correct indexing of the cells. 

### Test Data:

The following datasets can be used to evaluate the performance and scalability of the algorithms:

* **DEMs of five counties in Minnesota, USA:**
https://resources.gisdata.mn.gov/pub/data/elevation/lidar/county/  
*(Note:Please convert the data into GeoTIFF format using ArcGIS or another GIS tool before running the program)*

* **Texas and CONUS:**
https://data.isnew.info/meshed.html


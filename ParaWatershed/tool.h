#pragma once
#include <filesystem>
#include "grid_info.h"
#include <Grid/grid.h>
#include <Grid/io_gdal.h>
void generateTiles(const std::filesystem::path& filePath, int tileHeight, int tileWidth, const std::filesystem::path& outputFolder);

std::vector<std::filesystem::path> readAllTileFiles(const std::filesystem::path& filePath);
void writeAllTiles(std::vector<std::string>& lines, const std::filesystem::path& filePath);

void ProcessMemUsage(long& vmpeak, long& vmhwm);

template<typename T>
void mergeTiles(GridInfo& gridInfo, const std::filesystem::path& inputFolder, const std::filesystem::path& outputFilePath) {
    int grandHeight = gridInfo.grandHeight;
    int grandWidth = gridInfo.grandWidth;
    int gridHeight = gridInfo.gridHeight;
    int gridWidth = gridInfo.gridWidth;
    int tileHeight = gridInfo.tileHeight;
    int tileWidth = gridInfo.tileWidth;
    Grid<T> tiles(grandHeight, grandWidth);
    tiles.allocate();

    for (int tileRow = 0; tileRow < gridHeight; tileRow++) {
        for (int tileCol = 0; tileCol < gridWidth; tileCol++) {
            auto filePath = inputFolder / (Cell(tileRow, tileCol).to_string() + ".tif");
            if (!exists(filePath)) continue;
            Grid<T> tile = readRaster<T>(filePath);

            if (tileRow == 0 && tileCol == 0) {
                tiles.GeoTransform() = tile.GeoTransform();
            }
            tiles.setNoDataValue(tile.noDataValue());
            int height = tile.height();
            int width = tile.width();
            int startRow = tileHeight * tileRow;
            int startCol = tileWidth * tileCol;
            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    tiles(startRow + row, startCol + col) = tile(row, col);
                }
            }
        }
    }
    writeRaster(tiles, outputFilePath);
}


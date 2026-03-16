#pragma once

#include <string>
#include <filesystem>

class GridInfo {
public:
    int tileHeight, tileWidth;                                  // number of rows and cols in a tile
    int gridHeight, gridWidth;                                  // number of tiles along rows and cols
    int grandHeight, grandWidth;                                // total rows and cols in the grid
    int no_data_value = 0;
public:
    void write(const std::filesystem::path& outputPath);
    void read(const std::filesystem::path& inputPath);
};


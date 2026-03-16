#include "grid_info.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <Grid/cell.h>
void GridInfo::write(const std::filesystem::path& outputPath)
{
    std::ofstream fout;
    fout.open(outputPath);
    if (fout.fail())
        throw std::runtime_error("Failed to write file:"+outputPath.string());

    fout << gridHeight << "," << gridWidth << "\n";
    fout << tileHeight << "," << tileWidth << "\n";
    fout << grandHeight << "," << grandWidth << "\n";
    fout << no_data_value<<"\n";
    fout.close();
}

void GridInfo::read(const std::filesystem::path& inputPath)
{
    std::ifstream fin;
    fin.open(inputPath);
    if (!fin.is_open())
        throw std::runtime_error("Failed to read file:" + inputPath.string());

   
    std::string line;
    getline(fin, line);
    std::istringstream iss(line);
    char comma;
    iss >> gridHeight >> comma >> gridWidth;
    getline(fin, line);
    iss = std::istringstream(line);
    iss >> tileHeight >> comma >> tileWidth;

    getline(fin, line);
    iss = std::istringstream(line);
    iss >> grandHeight >> comma >> grandWidth;

    getline(fin, line);
    iss = std::istringstream(line);
    iss >> no_data_value;

    fin.close();
}


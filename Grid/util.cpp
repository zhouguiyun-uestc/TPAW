#include "util.h"
#include "cell.h"
#include <fstream>
#include <iostream>

using namespace std;
void writeOutletFile(
	std::map<int, int>& keyCellCount, //label-count map
	std::map<int, Cell> key2Cell, //label-outlet map
	const std::filesystem::path& outletFilePath)
{
	//Generate outlet file
	std::fstream file(outletFilePath, std::ios::out);
	int row, col, label;
	if (keyCellCount.size() > 0) {
		//write label by watershed size starting from 1
		int newLabel = 1;
		for (auto [key, count] : keyCellCount) {
			Cell c = key2Cell.at(key);
			file << c.row + 1 << "\t" << c.col + 1 << "\t" << newLabel << endl;
			newLabel++;
		}
	}
	else {
		for (auto [label, c] : key2Cell) {
			file << c.row + 1 << "\t" << c.col + 1 << "\t" << label << endl;
		}
	}
	file.close();
}

void writeOutletFile(
	const std::map<Cell, int>& outlets,
	const std::filesystem::path& outletFilePath)
{
	//Generate outlet file
	std::fstream file(outletFilePath, std::ios::out);
	int row, col, label;
	for (auto [c, label] : outlets) {
		file << c.row + 1 << "\t" << c.col + 1 << "\t" << label << endl;
	}
	file.close();
}

std::map<Cell, int> loadOutletLocations(const std::filesystem::path& outletFilePath)
{
	std::map<Cell, int> cell2Label;

	std::fstream file(outletFilePath, std::ios::in);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to read outlet file:" + outletFilePath.string());
	}

	int row, col, label;
	//int times = 0;
	while (file >> row >> col >> label)
	{
		//times++;
		//cout << times << endl;
		cell2Label[Cell(row - 1, col - 1)] = label;
	}

	file.close();
	return cell2Label;
}




//void remove_files_with_extension(const std::string& directory, const std::string& extension) {
//	namespace fs = std::filesystem;	
//	try {
//		// Iterate through all files in the specified directory
//		for (const auto& entry : fs::directory_iterator(directory)) {
//			const auto& path = entry.path();
//
//			// Check if it's a file and has the specified extension
//			if (fs::is_regular_file(path) && path.extension() == extension) {
//				// Remove the file
//				fs::remove(path);
//				std::cout << "file removed:" << path << endl;
//			}
//		}
//	}
//	catch (const std::exception& e) {
//		std::cerr << "Error: " << e.what() << std::endl;
//	}
//}


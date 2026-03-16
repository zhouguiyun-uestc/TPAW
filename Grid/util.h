#pragma once

#include <chrono>
#include <map>
#include <filesystem>
#include "cell.h"
struct TimeSpan
{
	std::chrono::system_clock::time_point  start;
	size_t total = 0;
	TimeSpan()
	{
		start = std::chrono::system_clock::now();
	}

	void reset()
	{
		start = std::chrono::system_clock::now();
	}

	int elapsed()
	{
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start);
		total+=elapsed.count();
		return elapsed.count();
	}

	int elapsed_millseconds()
	{
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
		total += elapsed.count();
		return elapsed.count();
	}
};
void writeOutletFile(
	const std::map<Cell, int>& outlets,
	const std::filesystem::path& outletFilePath);

void writeOutletFile(std::map<int, int>& keyCellCount, std::map<int, Cell> key2Cell, const std::filesystem::path& outletFilePath);
std::map<Cell, int> loadOutletLocations(const std::filesystem::path& outletFilePath);
//void remove_files_with_extension(const std::string& directory, const std::string& extension);

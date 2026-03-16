#include "string_util.h"
#include <fstream>
#include <iostream>
std::vector<std::string> split(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string token;
	while (std::getline(iss, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

std::vector<std::string> readAlllines(const std::filesystem::path& path) {
	std::ifstream file(path);

	if (!file.is_open())
		throw std::runtime_error("cannot open file:" + path.string());

	std::vector<std::string> lines;
	std::string line;

	while (std::getline(file, line)) {
		line = trim(line);
		if (line.length() > 0)
			lines.push_back(line);
	}

	file.close();

	for (const auto& l : lines) {
		std::cout << l << std::endl;
	}

	return lines;
}

std::string trim(const std::string& str) {
	auto start = str.begin();
	while (start != str.end() && std::isspace(*start)) {
		start++;
	}

	auto end = str.end();
	while (end != start && std::isspace(*(end - 1))) {
		end--;
	}

	return std::string(start, end);
}
#pragma once
#include <string>
#include <stdexcept>
#include "string_util.h"
class Cell
{
public:
	int row, col;
	Cell()
	{
		row = col = -1;
	}

	Cell(int row, int col) {
		this->row = row;
		this->col = col;
	}
	
	Cell operator-(const Cell& offset) const {
		return Cell(row - offset.row, col - offset.col);
	}
	Cell operator+(const Cell& offset) const {
		return Cell(row + offset.row, col + offset.col);
	}

	bool operator== (const Cell& c) const{
		return (row == c.row) && (col == c.col);
	}
	bool operator<(const Cell& c) const {
		if (row < c.row) return true;
		if (row > c.row) return false;

		return col < c.col;
	}
	std::string to_string() const {
		return std::to_string(row) + "_" + std::to_string(col);
	}
	static Cell fromString(const std::string& str)
	{
		Cell c;
		auto parts=split(str, '_');
		if (parts.size() != 2) throw std::runtime_error("invalid cell string format");
		c.row = stoi(parts[0]);
		c.col = stoi(parts[1]);
		return c;
	}
};


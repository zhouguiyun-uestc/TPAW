#pragma once

#include <map>
#include <Grid/flowDir.h>
#include <Grid/cell.h>
#include <Grid/grid.h>

struct BorderCellInfo
{
	static const int LABEL_UNKNOWN = -2; //indicate a cell flows to a cell outside its tile
	static const int UNLABELLED_OUTLET = -1; //indicate a cell has an unlabelled outlet 

	FlowDir flowDir; //flow dir of the border cell

	//global target cell, meaningful only when label==FlowOutside_Label, 
	//globalTargetCell==borderCell if flows immediately out of its tile
	//globalTargetCell!=borderCell if flows in and out of its tile
	Cell globalTargetCell; 

	//no_data_value in this tile or a valid label in this tile or FlowOutside_Label
	int label; 

	BorderCellInfo(FlowDir dir, Cell& targetCell, int label) :
		flowDir(dir), globalTargetCell(targetCell), label(label)
	{

	}
	BorderCellInfo() {

	}
};

//iterator to traverse border cell in a tile
struct NextBorderCellIter
{
	Cell curCell;
	int height, width;
	NextBorderCellIter(Grid<FlowDir>& dirGrid)
	{
		height = dirGrid.height();
		width = dirGrid.width();
		curCell = Cell(-1, -1);
	}

	bool Next(Cell& c)
	{
		//cells are accessed in clock-wise order
		if (curCell == Cell(-1, -1)) {
			c = curCell = Cell(0, 0);
			return true;
		}

		if (curCell.row == 0 && curCell.col < width - 1) {
			curCell.col++;
			c = curCell;
			return true;
		}

		if (curCell.row < height - 1 && curCell.col == width - 1) {
			curCell.row++;
			c = curCell;
			return true;
		}

		if (curCell.row == height - 1 && curCell.col > 0) {
			curCell.col--;
			c = curCell;
			return true;
		}

		if (curCell.row > 0 && curCell.col == 0) {
			curCell.row--;
			if (curCell.row == 0) return false; // the iteration should end
			c = curCell;
			return true;
		}
		return false;
	}
};

class LocalSolution
{
public:
	std::map<Cell, BorderCellInfo> borderCellInfoMap;
	std::map <Cell, int> interiorLocalOutlets;//outlet not included in the boder amp
	Cell gridCell;
	int rank=-1; // for identifying MPI process
	size_t transferredByte = 0;
};

class SolveLocal
{
public:
	LocalSolution solve(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid);
	void updateBorderCellLabel(LocalSolution& localSolution, std::map <Cell, int>& localInteriorOutlets,Grid<int>& wsGrid);
	void finalize(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid);
};

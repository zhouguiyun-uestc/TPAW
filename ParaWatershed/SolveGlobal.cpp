#include "SolveGlobal.h"

void SolveGlobal::computeTileDimension(const Cell& gridCell, int& height, int& width)
{
	height = (gridInfo.grandHeight - gridInfo.tileHeight * gridCell.row >= gridInfo.tileHeight) ? gridInfo.tileHeight : (gridInfo.grandHeight - gridInfo.tileHeight * gridCell.row);
	width = (gridInfo.grandWidth - gridInfo.tileWidth * gridCell.col >= gridInfo.tileWidth) ? gridInfo.tileWidth : (gridInfo.grandWidth - gridInfo.tileWidth * gridCell.col);
}

//find global solution
void SolveGlobal::solve()
{
	int gridWidth = gridLocalSolutions.width();
	int gridHeight = gridLocalSolutions.height();

	int no_data_value = gridInfo.no_data_value;

	//apply flow path traversal algorithm to the global graph
	std::vector<std::pair<Cell, Cell>> tracedBorderCells; //entry <gridCell, borderCell> 
	for (int gridRow = 0; gridRow < gridHeight; gridRow++)
	for (int gridCol = 0; gridCol < gridWidth; gridCol++)
	{
		auto& localSolution = gridLocalSolutions(gridRow, gridCol);

		for (auto& [cell, cellInfo] : localSolution.borderCellInfoMap) {

			Cell gridCell(gridRow, gridCol);
			Cell curCell = cell;
			
			//find the watershed id of this border source cell
			int id = BorderCellInfo::UNLABELLED_OUTLET;
			tracedBorderCells.clear();
			do
			{
				id = gridLocalSolutions(gridCell).borderCellInfoMap.at(curCell).label;
				if (id == BorderCellInfo::LABEL_UNKNOWN) //keep searching
					tracedBorderCells.emplace_back(gridCell, curCell);
				else
					break;

			} while (moveToDownstreamGlobalCell(gridCell, curCell));

			//
			if (id == no_data_value) id = BorderCellInfo::UNLABELLED_OUTLET;

			if (tracedBorderCells.size() > 0)
			{
				for (auto& [gridCell, borderCell] : tracedBorderCells)
					gridLocalSolutions(gridCell).borderCellInfoMap[borderCell].label = id;
			}
		}
	}
}

//Downstream global cell
bool SolveGlobal::moveToDownstreamGlobalCell(
	Cell& gridCell, //current tile in a grid
	Cell& cell)
{

	auto& localSolution = gridLocalSolutions(gridCell);
	auto& borderCellInfo = gridLocalSolutions(gridCell).borderCellInfoMap.at(cell);
#ifdef ENABLE_DEBUG
	if (borderCellInfo.label != BorderCellInfo::LABEL_UNKNOWN)
		throw std::runtime_error("the label of the border cell is invalid");
#endif

	if (cell != borderCellInfo.globalTargetCell)
	{
		// the cell flows to another global target cell (border cell or a labelled cell or NODATA cell) within the same tile
		//cell is updated as its target cell
		cell = borderCellInfo.globalTargetCell;
		return true;
	}

	//cell == borderCellInfo.globalTargetCell, flows immediately to another tile
	Cell exitBorderCell = borderCellInfo.globalTargetCell;
	auto& exitBorderCellInfo = borderCellInfo;
	int tileHeight, tileWidth, nTileHeight,nTileWidth;
	//compute the dimensions of current tile
	computeTileDimension(gridCell, tileHeight, tileWidth);
	
	if (exitBorderCellInfo.flowDir == FlowDir::Right) {
		gridCell.col++;
		cell.col=0;
	}
	else if (exitBorderCellInfo.flowDir == FlowDir::BottomRight) {
		if (exitBorderCell.row == tileHeight - 1) {
			gridCell.row++;
			cell.row = 0;
		}
		else
			cell.row++;

		if (exitBorderCell.col == tileWidth - 1)
		{
			gridCell.col++;
			cell.col = 0;
		}
		else
			cell.col++;
	}
	else if (exitBorderCellInfo.flowDir == FlowDir::Bottom) {
		gridCell.row++;
		cell.row=0;
	}
	else if (exitBorderCellInfo.flowDir == FlowDir::BottomLeft) {
		if (exitBorderCell.row == tileHeight - 1) {
			gridCell.row++;
			cell.row = 0;
		}
		else
			cell.row++;

		if (exitBorderCell.col == 0) {
			gridCell.col--;
			cell.col = gridInfo.tileWidth - 1; //this left tile must have a width of gridInfo.tileWidth
		}
		else
			cell.col--;
	}
	else if (exitBorderCellInfo.flowDir == FlowDir::Left) {
		gridCell.col--;
		cell.col = gridInfo.tileWidth - 1;
	}
	else if (exitBorderCellInfo.flowDir == FlowDir::TopLeft) {
		if (exitBorderCell.row == 0) {
			gridCell.row--;
			cell.row = gridInfo.tileHeight - 1;
		}
		else
			cell.row--;
		if (exitBorderCell.col == 0) {
			gridCell.col--;
			cell.col = gridInfo.tileWidth - 1;
		}
		else
			cell.col--;
	}
	else if (exitBorderCellInfo.flowDir == FlowDir::Top) {
		gridCell.row--;		
		cell.row = gridInfo.tileHeight - 1;
	}
	else if (exitBorderCellInfo.flowDir == FlowDir::TopRight) {
		if (exitBorderCell.col == tileWidth - 1) {
			gridCell.col++;
			cell.col=0;
		}
		else
			cell.col++;
		if (exitBorderCell.row == 0) {
			gridCell.row--;
			cell.row = gridInfo.tileHeight - 1;
		}
		else
			cell.row--;
	}

	//decide whether new tile indices are valid or not
	if (gridCell.row < 0 || gridCell.row >= gridInfo.gridHeight || gridCell.col < 0 || gridCell.col >= gridInfo.gridWidth)
		return false;

	//the exitTile is null or the new global cell is no_data;
	if (gridLocalSolutions(gridCell).borderCellInfoMap.count(cell)==0) return false;

	return true;
}



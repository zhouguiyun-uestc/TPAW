#include "SolveLocal.h"
#include <Grid/tool.h>
#include <FastWatershed/WatershedFlowPathTraversal.h>
#include <iostream>
#include <stdexcept>

//find local solution (part of the global graph, border cell labels and neighboring vertices) in a tile
LocalSolution SolveLocal::solve(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid) {
	if (dirGrid.width() <= 1 || dirGrid.height() <= 1)
		throw std::runtime_error("Both the width and height of a tile must be greater than 1");

	LocalSolution solution;
	//all local tile is expected to have the same NO_DATA value
	int no_data_value = wsGrid.noDataValue();

	NextBorderCellIter borderIter(dirGrid);
	Cell c;
	bool isDownstreamOutsideTile;
	//iterate over all border cells
	while (borderIter.Next(c))
	{
		//localSolution does not have entries for NO_DATA border cells
		if (dirGrid.isNoData(c)) continue;

		//if this border cell has a known label
		if (!wsGrid.isNoData(c)) {
			//should be a node of the global flow graph so that other upstream cells can reach it 
			solution.borderCellInfoMap[c] = BorderCellInfo(dirGrid(c), c, wsGrid(c));
			continue;
		}

		Cell cell = c;
		int id;
		do
		{
			if (moveToDownstreamCell(dirGrid, c, nullptr, &isDownstreamOutsideTile))
			{
				//downstream cell is a regular cell and located in current tile
				//c is updated as its downstream cell
				id = wsGrid(c);
				if (id != no_data_value) {
					//scenario 1, a pre-specified watershed label in this tile is encountered
					//an edge of the global graph within this tile is found
					solution.borderCellInfoMap[cell] = BorderCellInfo(dirGrid(cell), c, id);
					break;
				}
			}
			else {
				//no downstream cell
				if (isDownstreamOutsideTile)
				{					
					//in this case, c is not updated as its downstream cell. 
					// c must be a border cell and  might be an outlet
					//c is the target cell
					id = wsGrid(c);
					if (id == no_data_value) {
						//label of c is unknown
						id = BorderCellInfo::LABEL_UNKNOWN;
					}

					//scenario 2					 
					//solution to be determined, an edge is to be established between border cell to border 
					//either directly flow outside or through interior of this tile, depends on whether cell==c
					//the real label shall be resolved by global solution if cell is not an outlet
					solution.borderCellInfoMap[cell] = BorderCellInfo(dirGrid(cell), c, id);
				}
				else {
					//downstream cell of c is a NO_DATA cell, 
					//scenario 3
					//id = wsGrid(c);
					solution.borderCellInfoMap[cell] = BorderCellInfo(dirGrid(cell), c, BorderCellInfo::UNLABELLED_OUTLET);
				}
				break;
			}
		} while (true);
	}

	
	return solution;
}

//given global solution, update local labels
void SolveLocal::updateBorderCellLabel(LocalSolution& localSolution, std::map <Cell, int>& localInteriorOutlets,Grid<int>& wsGrid)
{
	//localSolution does not have entries for NO_DATA border cells
	for (auto& [cell, cellInfo] : localSolution.borderCellInfoMap) {
		if (cellInfo.label == BorderCellInfo::LABEL_UNKNOWN) //occurs when cell flows out of the entire DEM
			//wsGrid(cell) = wsGrid.noDataValue();
			wsGrid(cell) = BorderCellInfo::UNLABELLED_OUTLET;
		else 
			wsGrid(cell) = cellInfo.label;
	}

	//specify other global outlets
	for (auto& [cell, index] : localInteriorOutlets) {
		wsGrid(cell) = index;
	}

}

void SolveLocal::finalize(Grid<FlowDir>& dirGrid, Grid<int>& wsGrid)
{
	FlowPathTraversal(dirGrid, wsGrid, true);
}
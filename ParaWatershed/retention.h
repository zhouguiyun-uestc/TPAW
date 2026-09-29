#pragma once
#include <filesystem>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <Grid/grid.h>
#include <Grid/io_gdal.h>

//Retention policy for the intermediate flow-direction tiles produced between
//the two stages of the parallel watershed algorithm. This mirrors the
//'retention' option of richdem.
enum class Retention { Evict, Retain };

inline Retention parseRetention(const std::string& s)
{
    if (s == "@evict")  return Retention::Evict;
    if (s == "@retain") return Retention::Retain;
    throw std::invalid_argument("retention must be '@evict' or '@retain', got: '" + s + "'");
}

//Caches flow-direction tiles between stage 1 and stage 2 of the watershed
//algorithm.
//  @evict  - nothing is cached; stage 2 re-reads each tile from disk.
//  @retain - stage 1 keeps each tile in RAM; stage 2 takes it back from RAM,
//            avoiding the second disk read at the cost of holding every tile
//            assigned to the process in memory.
class TileFlowStore
{
    Retention mode_;
    std::map<Cell, Grid<FlowDir>> retained_;
    std::mutex mu_; //guards retained_ (put/get may run on OpenMP threads)

public:
    TileFlowStore(Retention mode) : mode_(mode) {}

    //Stage 1: hand the tile over for storage. In @evict mode the tile is
    //simply discarded.
    void put(const Cell& c, Grid<FlowDir>&& dir)
    {
        if (mode_ == Retention::Retain) {
            std::lock_guard<std::mutex> lock(mu_);
            retained_[c] = std::move(dir);
        }
    }

    //Stage 2: get the tile back, or re-read it from disk in @evict mode.
    Grid<FlowDir> get(const Cell& c, const std::filesystem::path& tilePath)
    {
        if (mode_ == Retention::Retain) {
            std::lock_guard<std::mutex> lock(mu_);
            auto node = retained_.extract(c);
            if (node.empty())
                throw std::runtime_error("TileFlowStore: no retained tile for cell " + c.to_string());
            return std::move(node.mapped());
        }
        return readRaster<FlowDir>(tilePath, false);
    }

    Grid<FlowDir> getOpenMP(
        const Cell& c,
        const std::filesystem::path& tilePath)
    {
        if (mode_ == Retention::Retain)
        {
            return get(c, tilePath);
        }

        return readRasterOpenMP<FlowDir>(tilePath);
    }
};

#pragma once
#include <cereal/cereal.hpp>
#include "SolveLocal.h"

template<class Archive>
void serialize(Archive& archive,
    Cell& m)
{
    archive(m.row, m.col);
}

template<class Archive>
void serialize(Archive& archive,
    BorderCellInfo& m)
{
    archive(m.flowDir, m.globalTargetCell, m.label);
}

template<class Archive>
void serialize(Archive& archive,
    LocalSolution& m)
{
    //in MPI, m.interiorLocalOutlets are cached on computing process and does not need to be serialized
    archive(m.borderCellInfoMap, m.gridCell, m.rank);
    //archive(m.borderCellInfoMap, m.interiorLocalOutlets,m.gridCell,m.rank);
}
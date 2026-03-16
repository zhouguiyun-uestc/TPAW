#pragma once
#include "SolveLocal.h"

void send(LocalSolution& localSol, int toRank);
LocalSolution receive(int fromRank);


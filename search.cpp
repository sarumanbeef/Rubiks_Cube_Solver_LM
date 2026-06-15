#include <iostream>
#include <algorithm>
#include <chrono>
#include "CubeStructure.h"
#include "PruneTables.h"

static const int numMoves = 18;
static size_t evalCount = 0;
std::chrono::steady_clock::time_point start;

// Only these 10 moves are allowed in Phase 2 to protect orientations and the E-slice!
static const MoveIndex Phase2Moves[] = {
    MoveIndex::U, MoveIndex::U_Prime, MoveIndex::U2,
    MoveIndex::D, MoveIndex::D_Prime, MoveIndex::D2,
    MoveIndex::R2, MoveIndex::L2, MoveIndex::F2, MoveIndex::B2
};
static const int numP2Moves = 10;

// Global ceiling trackers for the "Yield & Squeeze" optimization
int bestTotalMoves = 21;
std::vector<MoveIndex> bestPath;

// IDA* Search for Phase 2 Subgroup
static bool idaStarPhase2(CubeState currentCube, int currentDistance, int maxDistance, std::vector<MoveIndex> &p2Path, int boundaryFace) 
{
    evalCount++;

    // Calculate lower bound utilizing Phase 2 specific tables
    int CornerDistance     = CornerPermutationTable[GetCornerPermutation(currentCube)];
    int UDEdgeDistance     = UDEdgePermutationTable[GetUDEdgePermutation(currentCube)];
    int ESliceDistance     = ESlicePermutationTable[GetESlicePermutation(currentCube)];
    int estimatedMinDistance  = std::max({CornerDistance, UDEdgeDistance, ESliceDistance});

    // Prune branch if the heuristic proves we cannot reach the goal within the bound
    if (maxDistance < currentDistance + estimatedMinDistance) 
        return false;

    // Phase 2 fully solved
    if (estimatedMinDistance == 0)
        return true;

    // Generate constrained Phase 2 moves
    for (int i = 0; i < numP2Moves; i++) {
        MoveIndex move = Phase2Moves[i];

        int lastMoveFace;
        if (!p2Path.empty()) 
            lastMoveFace = static_cast<int>(p2Path.back()) / 3;
        else
            lastMoveFace = boundaryFace; // Prevents collision with Phase 1's final turn

        int currentMoveFace = static_cast<int>(move) / 3;

        if (lastMoveFace != -1) {
            // Optimization: Restrict turning the same face twice in a row
            if (lastMoveFace == currentMoveFace) continue; 

            // Optimization: Restrict commutative opposite face pairs to prevent duplicate branches
            if (lastMoveFace == 0 && currentMoveFace == 1) continue; // Block D after U
            if (lastMoveFace == 2 && currentMoveFace == 3) continue; // Block L after R
            if (lastMoveFace == 4 && currentMoveFace == 5) continue; // Block B after F
        }
                
        p2Path.push_back(move);
        CubeState nextCube = MakeMove(move, currentCube);

        if (idaStarPhase2(nextCube, currentDistance + 1, maxDistance, p2Path, boundaryFace)) 
            return true; // Solution found deeper down

        // Backtrack
        p2Path.pop_back();
    }

    return false;
}

// Master interface for bridging Phase 1 to Phase 2
void SolvePhaseTwo(CubeState startState, int maxPhase2Depth, std::vector<MoveIndex> p1Path) {
    int startDistance = std::max({CornerPermutationTable[GetCornerPermutation(startState)],
                                  UDEdgePermutationTable[GetUDEdgePermutation(startState)],
                                  ESlicePermutationTable[GetESlicePermutation(startState)]});
    
    // Only engage search if a theoretical improvement is mathematically possible
    if (startDistance <= maxPhase2Depth) {
        std::vector<MoveIndex> p2Path;
        int boundaryFace = -1;                                            
        if (!p1Path.empty()) {
            boundaryFace = static_cast<int>(p1Path.back()) / 3;
        }

        for(int p2Bound = startDistance; p2Bound <= maxPhase2Depth; p2Bound++) {
            if (idaStarPhase2(startState, 0, p2Bound, p2Path, boundaryFace)) {
                auto end = std::chrono::steady_clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                
                // Update the global ceiling with the new best run
                bestTotalMoves = p1Path.size() + p2Path.size();
                bestPath.clear();
                bestPath.insert(bestPath.end(), p1Path.begin(), p1Path.end());
                bestPath.insert(bestPath.end(), p2Path.begin(), p2Path.end());

                std::cout << "New solution found: " << bestTotalMoves << " moves!\n";
                std::cout << "Time Taken: " << elapsed_ms << "ms\n";
                std::cout << "Total Nodes Evaluated: " << evalCount << "\n";
                std::cout << "Move Sequence: ";

                for (MoveIndex m : bestPath) {
                    std::cout << MoveInString[static_cast<int>(m)] << " ";
                }
                std::cout << std::endl << std::endl;

                return;
            }
        }
    }
}

// IDA* Search for Phase 1 (Orientation & E-Slice Isolation)
void idaStarPhase1(CubeState currentCube, int currentDistance, int maxDistance, std::vector<MoveIndex> &p1Path) 
{
    evalCount++;

    int flipSliceDistance     = FlipSliceTable[GetFlipSliceIndex(currentCube)];
    int twistSliceDistance    = TwistSliceTable[GetTwistSliceIndex(currentCube)];
    int estimatedMinDistance  = std::max(flipSliceDistance, twistSliceDistance);

    if (maxDistance < currentDistance + estimatedMinDistance) 
        return;

    // Phase 1 Subgroup successfully reached!
    if (estimatedMinDistance == 0) {
        // Yield to Phase 2: Squeeze the depth limit to strictly beat the current record
        int maxPhase2Depth = bestTotalMoves - currentDistance - 1;
        SolvePhaseTwo(currentCube, maxPhase2Depth, p1Path);
        return; // Always return to force Phase 1 to keep exploring alternatives
    }

    // Generate all 18 moves
    for (int i = 0; i < numMoves; i++) {
        MoveIndex move = static_cast<MoveIndex>(i);

        if (!p1Path.empty()) {
            int lastMoveFace = static_cast<int>(p1Path.back()) / 3;
            int currentMoveFace = i / 3;

            if (lastMoveFace == currentMoveFace) continue; 
            if (lastMoveFace == 0 && currentMoveFace == 1) continue;
            if (lastMoveFace == 2 && currentMoveFace == 3) continue;
            if (lastMoveFace == 4 && currentMoveFace == 5) continue;
        }

        p1Path.push_back(move);
        CubeState nextCube = MakeMove(move, currentCube);

        idaStarPhase1(nextCube, currentDistance + 1, maxDistance, p1Path);

        p1Path.pop_back();
    }
}

// Master execution loop
void SolveCube(CubeState startState) {
    start = std::chrono::steady_clock::now();
    evalCount = 0;

    int p1StartDistance = std::max(FlipSliceTable[GetFlipSliceIndex(startState)], 
                                   TwistSliceTable[GetTwistSliceIndex(startState)]);

    std::vector<MoveIndex> path;

    // Loop automatically terminates when p1Bound eclipses the dynamically shrinking bestTotalMoves
    for(int p1Bound = p1StartDistance; p1Bound <= bestTotalMoves; p1Bound++)
        idaStarPhase1(startState, 0, p1Bound, path);
}
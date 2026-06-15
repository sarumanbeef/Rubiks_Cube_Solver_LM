#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <chrono>
#include <cstdint>
#include <bit>
#include "CubeStructure.h"

// Strict 10-move limit prevents destroying Phase 1 alignments
static const MoveIndex Phase2Moves[] = {
    MoveIndex::U, MoveIndex::U_Prime, MoveIndex::U2,
    MoveIndex::D, MoveIndex::D_Prime, MoveIndex::D2,
    MoveIndex::R2, MoveIndex::L2, MoveIndex::F2, MoveIndex::B2
};
static const int numP2Moves = 10;

int fac(int num) {
    int value = 1;
    for (int i = 2; i <= num; i++) value *= i;
    return value;
}

// Mathematical Lehmer mapping: converts unique sequences into a clean integer index
int GetPermutationIndex(std::vector<int> sequence) {
    int length = sequence.size();
    int index = 0;
    unsigned long currentNumBits = (1UL << length) - 1;
    for (int i = 0; i < length; i++) {
        int currentValue = sequence[i];

        unsigned long lowerMask = (1UL << currentValue) - 1;
        int numsLessThanCurrentValue = std::popcount(currentNumBits & lowerMask);

        index += numsLessThanCurrentValue * fac(length - 1 - i);
        currentNumBits &= ~(1UL << currentValue);
    }
    return index;
}

int GetCornerPermutation(CubeState cube) {
    std::vector<int> corners(8);
    for (int cornerLoc = 0; cornerLoc < 8; cornerLoc++) 
        corners[cornerLoc] = (cube.CornerState >> (BitsPerEntry * cornerLoc)) & 0b00111;

    return GetPermutationIndex(corners);
}

int GetUDEdgePermutation(CubeState cube) {
    std::vector<int> edges(8);
    
    // Iterate over the U Layer slots
    for (int edgeLoc = 0; edgeLoc < 4; edgeLoc++) {             
        int edgeID = ((cube.EdgeState >> (edgeLoc * BitsPerEntry)) & 0x0F);
        if (edgeID > 7) edgeID -= 4; // Normalize pieces displaced by R2/L2/F2/B2
        edges[edgeLoc] = edgeID;
    }

    // Iterate over the D Layer slots
    for (int edgeLoc = 8; edgeLoc < 12; edgeLoc++) {
        int edgeID = (cube.EdgeState >> (BitsPerEntry * edgeLoc)) & 0x0F;
        if (edgeID > 7) edgeID -= 4; 
        edges[edgeLoc - 4] = edgeID;
    }

    return GetPermutationIndex(edges);
}

int GetESlicePermutation(CubeState cube) {
    std::vector<int> edges(4);
    for (int edgeLoc = 4; edgeLoc < 8; edgeLoc++) { 
        int edgeID = (cube.EdgeState >> (BitsPerEntry * edgeLoc)) & 0x0F;
        edges[edgeLoc - 4] = edgeID - 4; // Shift absolute IDs 4-7 down to 0-3
    }

    return GetPermutationIndex(edges);
}

void generatePhaseTwoTable() {
    std::cout << "Generating Phase 2 Prune Tables...\n";
    // ... Standard BFS queue logic remains perfectly intact here ...

    const int CornerTableSize = 40320;
    const int UDEdgeTableSize = 40320;
    const int ESliceTableSize = 24;
    
    static std::vector<uint8_t> CornerPermutationTable(CornerTableSize, 255);
    static std::vector<uint8_t> UDEdgePermutationTable(UDEdgeTableSize, 255);
    static std::vector<uint8_t> ESlicePermutationTable(ESliceTableSize, 255);
    std::queue<CubeState> CubeQ;
    size_t evalCount = 1;

    CubeState solvedCube(SolvedEdgeBits, SolvedCornerBits);
    auto start = std::chrono::steady_clock::now();

    CornerPermutationTable[GetCornerPermutation(solvedCube)] = 0;
    solvedCube.distance = 0;
    CubeQ.push(solvedCube);

    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        uint8_t currentDist = currentCube.distance;

        for (int i = 0; i < numP2Moves; i++) {
            CubeState nextCube = MakeMove(Phase2Moves[i], currentCube);
            int nextCornerIndx = GetCornerPermutation(nextCube);
            evalCount++;

            if (CornerPermutationTable[nextCornerIndx] == 255) 
            {                                                          
                CornerPermutationTable[nextCornerIndx] = currentDist + 1;

                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }                    
        }
    }

    UDEdgePermutationTable[GetUDEdgePermutation(solvedCube)] = 0;
    solvedCube.distance = 0;
    CubeQ.push(solvedCube);

    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        uint8_t currentDist = currentCube.distance;

        for (int i = 0; i < numP2Moves; i++) {
            CubeState nextCube = MakeMove(Phase2Moves[i], currentCube);
            int nextUDIndx = GetUDEdgePermutation(nextCube);
            evalCount++;

            if (UDEdgePermutationTable[nextUDIndx] == 255)
            {                                                         
                UDEdgePermutationTable[nextUDIndx] = currentDist + 1;

                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }                   
        }
    }


    solvedCube.distance = 0;
    CubeQ.push(solvedCube);
    ESlicePermutationTable[GetESlicePermutation(solvedCube)] = 0;

    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        uint8_t currentDist = currentCube.distance;

        for (int i = 0; i < numP2Moves; i++) {
            CubeState nextCube = MakeMove(Phase2Moves[i], currentCube);
            int nextESliceIndx = GetESlicePermutation(nextCube);
            evalCount++;

            if (ESlicePermutationTable[nextESliceIndx] == 255) 
            {                                                        
                ESlicePermutationTable[nextESliceIndx] = currentDist + 1;

                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }                   
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "States Evaluated: " << evalCount << "\n";
    std::cout << "Time Taken: " <<  elapsed_ms << "ms\n";

    std::cout << "\nVerifying CornerPermutationTable Calculations...\n";
    int maxDepth = CornerPermutationTable[0];
    // Brute force and check if all indices have been filled
    for (int i = 0; i < CornerTableSize; i++) {
        if (CornerPermutationTable[i] > maxDepth) maxDepth = CornerPermutationTable[i];
        if (CornerPermutationTable[i] == 255) {
            std::cout << "Index " << i << " not filled\n";
            return;
        }
    }
    std::cout << "CornerPermutationTable Verification complete!\n";
    std::cout << "Max Depth : " << maxDepth << "\n\n";

    std::cout << "Verifying UDEdgePermutationTable Calculations...\n";
    maxDepth = UDEdgePermutationTable[0];
    // Brute force and check if all indices have been filled
    for (int i = 0; i < UDEdgeTableSize; i++) {
        if (UDEdgePermutationTable[i] > maxDepth) maxDepth = UDEdgePermutationTable[i];
        if (UDEdgePermutationTable[i] == 255) {
            std::cout << "Index " << i << " not filled\n";
            return;
        }
    }
    std::cout << "UDEdgePermutationTable Verification complete!\n";
    std::cout << "Max Depth : " << maxDepth << "\n\n";

    std::cout << "Verifying ESlicePermutationTable Calculations...\n";
    maxDepth = ESlicePermutationTable[0];
    // Brute force and check if all indices have been filled
    for (int i = 0; i < ESliceTableSize; i++) {
        if (ESlicePermutationTable[i] > maxDepth) maxDepth = ESlicePermutationTable[i];
        if (ESlicePermutationTable[i] == 255) {
            std::cout << "Index " << i << " not filled\n";
            return;
        }
    }
    std::cout << "ESlicePermutationTable Verification complete!\n";
    std::cout << "Max Depth : " << maxDepth << "\n\n";


    std::ofstream outputFile("Phase2PruneTables.txt");
    if (!outputFile) {
        std::cerr << "Error: Could not open or create the file!" << std::endl;
        return; 
    }

    std::cout << "Writing CornerPermutationTable...\n";
    outputFile << "\nconst std::vector<uint8_t> CornerPermutationTable = {";
    for (int i = 0; i < CornerTableSize; i++) {
        if (i % 32 == 0) outputFile << "\n    ";
        outputFile << std::format("{:2d}", static_cast<int>(CornerPermutationTable[i]));
        if (i != CornerTableSize - 1) outputFile << ", ";
    }
    outputFile << "\n};\n\n";
    std::cout << "Done.\n\n";

    std::cout << "Writing UDEdgePermutationTable...\n";
    outputFile << "\nconst std::vector<uint8_t> UDEdgePermutationTable = {";
    for (int i = 0; i < UDEdgeTableSize; i++) {
        if (i % 32 == 0) outputFile << "\n    ";
        outputFile << std::format("{:2d}", static_cast<int>(UDEdgePermutationTable[i]));
        if (i != UDEdgeTableSize - 1) outputFile << ", ";
    }
    outputFile << "\n};\n\n";
    std::cout << "Done.\n\n";

    std::cout << "Writing ESlicePermutationTable...\n";
    outputFile << "\nconst std::vector<uint8_t> ESlicePermutationTable = {";
    for (int i = 0; i < ESliceTableSize; i++) {
        if (i % 6 == 0) outputFile << "\n    ";
        outputFile << static_cast<int>(ESlicePermutationTable[i]);
        if (i != ESliceTableSize - 1) outputFile << ", ";
    }
    outputFile << "\n};\n";
    std::cout << "Done.\n";
}
#include <string>
#include <iostream>
#include "CubeStructure.h"



int main() {
    // Initialize a perfectly solved cube
	CubeState cube(SolvedEdgeBits, SolvedCornerBits);

	std::string scrambleSequence;
    std::cout << "Enter scramble sequence (eg: R U R' U' ... ) -> ";
    std::getline(std::cin, scrambleSequence);
    std::cout << "\n";
    
    // Apply the user's scramble and draw the unfolded 2D map
    cube = ApplySequence(cube, scrambleSequence);
	displayCube(cube);
    
    // Trigger the master Phase 1 -> Phase 2 search engine
	SolveCube(cube);

	return 0;
}
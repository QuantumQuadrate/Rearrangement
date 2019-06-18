// Rearangement.cpp : Defines the entry point for the console application.
//


#include "Rearrangement.h"
#include <algorithm>
//#include <iostream>
#include <cmath>
#include <limits>
//#include <fstream>
//#include <random>
//#include <utility>
//#include <chrono>




Rearranger::Rearranger() {
	this->top = 0;
	this->left = 0;
	this->camSize = 0;
	this->xdim = 0;
	this->ydim = 0;
	this->numAtoms = 0;
	this->maskLength = 0;
	this->costRows = 0;
	this->costCols = 0;
}

Rearranger::~Rearranger() {
}

void Rearranger::setPattern(int pattern[], int top, int left, float thresh[], float mask[], int width, int height, int camLength) {
	/*
	Reset all relevant variables for performing rearrangement to be compatible with the new provided architecture.
	All variables that can be computed prior to receiving camera data are dealt with here.
	Arguments:
	int pattern[] -- Array of integers that describes where we want and don't want atoms.
	A 1 indicates we want an atom at a site.
	A 2 indicates we don't care about a site.
	Anything else means we don't want an atom at a site.
	int top -- The y coordinate for the top-left corner of the region that we perform rearrangement in
	int left -- the x coordinate for the top-left corner of the region that we perform rearrangement in
	float thresh[] -- Thresholds to be used to determine atom occupancy
	float mask[] -- Masks to be used to determine atom occupancy
	int width -- How many atoms wide (x dimension) is the region we are performing matching in
	int height -- How many atoms tall (y dimension) is the region we are performing matching in
	int camLength -- The number of camera pixels we will recieve to determine atom occupancy
	*/

	//Assign all the values describing the geometry of the problem and the size of the incoming camera data
	this->top = top;
	this->left = left;
	this->camSize = camLength;
	this->xdim = width;
	this->ydim = height;
	this->numAtoms = width * height;
	this->maskLength = this->numAtoms*camLength;

	//Empty and refill pre-computable pattern information
	clearPatternMemory();
	reservePatternMemory();
	//Populate the precomputable vectors
	int index = 0;
	for (int r = 0; r < this->ydim; ++r) {
		for (int c = 0; c < this->xdim; ++c) {
			this->pattern.push_back(pattern[index]);
			this->thresholds.push_back(thresh[index]);
			switch (pattern[index]) {
			case 1:
			{
				this->desiredSites.push_back(coordinate(r+this->top, c+this->left));
				this->sinkSites.push_back(coordinate(r+this->top, c+this->left));
				break;
			}
			case 2:
			{
				this->sinkSites.push_back(coordinate(r+this->top, c+this->left));
				break;
			}
			default: {
				this->badSites.push_back(coordinate(r+this->top, c+this->left));
				break;
			}
			}
			index += 1;
		}
	}
	for (int ind = 0; ind < maskLength; ++ind) {
		this->masks.push_back(mask[ind]);
	}

	//Reset pre-computable assignment information
	resetAssignmentData();
}

void Rearranger::clearPatternMemory() {
	/*
	Empty all vectors containing information about the current architecture to make way for new information
	*/
	this->desiredSites.clear();
	this->sinkSites.clear();
	this->badSites.clear();
	this->pattern.clear();
	this->occupiedCoords.clear();
	this->thresholds.clear();
	this->masks.clear();
}

void Rearranger::reservePatternMemory() {
	/*
	Reserve the maximum amount of space we might need for each vector to avoid needing
	to expand the underlying array during rearrangement.
	*/
	this->desiredSites.reserve(this->numAtoms);
	this->sinkSites.reserve(this->numAtoms);
	this->badSites.reserve(this->numAtoms);
	this->pattern.reserve(this->numAtoms);
	this->occupiedCoords.reserve(this->numAtoms);
	this->thresholds.reserve(this->numAtoms);
	this->masks.reserve(this->maskLength);
}

float Rearranger::square(int x) {
	/*Return the square of an integer as a floating point variable
	Arguments:
	int x -- The variable we want to square
	Returns:
	The square of the given argument as a floating point value
	*/
	return float(x*x);
}

void Rearranger::resetAssignmentData() {
	/*
	Clear all memory associated with previous assignment.
	Reset any pre-computable values to defaults.
	*/
	this->costRows = desiredSites.size();
	this->cost.clear();
	this->starredRowInCol.clear();
	this->isCoveredCol.clear();
	this->moves.clear();
	this->primedColInRow.clear();
	this->starredColInRow.clear();
	this->isCoveredRow.clear();
	this->occupiedCoords.clear();
	this->primedColInRow.resize(costRows, -1);
	this->starredColInRow.resize(costRows, -1);
	this->isCoveredRow.resize(costRows, false);
}

std::string Rearranger::generateInstructionsCorner(unsigned short cameraData[]) {
	/*
	Given an array containing the camera data, return string formatted to describe rearrangement instructions.
	Strings are formatted as follows:
	u>Number of moves>Starty>Startx>Endy>Endx>......
	So an example string would be
	u>2>1>2>3>3>6>6>-1>-1>
	The 2 after the u indicates that there are two moves the microcontroller has to perform.
	This first move sends site (1,2) to (3,3)
	The second move ends in (-1,-1) which signals that the microcontroller should eject this atom from the array.
	All ejection moves are done after the normal movement-based moves.
	Arguments:
	unsigned short cameraData[]:
	camera pixel counts measuring atom flourescence
	Returns:
	std::string formatted as described above to encode rearrangement instructions for the microcontroller
	*/

	//Populate the occupiedCoords vector with locations of atoms
	occupation(cameraData);

	//Initialize the remaining Hungairan Matching Algorithm data 
	costCols = occupiedCoords.size();
	smallest = std::min(costRows, costCols);
	cost.reserve(costRows*costCols);
	starredRowInCol.resize(costCols, -1);
	isCoveredCol.resize(costCols, false);

	//Generate cost matrx
	//Cost Function used is ( (x1-x2)^2 + (y1-y2)^2 )^1.01
	//The reason this isn't the normal Euclidean distance or Taxi-Cab distance is to
	//Favor certain special cases over others which should improve movement time
	for (int r = 0; r < costRows; ++r) {
		for (int c = 0; c < costCols; ++c) {
			cost.push_back(std::pow(std::pow(std::abs(occupiedCoords[c].x - desiredSites[r].x),1.001f) + std::pow(std::abs(occupiedCoords[c].y - desiredSites[r].y),1.001f), 1.01f));
		}
	}

	//Perform matching using the Hungarian Matching Algorithm
	if (costRows > costCols) {
		step0();
	}
	else {
		step1();
	}

	//Extract list of moves to be served from the matching
	for (int c = 0; c < costCols; ++c) {
		//Check if this atom has been given a task and that the task isn't to stay still
		if (starredRowInCol[c] > -1 && occupiedCoords[c] != desiredSites[starredRowInCol[c]]) {
			moves.push_back(move(occupiedCoords[c], desiredSites[starredRowInCol[c]]));
		}
	}

	//Order the moves and add the special corner moves to remove unwanted atoms
	return orderMovesCorner();
}

void Rearranger::occupation(unsigned short cameraData[]) {
	/*
	Given an array of camera data, determine which sites are occupied in the lattice
	Each site that is found to be loaded is added to the occupiedCoords vector
	Arguments:
	unsigned short cameraData[]:
	camera pixel counts measuring atom flourescence
	*/
	float innerProduct;
	int maskIndex = 0;
	int atom = 0;
	for (int row = 0; row < this->ydim; ++row) {
		for (int col = 0; col < this->xdim; ++col)
		{
			innerProduct = 0;
			for (int camIndex = 0; camIndex < this->camSize; ++camIndex) {
				innerProduct += cameraData[camIndex] * this->masks[maskIndex];
				maskIndex += 1;
			}
			if (innerProduct > this->thresholds[atom]) {
				this->occupiedCoords.push_back(coordinate(row + this->top, col + this->left));
			}
			atom += 1;
		}
	}
}

void Rearranger::step0() {
	/*
	Step 0:
	Subtract the smallest element in each column from every element in that column.
	Go To:
	Step 2
	*/
	float minimum;
	for (int c = 0; c < costCols; ++c) {
		minimum = cost[c];
		for (int r = 0; r < costRows; ++r) {
			if (minimum > cost[r*costCols + c]) {
				minimum = cost[r*costCols + c];
			}
		}
		for (int r = 0; r < costRows; ++r) {
			cost[r*costCols + c] -= minimum;
		}
	}
	step2();
}

void Rearranger::step1() {
	/*
	Step 1:
	Subtract the smallest element in each row from every element in that row.
	Go To:
	Step 2
	*/
	float minimum;
	int costSize = costRows * costCols;
	//Iterate over each row with the costCols stride due to the 1D array storage
	for (int rowStart = 0; rowStart < costSize; rowStart += costCols) {
		minimum = cost[rowStart];
		//Identify the minimum value in the row
		for (int c = 0; c < costCols; ++c) {
			if (minimum > cost[rowStart + c]) {
				minimum = cost[rowStart + c];
			}
		}
		//Subtract the minimum value in the row from each value in the row
		for (int c = 0; c < costCols; ++c) {
			cost[rowStart + c] -= minimum;
		}

	}
	step2();
}

void Rearranger::step2() {
	/*
	Step 2:
	Iterate through each element of the matrix.
	For each zero found, check if there is a starred zero in its row or column.
	If not, star the found zero.
	Go To:
	Step 3
	*/
	int costIndex = 0;
	for (int r = 0; r < costRows; ++r) {
		for (int c = 0; c < costCols; ++c) {
			if (cost[costIndex] == 0) {
				if (starredColInRow[r] == -1 && starredRowInCol[c] == -1) {
					starredRowInCol[c] = r;
					starredColInRow[r] = c;
				}
			}
			costIndex += 1;
		}
	}
	step3();
}

void Rearranger::step3() {
	/*
	Step 3:
	Cover every starred zero.
	If enough columns are covered, the starred zeros form the desired solution and we terminate.
	Go To:
	Step 4
	*/
	int cov_count = 0;
	for (int r = 0; r < costRows; ++r) {
		for (int c = 0; c < costCols; ++c) {
			if (starredRowInCol[c] == r) {
				isCoveredCol[c] = true;
				cov_count += 1;
			}
		}
	}
	if (cov_count != smallest) {
		step4();
	}

}

void Rearranger::step4() {
	/*
	Step 4:
	Choose an uncovered zero and prime it.
	If there is no starred zero in the row of the chosen zero, Go To Step 5.
	If there is a starred zero, cover the row and uncover the column of the chosen zero.
	Repeat until all zeros are covered and Go To Step 6.
	*/
	bool proceed = true;
	int row, col;
	while (proceed) {
		//find_zero returns true if it finds a zero and stores the row/col of that zero in row and col
		if (find_zero(&row, &col))
		{	//An uncovered zero is found
			primedColInRow[row] = col;
			if (starredColInRow[row] == -1)
			{	// No starred zeros in the row
				step5(row, col);
				proceed = false;

			}
			else
			{	//A starred zero was found in the row
				isCoveredCol[starredColInRow[row]] = false;
				isCoveredRow[row] = true;
			}
		}
		else
		{	//No uncovered zeros were found
			step6();
			proceed = false;

		}
	}
}

void Rearranger::step5(int row, int col) {
	/*
	Step 5:
	row and col refer to the row and column of a primed zero to start a sequence.
	Let Z0 refer to the primed zero we have found prior to step 5.
	Let Z1 denote a starred zero in Z0's column (if any).
	If a starred zero is found, there is garunteed to be a primed zero in its row.
	Let Z2 denote the primed zero in Z1's row.
	Continue the sequence until there is a primed zero with no starred zero in its column.
	Unstar each starred zero in the sequence and star the primed zeros.
	Unprime every zero and uncover every row and column.
	Go To:
	Step 3
	*/
	//Construct the sequence
	std::vector<row_col_pair> path;
	path.push_back(row_col_pair(row, col));
	row = starredRowInCol[col];
	while (row != -1) {
		path.push_back(row_col_pair(row, col));
		col = primedColInRow[row];
		path.push_back(row_col_pair(row, col));
		row = starredRowInCol[col];
	}
	//Unstarring needs to happen before starring because of how the star memory is stored

	//Unstar all the starred members of the sequence (those with odd index values)
	for (unsigned int starredIndex = 1; starredIndex < path.size(); starredIndex += 2) {
		starredRowInCol[path[starredIndex].col] = -1;
		starredColInRow[path[starredIndex].row] = -1;
	}
	//Star all of the primed members of the sequence (those with even index values)
	for (unsigned int primeIndex = 0; primeIndex < path.size(); primeIndex += 2) {
		starredRowInCol[path[primeIndex].col] = path[primeIndex].row;
		starredColInRow[path[primeIndex].row] = path[primeIndex].col;
	}

	//Unprime and Uncover everything
	for (int r = 0; r < costRows; ++r) {
		primedColInRow[r] = -1;
		isCoveredRow[r] = false;
	}
	for (int c = 0; c < costCols; ++c) {
		isCoveredCol[c] = false;
	}
	step3();
}

void Rearranger::step6() {
	/*
	Step 6:
	Find the lowest uncovered value in matrix.
	Add it to each covered row.
	Subtract it from each uncovered column
	Go To:
	Step 4
	*/
	float min = std::numeric_limits<float>::infinity();
	int costIndex = 0;
	for (int r = 0; r < costRows; ++r) {
		for (int c = 0; c < costCols; ++c) {
			if (!(isCoveredRow[r] || isCoveredCol[c])) {
				if (cost[costIndex] < min) {
					min = cost[costIndex];
				}
			}
			costIndex += 1;
		}
	}

	for (int r = 0; r < costRows; ++r) {
		costIndex = r * costCols;
		if (isCoveredRow[r]) { //covered row
			for (int c = 0; c < costCols; ++c) {
				cost[costIndex] += min;
				costIndex += 1;
			}
		}
	}

	for (int c = 0; c < costCols; ++c) {
		costIndex = c;
		if (!isCoveredCol[c]) { // uncovered column
			for (int r = 0; r < costRows; ++r) {
				cost[costIndex] -= min;
				costIndex += costCols;
			}
		}
	}
	step4();
}

bool Rearranger::find_zero(int* row, int* col) {
	/*
	Search for an uncovered zero in the cost matrix
	Arguments:
	int* row: pointer to variable that will contain the row of a found zero
	int* col: pointer to variable that will contain the column of a found zero
	Returns:
	bool that is true if an uncovered zero was found and false otherwise
	*/
	int costIndex = 0;
	for (int r = 0; r < costRows; ++r) {
		for (int c = 0; c < costCols; ++c) {
			if (cost[costIndex] == 0) {
				if (!(isCoveredRow[r] || isCoveredCol[c])) {
					*row = r;
					*col = c;
					return true;
				}
			}
			costIndex += 1;
		}
	}
	return false;
}

std::string Rearranger::orderMovesCorner() {
	/*
	Order the moves so that we have no atom collisions and book-end the list with the special moves
	Returns:
	std::string formatted as described in the docstring of the generateInstructionsCorner() function
	*/
	std::string directions = "u>";
	size_t numCornerMoves = 0;
	std::string cornerMoveString = "";
	bool wasOccupied, stayedStill;
	//For each coordinate in our pattern that we want empty,
	//determine if it was occupied and will stay still based on the matching.
	//If so, add a -1,-1 move to the end of the list which will pull it into a corner
	for (unsigned int badIndex = 0; badIndex < badSites.size(); ++badIndex) { //Iterate over each coordinate we want empty
		wasOccupied = false;
		stayedStill = true;;
		for (unsigned int occIndex = 0; occIndex < occupiedCoords.size(); ++occIndex) { //Determine if that site was occupied
			if (badSites[badIndex] == occupiedCoords[occIndex]) {
				wasOccupied = true;
				break;
			}
		}
		if (wasOccupied) {
			for (unsigned int moveIndex = 0; moveIndex < moves.size(); ++moveIndex) { //Determine if the instructions so far moved it
				if (badSites[badIndex] == moves[moveIndex].start) {
					stayedStill = false;
					break;
				}
			}
			if (stayedStill) { //If it stayed still we add it as a corner move to the end of the normal list
				cornerMoveString += std::to_string(badSites[badIndex].y) + ">" + std::to_string(badSites[badIndex].x) + ">-1>-1>";
				numCornerMoves += 1;
			}
		}
	}
	size_t totalMoves = numCornerMoves + moves.size();
	return directions + std::to_string(totalMoves) + ">" + orderMoves() + cornerMoveString;
}

std::string Rearranger::orderMoves() {
	/*
	Order the moves that the Hungarian Algorithm decided on to avoid atom collisions
	Returns:
	std::string that encodes the moves that actually perform movement
	*/

	std::string directions = "";
	while (moves.size() > 0) {
		//Keep going as long as there is another move that needs to be served
		unsigned int testMove = 0;
		while (testMove < moves.size()) {
			//Each iteration of the second outermost while loop tests to see if a specific move can go next
			//At least one such move will be found before the test condition on the loop becomes false
			//It is possible that more than one will be found per iteration of the loop though
			unsigned int index = 0;
			while (index < moves.size()) {
				//Each iteration of the innermost while loop tests to see if a specific move must go before the current test move
				//If so, the testMove can't go and we break out as further testing isn't necessary
				if (moveABeforeB(moves[index], moves[testMove])) break;
				index++;
			}

			//If index = moves.size then we didn't hit the break statement in the innermost loop and the move can go
			//otherwise the innermost loop was broken out of and we want to test the next move
			if (index == moves.size()) {
				//add the current move to the directions string
				directions += moves[testMove].to_string();
				//swap current move with last move in vector
				std::swap(moves[testMove], moves[moves.size() - 1]);
				//pop last move in vector
				moves.pop_back();
			}
			testMove++;
		}

	}
	return directions;
}

bool Rearranger::moveABeforeB(move a, move b) {
	//Returns true if move A needs to be served before move B to prevent collisions
	return (a.start != b.start) && (coordInMovesWay(a, b.end) || coordInMovesWay(b, a.start));
}

bool Rearranger::coordInMovesWay(move path, coordinate coord) {
	/*
	Given a move path and coordinate coord, determine if a collision would be caused by
	an atom moving from the start of path to the end of path if the coord is loaded
	*/
	//vertical movement first so corner x is same as start x and corner y is same as end y
	//returns whether the given coordinate is contained in the given path
	coordinate corner = coordinate(path.end.y, path.start.x);
	if (coord.y == corner.y) { //coordinates are in same column and so we check if the x values work out
		int minx = std::min(path.end.x, corner.x);
		int maxx = std::max(path.end.x, corner.x);
		return minx <= coord.x && coord.x <= maxx;
	}
	else if (coord.x == corner.x) { //coordinates are in the same row so we check if the y values work out
		int miny = std::min(path.start.y, corner.y);
		int maxy = std::max(path.start.y, corner.y);
		return miny <= coord.y && coord.y <= maxy;
	}
	return false;
}

///*
int main() {
	return 0;
}
//*/

/*
int main() {
//using namespace rearrangement;
int pattern[] = { 2,2,2,2,2,2,2,   2,0,0,0,0,0,2, 2,0,1,1,1,0,2, 2,0,1,1,1,0,2, 2,0,1,1,1,0,2, 2,0,0,0,0,0,2, 2,2,2,2,2,2,2};
int top = 0;
int left = 0;
int width = 7;
int height = 7;
int camlength = 49;
unsigned short cam[] =      { 0,0,1,0,0,0,0,  1,1,1,0,0,1,0,  1,0,0,0,1,0,1,  0,0,1,0,0,0,1, 1,1,0,1,0,0,0, 1,1,0,1,1,1,1, 0,0,1,0,0,1,0 };
float thresh[] = { 1,1,1,1,1,1,1,  1,1,1,1,1,1,1,  1,1,1,1,1,1,1,  1,1,1,1,1,1,1, 1,1,1,1,1,1,1, 1,1,1,1,1,1,1, 1,1,1,1,1,1,1 };
float mask[2401];
for (int index = 0; index < 2401; ++index) {
if (index % 50 == 0) mask[index] = 2;
else mask[index] = 0;
}
Rearranger reeee;
std::string out;
int repetitions = 10000;
reeee.setPattern(pattern, top, left, thresh, mask, width, height, camlength);
typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::chrono::duration<float> fsec;
auto t0 = Time::now();
for (int i = 0; i < repetitions; ++i) {
reeee.generateInstructionsCorner(cam);
reeee.resetAssignmentData();
}
auto t1 = Time::now();
fsec fs = t1 - t0;
std::cout << fs.count() / repetitions << std::endl;
std::cout << reeee.generateInstructionsCorner(cam) << std::endl;
reeee.resetAssignmentData();

int pattern2[] = { 2, 1,   2, 1 };
width = 2;
height = 2;
camlength = 4;
unsigned short cam2[] = { 0,0,1,1 };
float thresh2[] = { 1,1,1,1 };
float mask2[16];
for (int index = 0; index < 16; ++index) {
if (index % 5 == 0) mask2[index] = 2;
else mask2[index] = 0;
}

reeee.setPattern(pattern2, top, left, thresh2, mask2, width, height, camlength);
std::cout << reeee.generateInstructionsCorner(cam2) << std::endl;
reeee.resetAssignmentData();

return 0;
}
*/

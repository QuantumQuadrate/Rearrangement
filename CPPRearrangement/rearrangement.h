#pragma once

#include <vector>
#include <stdlib.h>
#include <string>

struct row_col_pair {
	int row;
	int col;
	row_col_pair(int arow, int acol) : row(arow), col(acol) {}
	bool operator==(const row_col_pair &b) {
		return (row == b.row && col == b.col);
	};
	bool operator!=(const row_col_pair &b) {
		return (row != b.row || col != b.col);
	};
};

struct coordinate {
	int y;
	int x;
	coordinate(int ay, int ax) : y(ay), x(ax) {}
	bool operator==(const coordinate &b) {
		return (y == b.y && x == b.x);
	};
	bool operator!=(const coordinate &b) {
		return (y != b.y || x != b.x);
	};
};

struct move {
	coordinate start;
	coordinate end;
	char buffer[65];
	move(coordinate astart, coordinate aend) : start(astart), end(aend) {}
	bool operator==(const move &b) {
		return (start == b.start && end == b.end);
	};
	bool operator!=(const move &b) {
		return (start != b.start || end != b.end);
	};
	std::string to_string() {
		return std::to_string(start.y)+">"+std::to_string(start.x)+">"+std::to_string(end.y)+">"std::to_string(end.x)+">";
	};
};

class Rearranger {
	int top; // Y coordinate of top left corner of pattern;
	int left; // X coordinate of top left corner of pattern;
	int camSize; // How many pixels are in the camera data;
	int xdim; //Width of the pattern;
	int ydim; //Height of the pattern;
	int numAtoms; //xdim*ydim;
	int maskLength; //numAtoms*camSize
	std::vector<coordinate> sinkSites; //coordinates where atoms are allowed to be
	std::vector<coordinate> desiredSites; //coordinates where we want atoms to be
	std::vector<coordinate> badSites; //coordinates we want empty
	std::vector<coordinate> occupiedCoords; //coordinates of occupied sites
	std::vector<float> thresholds; //thresholds for each site in the pattern
	std::vector<float> masks; //masks for each site in the pattern
	std::vector<int> pattern; //array encoding the distribution of sink, desired, and bad sites
	std::vector<float> cost; //cost matrix
	std::vector<int> primedColInRow; //stores which column in each row has a primed zero
	std::vector<int> starredRowInCol; //stores which row in each column has a starred zero
	std::vector<int> starredColInRow; //stores which column in each row has a starred zero
	std::vector<bool> isCoveredRow; //Tells whether each row is covered
	std::vector<bool> isCoveredCol; //Tells whether each column is covered
	std::vector<move> moves;
	size_t costRows; //Number of rows in the cost matrix (places to put atoms)
	size_t costCols; //Number of columns in the cost matrix (number of available atoms)
	size_t smallest; //Lesser of costRows and costCols

					 //Hungarian Algorithm support methods
	void step0();
	void step1();
	void step2();
	void step3();
	void step4();
	void step5(int, int);
	void step6();
	bool find_zero(int*, int*);
	std::string orderMovesCorner();
	std::string orderMoves();
	bool moveABeforeB(move, move);
	bool coordInMovesWay(move, coordinate);
	void occupation(unsigned short[]);
	void clearPatternMemory();
	void reservePatternMemory();
	float square(int x);
	std::string to_string(size_t x);

public:
	Rearranger();
	~Rearranger();
	void setPattern(int[], int, int, float[], float[], int, int, int);
	std::string generateInstructionsCorner(unsigned short[]);
	void resetAssignmentData();




};


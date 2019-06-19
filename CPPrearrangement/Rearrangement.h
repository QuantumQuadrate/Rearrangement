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
    return std::to_string(start.y) + ">" + std::to_string(start.x) + ">" + std::to_string(end.y) + ">"
        + std::to_string(end.x) + ">";
  };
};

class Rearranger {

 private:
  int top;         //!< Y coordinate of top left corner of pattern
  int left;        //!< X coordinate of top left corner of pattern
  int xdim;        //!< Width of the pattern
  int ydim;        //!< Height of the pattern
  int numAtoms;    //!< Number of atoms in the pattern

  std::vector<coordinate> sinkSites;      //<! coordinates where atoms are allowed to be
  std::vector<coordinate> desiredSites;   //<! coordinates where we want atoms to be
  std::vector<coordinate> badSites;       //<! coordinates we want empty
  std::vector<coordinate> occupiedCoords; //<! coordinates of occupied sites

  std::vector<int> pattern;         //<! array encoding the distribution of sink, desired, and bad sites
  std::vector<float> cost;          //<! cost matrix
  std::vector<int> primedColInRow;  //<! stores which column in each row has a primed zero
  std::vector<int> starredRowInCol; //<! stores which row in each column has a starred zero
  std::vector<int> starredColInRow; //<! stores which column in each row has a starred zero
  std::vector<bool> isCoveredRow;   //<! Tells whether each row is covered
  std::vector<bool> isCoveredCol;   //<! Tells whether each column is covered
  size_t costRows;                  //<! Number of rows in the cost matrix (places to put atoms)
  size_t costCols;                  //<! Number of columns in the cost matrix (number of available atoms)
  size_t smallest;                  //<! Lesser of costRows and costCols

  std::vector<move> moves;          //<! A list of moves to perform

  // Hungarian Algorithm support methods
  void step0();
  void step1();
  void step2();
  void step3();
  void step4();
  void step5(int, int);
  void step6();
  bool find_zero(int *, int *);
  std::string orderMovesCorner();
  std::string orderMoves();
  bool moveABeforeB(move, move);
  bool coordInMovesWay(move, coordinate);
  void occupation(unsigned short[]);
  void clearPatternMemory();
  void reservePatternMemory();
  std::string to_string(size_t x);

 public:
  Rearranger();
  ~Rearranger();

  //! Reset all relevant variables for performing rearrangement to be compatible with the new provided architecture.
  /*!
   * Reset all relevant variables for performing rearrangement to be compatible with the new provided architecture.
   * All variables that can be computed prior to receiving camera data are dealt with here.
   * Arguments:
   *    int pattern[] -- Array of integers that describes where we want and don't want atoms.
   *        A 1 indicates we want an atom at a site.
   *        A 2 indicates we don't care about a site.
   *        Anything else means we don't want an atom at a site.
   *    int top -- The y coordinate for the top-left corner of the region that we perform rearrangement in
   *    int left -- the x coordinate for the top-left corner of the region that we perform rearrangement in
   *    int width -- How many atoms wide (x dimension) is the region we are performing matching in
   *    int height -- How many atoms tall (y dimension) is the region we are performing matching in
   */
  void setPattern(int pattern[], int top, int left, int width, int height);

  //! Generate a sequence of moves that
  /*!
   * Given an array containing the camera data, return string formatted to describe rearrangement instructions.
   *
   * Strings are formatted as follows:
   *    u>Number of moves>Starty>Startx>Endy>Endx>......
   *    So an example string would be
   *    u>2>1>2>3>3>6>6>-1>-1>
   *    The 2 after the u indicates that there are two moves the microcontroller has to perform.
   *    This first move sends site (1,2) to (3,3)
   *    The second move ends in (-1,-1) which signals that the microcontroller should eject this atom from the array.
   *    All ejection moves are done after the normal movement-based moves.
   *
   * @return std::string formatted as described above to encode rearrangement instructions for the microcontroller
   */
  std::string generateInstructionsCorner(uint8_t occupationVector[]);
  void resetAssignmentData();
};

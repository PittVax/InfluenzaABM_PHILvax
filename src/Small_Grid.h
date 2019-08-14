//
//
// File: Small_Grid.h
//

#ifndef _PHIL_SMALL_GRID_H
#define _PHIL_SMALL_GRID_H

#include "Global.h"
#include "Abstract_Grid.h"
class Large_Grid;
class Small_Cell;

class Small_Grid : public Abstract_Grid {
  public:
    Small_Grid(Large_Grid * lgrid);
    ~Small_Grid() {}
    void get_parameters();
    Small_Cell ** get_neighbors(int row, int col);
    Small_Cell * get_grid_cell(int row, int col);
    Small_Cell * get_grid_cell(phil::geo lat, phil::geo lon);
    Small_Cell * select_random_grid_cell();
    void quality_control(char * directory);

    // Specific to Small_Cell grid:
    void initialize_gaia_data(char * directory, int run);
    void print_gaia_data(char * directory, int run, int day);

  protected:
    Small_Cell ** grid;            // Rectangular array of grid_cells
    Large_Grid * large_grid;

    // Specific to Small_Cell grid:
    void print_population_data(char * dir, int disease_id, int day);
    void print_output_data(char * dir, int disease_id, int output_code, char * output_str, int day);
};

#endif // _PHIL_SMALL_GRID_H

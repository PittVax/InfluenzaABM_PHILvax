//
//
// File: Large_Grid.h
//

#ifndef _PHIL_LARGE_GRID_H
#define _PHIL_LARGE_GRID_H

#include <string.h>
#include <math.h>

#include "Place.h"
#include "Abstract_Grid.h"
#include "Global.h"

class Large_Cell;

class Large_Grid : public Abstract_Grid {
  public:
    Large_Grid(phil::geo minlon, phil::geo minlat, phil::geo maxlon, phil::geo maxlat);
    ~Large_Grid() {}
    Large_Cell ** get_neighbors(int row, int col);
    Large_Cell * get_grid_cell(int row, int col);
    Large_Cell * get_grid_cell(phil::geo lat, phil::geo lon);
    Large_Cell * get_grid_cell_with_global_coords(int row, int col);
    Large_Cell * get_grid_cell_from_id(int id);
    Large_Cell * select_random_grid_cell();
    Place * get_nearby_workplace(int row, int col, double x, double y, int min_staff, int max_staff, double * min_dist);
    void get_parameters();
    void set_population_size();
    void quality_control(char * directory);
    void read_max_popsize();

  protected:
    Large_Cell ** grid;            // Rectangular array of grid_cells
};

#endif // _PHIL_LARGE_GRID_H

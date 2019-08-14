
#ifndef _ABSTRACT_GRID_H
#define _ABSTRACT_GRID_H

#include "Global.h"
#include "Geo_Utils.h"

class Abstract_Grid {

  public:

    int get_rows() {
        return rows;
    }
    int get_cols() {
        return cols;
    }
    int get_number_of_cells() {
        return rows*cols;
    }
    phil::geo get_min_lat() {
        return min_lat;
    }
    phil::geo get_min_lon() {
        return min_lon;
    }
    phil::geo get_max_lat() {
        return max_lat;
    }
    phil::geo get_max_lon() {
        return max_lon;
    }
    double get_min_x() {
        return min_x;
    }
    double get_min_y() {
        return min_y;
    }
    double get_max_x() {
        return max_x;
    }
    double get_max_y() {
        return max_y;
    }
    double get_grid_cell_size() {
        return grid_cell_size;
    }
    int get_row(double y) {
        return (int)((y-min_y)/grid_cell_size);
    }
    int get_col(double x) {
        return (int)((x-min_x)/grid_cell_size);
    }
    int get_row(phil::geo lat) {
        double y = Geo_Utils::get_y(lat);
        return (int)((y-min_y)/grid_cell_size);
    }
    int get_col(phil::geo lon) {
        double x = Geo_Utils::get_x(lon);
        return (int)((x-min_x)/grid_cell_size);
    }
    int getGlobalRow(int row) {
        return row + global_row_min;    // ???
    }
    int getGlobalCol(int col) {
        return col + global_col_min;    // ???
    }

    int get_global_row_min() {
        return global_row_min;                // global row coord of S row
    }
    int get_global_col_min() {
        return global_col_min;                // global col coord of W col
    }
    int get_global_row_max() {
        return global_row_max;                // global row coord of N row
    }
    int get_global_col_max() {
        return global_col_max;                // global col coord of E col
    }

  protected:

    int rows;                    // number of rows
    int cols;                    // number of columns
    double grid_cell_size;            // km per side
    phil::geo min_lat;                // lat of SW corner
    phil::geo min_lon;                // lon of SW corner
    phil::geo max_lat;                // lat of NE corner
    phil::geo max_lon;                // lon of NE corner
    double min_x;                    // global x of SW corner
    double min_y;                    // global y of SW corner
    double max_x;                    // global x of NE corner
    double max_y;                    // global y of NE corner
    int global_row_min;                // global row coord of S row
    int global_col_min;                // global col coord of W col
    int global_row_max;                // global row coord of N row
    int global_col_max;                // global col coord of E col
};

#endif

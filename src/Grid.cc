//
//
// File: Grid.cc
//

#include <utility>
#include <list>
#include <string>
using namespace std;

#include "Global.h"
#include "Geo_Utils.h"
#include "Grid.h"
#include "Cell.h"
#include "Place_List.h"
#include "Place.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"
#include "Household.h"
#include "Population.h"
#include "Date.h"
#include "Large_Grid.h"

Grid::Grid(Large_Grid * lgrid) {
    large_grid = lgrid;
    int large_grid_rows = large_grid->get_rows();
    int large_grid_cols = large_grid->get_cols();
    int large_grid_cell_size = large_grid->get_grid_cell_size();
    min_lat = large_grid->get_min_lat();
    min_lon = large_grid->get_min_lon();
    max_lat = large_grid->get_max_lat();
    max_lon = large_grid->get_max_lon();
    min_x = large_grid->get_min_x();
    min_y = large_grid->get_min_y();
    max_x = large_grid->get_max_x();
    max_y = large_grid->get_max_y();

    get_parameters();

    // find the multiple to use in defining this grid;
    // the multiple must be an integer
    int mult = large_grid_cell_size / grid_cell_size;
    assert(mult == (1.0*large_grid_cell_size / (1.0* grid_cell_size)));

    rows = large_grid_rows * mult;
    cols = large_grid_cols * mult;

    if (Global::Verbose > 0) {
        fprintf(Global::Statusfp, "Grid min_lon = %f\n", min_lon);
        fprintf(Global::Statusfp, "Grid min_lat = %f\n", min_lat);
        fprintf(Global::Statusfp, "Grid max_lon = %f\n", max_lon);
        fprintf(Global::Statusfp, "Grid max_lat = %f\n", max_lat);
        fprintf(Global::Statusfp, "Grid rows = %d  cols = %d\n",rows,cols);
        fprintf(Global::Statusfp, "Grid min_x = %f  min_y = %f\n",min_x,min_y);
        fprintf(Global::Statusfp, "Grid max_x = %f  max_y = %f\n",max_x,max_y);
        fflush(Global::Statusfp);
    }

    grid = new Cell * [rows];
    for (int i = 0; i < rows; i++) {
        grid[i] = new Cell[cols];
        for (int j = 0; j < cols; j++) {
            grid[i][j].setup(this,i,j);
        }
    }

}

void Grid::setup(Place::Allocator< Neighborhood > & neighborhood_allocator) {
    // fill grid with cells; create one neighborhood per cell
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (grid[i][j].get_houses() > 0)
                grid[i][j].make_neighborhood(neighborhood_allocator);
        }
    }

    if (Global::Verbose > 1) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                printf("print grid[%d][%d]:\n",i,j);
                grid[i][j].print();
            }
        }
    }
    vacant_houses.clear();
}

void Grid::get_parameters() {
    Params::get_param_from_string("grid_cell_size", &grid_cell_size);
}

Cell * Grid::get_grid_cell(int row, int col) {
    if (row >= 0 && col >= 0 && row < rows && col < cols)
        return &grid[row][col];
    else
        return NULL;
}

Cell * Grid::get_grid_cell(phil::geo lat, phil::geo lon) {
    int row = get_row(lat);
    int col = get_col(lon);
    return get_grid_cell(row,col);
}


Cell * Grid::select_random_grid_cell(double x0, double y0, double dist) {
    // select a random cell within given distance.
    // if no luck after 20 attempts, return NULL
    for (int i = 0; i < 20; i++) {
        double r = RANDOM()*dist;      // random distance
        double ang = Geo_Utils::DEG_TO_RAD * URAND(0,360); // random angle
        double x = x0 + r*cos(ang);      // corresponding x coord
        double y = y0 + r*sin(ang);      // corresponding y coord
        int row = get_row(y);
        int col = get_col(x);
        Cell * cell = get_grid_cell(row,col);
        if (cell != NULL) return cell;
    }
    return NULL;
}


Cell * Grid::select_random_grid_cell() {
    int row = IRAND(0, rows-1);
    int col = IRAND(0, cols-1);
    return &grid[row][col];
}


Cell * Grid::select_random_neighbor(int row, int col) {
    int n = IRAND_0_7();
    if (n>3) n++;        // excludes local grid_cell
    int r = row-1 + (n/3);
    int c = col-1 + (n%3);
    return get_grid_cell(r,c);
}


void Grid::quality_control(char * directory) {
    if (Global::Verbose) {
        fprintf(Global::Statusfp, "grid quality control check\n");
        fflush(Global::Statusfp);
    }

    int popsize = 0;
    int tot_occ_cells = 0;
    for (int row = 0; row < rows; row++) {
        int min_occ_col = cols+1;
        int max_occ_col = -1;
        for (int col = 0; col < cols; col++) {
            grid[row][col].quality_control();
            int cell_pop = grid[row][col].get_target_popsize();
            if (cell_pop > 0) {
                if (col > max_occ_col) max_occ_col = col;
                if (col < min_occ_col) min_occ_col = col;
                popsize += cell_pop;
            }
        }
        if (min_occ_col < cols) {
            int cells_occ = max_occ_col - min_occ_col + 1;
            tot_occ_cells += cells_occ;
        }
    }

    if (Global::Verbose>1) {
        char filename [PHIL_STRING_SIZE];
        sprintf(filename, "%s/grid.dat", directory);
        FILE *fp = fopen(filename, "w");
        for (int row = 0; row < rows; row++) {
            if (row%2) {
                for (int col = cols-1; col >= 0; col--) {
                    double x = grid[row][col].get_center_x();
                    double y = grid[row][col].get_center_y();
                    fprintf(fp, "%f %f\n",x,y);
                }
            } else {
                for (int col = 0; col < cols; col++) {
                    double x = grid[row][col].get_center_x();
                    double y = grid[row][col].get_center_y();
                    fprintf(fp, "%f %f\n",x,y);
                }
            }
        }
        fclose(fp);
    }

    if (Global::Verbose) {
        int total_area = rows*cols;
        int convex_area = tot_occ_cells;
        fprintf(Global::Statusfp, "Density: popsize = %d total region = %d total_density = %f\n",
                popsize, total_area, (total_area>0) ? (double) popsize / (double) total_area : 0.0);
        fprintf(Global::Statusfp, "Density: popsize = %d convex region = %d convex_density = %f\n",
                popsize, convex_area, (convex_area>0) ? (double) popsize / (double) convex_area : 0.0);
        fprintf(Global::Statusfp, "grid quality control finished\n");
        fflush(Global::Statusfp);
    }
}

void Grid::quality_control(char * directory, double min_x, double min_y) {
    if (Global::Verbose) {
        fprintf(Global::Statusfp, "grid quality control check\n");
        fflush(Global::Statusfp);
    }

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            grid[row][col].quality_control();
        }
    }

    if (Global::Verbose>1) {
        char filename [PHIL_STRING_SIZE];
        sprintf(filename, "%s/grid.dat", directory);
        FILE *fp = fopen(filename, "w");
        for (int row = 0; row < rows; row++) {
            if (row%2) {
                for (int col = cols-1; col >= 0; col--) {
                    double x = grid[row][col].get_center_x();
                    double y = grid[row][col].get_center_y();
                    fprintf(fp, "%f %f\n",x,y);
                }
            } else {
                for (int col = 0; col < cols; col++) {
                    double x = grid[row][col].get_center_x();
                    double y = grid[row][col].get_center_y();
                    fprintf(fp, "%f %f\n",x,y);
                }
            }
        }
        fclose(fp);
    }

    if (Global::Verbose) {
        fprintf(Global::Statusfp, "grid quality control finished\n");
        fflush(Global::Statusfp);
    }
}


// Specific to Cell Grid:

int Grid::get_number_of_neighborhoods() {
    int n = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            if (grid[row][col].get_houses() > 0)
                n++;
        }
    }
    return n;
}

void Grid::record_favorite_places() {
    #pragma omp parallel
    {
        int partial_target_popsize = 0;
        int partial_target_households = 0;
        #pragma omp for
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                Cell * cell = (Cell *) &grid[row][col];
                if (cell->get_houses() > 0) {
                    cell->record_favorite_places();
                    partial_target_popsize += cell->get_neighborhood()->get_size();
                    partial_target_households += cell->get_houses();
                }
            }
        }
        #pragma omp atomic
        target_popsize += partial_target_popsize;
        #pragma omp atomic
        target_households += partial_target_households;
    }
}

vector < Place * >  Grid::get_households_by_distance(phil::geo lat, phil::geo lon, double radius_in_km) {
    double px = Geo_Utils::get_x(lon);
    double py = Geo_Utils::get_y(lat);
    //  get cells around the point, make sure their rows & cols are in bounds
    int r1 = rows-1 - (int)((py + radius_in_km) / grid_cell_size);
    r1 = (r1 >= 0) ? r1 : 0;
    int r2 = rows-1 - (int)((py - radius_in_km) / grid_cell_size);
    r2 = (r2 <= rows-1) ? r2 : rows-1;

    int c1 = (int)((px - radius_in_km) / grid_cell_size);
    c1 = (c1 >= 0) ? c1 : 0;
    int c2 = (int)((px + radius_in_km) / grid_cell_size);
    c2 = (c2 <= cols-1) ? c2 : cols-1;

    //printf("DEBUG: r1 %d r2 %d c1 %d c2 %d\n", r1,r2,c1,c2);
    vector <Place *> households; // store all households in cells ovelapping the radius

    for (int r = r1; r <= r2; r++) {
        for (int c = c1; c <= c2; c++) {
            Cell * p = get_grid_cell(r,c);
            vector <Place *> h = p->get_households();
            for (vector <Place *>::iterator hi = h.begin(); hi != h.end(); hi++) {
                phil::geo hlat = (*hi)->get_latitude();
                phil::geo hlon = (*hi)->get_longitude();
                //printf("DEBUG: household_latitude %f, household_longitude %f\n",hlat,hlon);
                double hx = Geo_Utils::get_x(hlon);
                double hy = Geo_Utils::get_y(hlat);
                if (sqrt((px-hx)*(px-hx)+(py-hy)*(py-hy)) <= radius_in_km) {
                    households.push_back((*hi));
                }
            }
        }
    }
    return households;
}


void Grid::add_vacant_house(Place * house) {
    vacant_houses.push_back(house);
}

Place * Grid::get_vacant_house() {
    Place * house = NULL;
    int count = vacant_houses.size();
    if (count > 0) {
        // pick a random vacant house
        int i = IRAND(0, count-1);
        house = vacant_houses[i];
        vacant_houses[i] = vacant_houses.back();
        vacant_houses.pop_back();
    }
    return house;
}

void Grid::population_migration(int day) {
    select_emigrants(day);
    select_immigrants(day);
}


void Grid::select_emigrants(int day) {
    Person * resident[100];
    if (Global::Verbose) {
        printf("EMIG oldpopsize = %d\n", Global::Pop.get_pop_size());
    }
    double epct;
    // monthly prob of emigrating
    epct = 0.02/12.0;
    int houses_to_emigrate;
    houses_to_emigrate = epct * target_households;
    printf("houses_to_emigrate = %d\n", houses_to_emigrate);
    fflush(stdout);
    int houses_vacated = 0;
    int people_removed = 0;
    for (int h = 0; h < houses_to_emigrate; h++) {

        Person * per = Global::Pop.select_random_person();
        if (RANDOM() < 0.5) {
            // pick a random person between 50 and 60
            int age = per->get_age();
            while (age < 50 || age > 60) {
                per = Global::Pop.select_random_person();
                age = per->get_age();
            }
        }
        Household * house_to_vacate = (Household *) per->get_household();

        // vacate the house
        int size = house_to_vacate->get_size();
        printf("house_to_vacate = %d  size = %d\n", house_to_vacate->get_id(), size);
        fflush(stdout);

        // get a list of all housemates:
        for (int i = 0; i < size; i++) {
            resident[i] = house_to_vacate->get_housemate(i);
        }

        for (int i = 0; i < size; i++) {
            Person * emigrant = resident[i];
            // unenroll for all favorite places, including the house
            printf("person_to_emigrate = %d  age = %d\n", emigrant->get_id(), emigrant->get_age());
            fflush(stdout);
            printf("deleting from population\n");
            fflush(stdout);
            // remove from population
            Global::Pop.delete_person(emigrant);
            printf("deleted from population\n");
            fflush(stdout);
            people_removed++;
        }
        houses_vacated++;
    }
    if (Global::Verbose) {
        printf("newpopsize = %d  people_removed = %d houses_vacated = %d\n",
               Global::Pop.get_pop_size(), people_removed, houses_vacated);
        fflush(stdout);
    }
}

void Grid::select_immigrants(int day) {
    // NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE //
    //                                                                       //
    // Last revision that included this (commented out) code was 1.36        //
    //                                                                       //
    // NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE //
}

void Grid::print_household_distribution(char * dir, char * date_string, int run) {
    FILE *fp;
    int targ, count;
    double pct;
    char filename[PHIL_STRING_SIZE];
    sprintf(filename, "%s/household_dist_%s.%02d", dir, date_string, run);
    printf("print_household_dist entered, filename = %s\n", filename);
    fflush(stdout);

    fp = fopen(filename, "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Cell * cell = (Cell *) &grid[i][j];
            count = cell->get_occupied_houses();
            targ = cell->get_target_households();
            if (targ > 0) pct = (100.0*count)/targ;
            else pct = 0.0;
            fprintf(fp, "Cell %d %d target %d actual %d pct %f\n", i, j, targ, count, pct);
        }
    }
    fclose(fp);
}

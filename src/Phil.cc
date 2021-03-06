//
//
// File: Phil.cc
//

#include "Phil.h"
#include "Utils.h"
#include "Global.h"
#include "Population.h"
#include "Place_List.h"
#include "Grid.h"
#include "Large_Grid.h"
#include "Small_Grid.h"
#include "Params.h"
#include "Random.h"
#include "Vaccine_Manager.h"
#include "Date.h"
#include "Evolution.h"
#include "Travel.h"
#include "Epidemic.h"
#include "Seasonality.h"
#include "Past_Infection.h"
#include "Activities.h"
#include "Tracker.h"
#include "Report.h"
#include "json.h"

using nlohmann::json;

#include "execinfo.h"
#include <csignal>
#include <cstdlib>
#include <cxxabi.h>

int main(int argc, char* argv[]) {

    int run;          // number of current run
    unsigned long new_seed;
    char directory[PHIL_STRING_SIZE];
    char paramfile[PHIL_STRING_SIZE];

    Global::Statusfp = stdout;
    Utils::phil_start_timer();
    Utils::phil_print_wall_time("PHIL started");

    // read optional param file name from command line
    if (argc > 1) {
        strcpy(paramfile, argv[1]);
    } else {
        strcpy(paramfile, "params");
    }
    fprintf(Global::Statusfp, "param file = %s\n", paramfile);
    fflush(Global::Statusfp);

    // read optional run number from command line (must be 2nd arg)
    if (argc > 2) {
        sscanf(argv[2], "%d", &run);
    } else {
        run = 1;
    }

    // read optional working directory from command line (must be 3rd arg)
    if (argc > 3) {
        strcpy(directory, argv[3]);
    } else {
        strcpy(directory, "");
    }

    // get runtime parameters
    Params::read_parameters(paramfile);
    Global::get_global_parameters();

    // get runtime population parameters
    Global::Pop.get_parameters();

    // initialize masks in Global::Pop
    Global::Pop.initialize_masks();

    if (strcmp(directory, "") == 0) {
        // use the directory in the params file
        strcpy(directory, Global::Output_directory);
        //printf("directory = %s\n",directory);
    } else {
        // change the Output_directory
        strcpy(Global::Output_directory, directory);
        PHIL_STATUS(0, "Overridden from command line: Output_directory = %s\n",
                    Global::Output_directory);
    }

    // create the output directory, if necessary
    Utils::phil_make_directory(directory);

    // open output files with global file pointers
    Utils::phil_open_output_files(directory, run);

    // initialize RNG
    INIT_RANDOM(Global::Seed);

    // Date Setup
    // Start_date parameter must have format 'YYYY-MM-DD'
    Global::Sim_Start_Date = new Date(string(Global::Start_date));
    Global::Sim_Current_Date = new Date(string(Global::Start_date));

    if (Global::Rotate_start_date) {
        // add one day to the start date for each additional run,
        // rotating the days of the week to reduce weekend effect.
        Global::Sim_Start_Date->advance((run-1)%7);
        Global::Sim_Current_Date->advance((run-1)%7);
    }

    // set random number seed based on run number
    if (run > 1 && Global::Reseed_day == -1) {
        new_seed = Global::Seed * 100 + (run-1);
    } else {
        new_seed = Global::Seed;
    }
    fprintf(Global::Statusfp, "seed = %lu\n", new_seed);
    INIT_RANDOM(new_seed);

    Utils::phil_print_wall_time("\nPHIL run %d started", (int) run);

    // initializations

    // Initializes Synthetic Population parameters, determines the synthetic
    // population id if the city or county was specified as a parameter
    // Must be called BEFORE Pop.split_synthetic_populations_by_deme() because
    // city/county population lookup may overwrite Global::Synthetic_population_id
    Global::Places.get_parameters();

    // split the population id parameter string ( that was initialized in
    // Places::get_parameters ) on whitespace; each population id is processed as a
    // separate deme, and stored in the Population object.
    Global::Pop.split_synthetic_populations_by_deme();

    // Loop over all Demes and read in the household, schools and workplaces
    // and setup grids and cells
    Global::Places.read_all_places(Global::Pop.get_demes());
    Utils::phil_print_lap_time("Places.read_places");

    // initialize activities
    Activities::read_init_files();

    // read in the population and have each person enroll
    // in each favorite place identified in the population file
    Global::Pop.setup();
    Global::Places.setup_households();
    Utils::phil_print_lap_time("Pop.setup");

    if (Global::Report_Epidemic_Data_By_Census_Block) {
        Global::Block_Epi_Day_Tracker = new Tracker<string>("Census Day Block String Tracker","BlockGroup");
        Global::Pop.initialize_disease_state_counts_by_block();
    }
    // define PHIL-specific places and have each person enroll as needed

    // classrooms
    Global::Places.setup_classrooms();
    Global::Pop.assign_classrooms();
    Utils::phil_print_lap_time("assign classrooms");

    // teachers
    if (Global::Assign_Teachers) {
        Global::Places.assign_teachers();
        Utils::phil_print_lap_time("assign teachers");
    }

    // offices
    Global::Places.setup_offices();
    Global::Pop.assign_offices();
    Utils::phil_print_lap_time("assign offices");

    // after all enrollments, prepare to receive visitors
    Global::Places.prepare();

    // record the favorite places for households within each grid cell
    Global::Cells->record_favorite_places();
    Utils::phil_print_lap_time("place prep");

    if (Global::Enable_Travel) {
        Global::Large_Cells->set_population_size();
        Travel::setup(directory);
        Utils::phil_print_lap_time("Travel setup");
    }

    if (Global::Quality_control) {
        Global::Pop.quality_control();
        Global::Places.quality_control(directory);
        Global::Large_Cells->quality_control(directory);
        Global::Cells->quality_control(directory);
        if (Global::Enable_Small_Grid) {
            Global::Small_Cells->quality_control(directory);
        }
        if (Global::Track_network_stats) {
            Global::Pop.get_network_stats(directory);
        }
        Utils::phil_print_lap_time("quality control");
    }

    if (Global::Track_age_distribution) {
        Global::Pop.print_age_distribution(directory,
                                           (char *) Global::Sim_Start_Date->get_YYYYMMDD().c_str(), run);
    }

    if (Global::Report_Epidemic_Data_By_Census_Block) {
        //Global::Block_Tracker = new Tracker<string>("Census String Block Tracker Test","BlockGroup");
        //Global::Pop.report_mean_hh_stats_per_census_block();
        //Global::Pop.report_mean_hh_stats_per_census_block();
    }

    if (Global::Enable_Seasonality) {
        Global::Clim->print_summary();
    }

    for (int d = 0; d < Global::Diseases; ++d) {
        Disease * disease = Global::Pop.get_disease(d);
        disease->initialize_evolution_reporting_grid(Global::Large_Cells);
        disease->init_prior_immunity();
    }

    // initialize GAIA data if desired
    if (Global::Print_GAIA_Data && run == 1) {
        Global::Small_Cells->initialize_gaia_data(directory, run);
    }

    Utils::phil_print_lap_time("PHIL initialization");
    Utils::phil_print_wall_time("PHIL initialization complete");

    Global::Rpt.setup();

    json j;

    j["event"] = "parameters";

    for (int i = 0; i < Params::param_count; i++) {
        j[Params::param_name[i]] = Params::param_value[i];
    }

    Global::Rpt.append(j);

    time_t simulation_start_time;
    Utils::phil_start_timer(&simulation_start_time);

    for (int day = 0; day < Global::Days; day++) {

        Utils::phil_start_day_timer();
        if (day == Global::Reseed_day) {
            fprintf(Global::Statusfp, "************** reseed day = %d\n", day);
            fflush(Global::Statusfp);
            INIT_RANDOM(new_seed + run - 1);
        }

        if (Date::match_pattern(Global::Sim_Current_Date, "01-01-*")) {
            if (Global::Track_age_distribution) {
                Global::Pop.print_age_distribution(directory,
                                                   (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str(), run);
                Global::Places.print_household_size_distribution(directory,
                        (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str(), run);
            }
            if (Global::Track_household_distribution) {
                Global::Cells->print_household_distribution(directory,
                        (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str(), run);
            }
        }

        Global::Places.update(day);
        Utils::phil_print_lap_time("day %d update places", day);

        Global::Pop.update(day);
        Utils::phil_print_lap_time("day %d update population", day);

        Epidemic::update(day);
        Utils::phil_print_lap_time("day %d update epidemics", day);

        Global::Pop.report(day);
        Utils::phil_print_lap_time("day %d report population", day);

        // If Block Output is desired, update this
        if (Global::Report_Epidemic_Data_By_Census_Block) {
            Global::Block_Epi_Day_Tracker->set_all_index_for_key("Day",day);
            bool printHeader = false;
            if (day == 0) printHeader=true;
            Global::Block_Epi_Day_Tracker->output_csv_report_format(Global::BlockDayfp,printHeader);
            Global::Block_Epi_Day_Tracker->set_all_index_for_key("C",int(0));
            Global::Block_Epi_Day_Tracker->set_all_index_for_key("Cs",int(0));
            Global::Block_Epi_Day_Tracker->set_all_index_for_key("V",int(0));
            Global::Block_Epi_Day_Tracker->set_all_index_for_key("Av",int(0));
        }

        // print GAIA data if desired
        if (Global::Print_GAIA_Data && run == 1) {
            Global::Small_Cells->print_gaia_data(directory, run, day);
            Utils::phil_print_lap_time("day %d print_gaia_data", day);
        }

        if (Global::Enable_Migration
                && Date::match_pattern(Global::Sim_Current_Date, "02-*-*")) {
            Global::Cells->population_migration(day);
        }

        if (Global::Enable_Aging && Global::Verbose
                && Date::match_pattern(Global::Sim_Current_Date, "12-31-*")) {
            Global::Pop.quality_control();
        }

        if (Date::match_pattern(Global::Sim_Current_Date, "01-01-*")) {
            if (Global::Track_age_distribution) {
                Global::Pop.print_age_distribution(directory,
                                                   (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str(), run);
                Global::Places.print_household_size_distribution(directory,
                        (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str(), run);
            }
            if (Global::Track_household_distribution) {
                Global::Cells->print_household_distribution(directory,
                        (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str(), run);
            }
        }

        // incremental trace
        if (Global::Incremental_Trace && day && !(day%Global::Incremental_Trace))
            Global::Pop.print(1, day);

        if (Global::Track_vaccine_infection_events) {
            if (Global::Enable_Vaccination) {
                Global::Pop.report_vaccine_infection_events(day);
            }
        }

        #pragma omp parallel sections
        {
            #pragma omp section
            {
                // this refreshes all RNG buffers in a new thread team
                RNG::refresh_all_buffers();
            }
            #pragma omp section
            {
                // flush infections file buffer
                fflush(Global::Infectionfp);
            }
        }

        Utils::phil_print_wall_time("day %d finished", day);

        Utils::phil_print_day_timer(day);
        Utils::phil_print_resource_usage(day);

        Global::Sim_Current_Date->advance();

        Global::Rpt.print();
        Global::Rpt.clear();
    }

    fflush(Global::Infectionfp);

    Utils::phil_print_lap_time(&simulation_start_time,
                               "\nPHIL simulation complete. Excluding initialization, %d days",
                               Global::Days);

    Utils::phil_print_wall_time("PHIL finished");
    Utils::phil_print_finish_timer();

    // finish up
    Global::Pop.end_of_run();
    Global::Places.end_of_run();

    // close all open output files with global file pointers
    Utils::phil_end();

    return 0;
}

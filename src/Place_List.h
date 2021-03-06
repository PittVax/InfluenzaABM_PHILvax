//
//
// File: Place_List.h
//

#ifndef _PHIL_PLACE_LIST_H
#define _PHIL_PLACE_LIST_H

#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <iostream>
#include <fstream>

#include "Global.h"
#include "Place.h"
#include "Utils.h"

using namespace std;

class School;
class Neighborhood;
class Household;
class Office;
class Hospital;
class Classroom;
class Workplace;

#include <tr1/unordered_map>

// Helper class used during read_all_places/read_places; definition
// after Place_List class
class Place_Init_Data;

class Place_List {

    typedef std::set< Place_Init_Data > InitSetT;
    typedef std::tr1::unordered_map< string, int > LabelMapT;
    typedef std::pair< InitSetT::iterator, bool > SetInsertResultT;
    typedef std::map< char, int > TypeCountsMapT;
    typedef std::map< char, string > TypeNameMapT;

  public:

    Place_List() {
        place_label_map = new LabelMapT();
        places.clear();
        workplaces.clear();
        next_place_id = 0;
        init_place_type_name_lookup_map();
    }

    ~Place_List();

    void read_all_places(const std::vector< Utils::Tokens > & Demes);
    void read_places(const char * pop_dir, const char * pop_id,
                     unsigned char deme_id, InitSetT & pids);

    void prepare();
    void update(int day);
    void quality_control(char * directory);
    void get_parameters();
    Place * get_place_from_label(char *s) const;
    Place * get_place_at_position(int i) {
        return places[i];
    }


    // TODO have to assign id consistently.  don't set id in place constructor, instead
    // initialize it with -1, then set the id in add_place.

    int get_number_of_places() {
        return places.size();
    }

    int get_number_of_places(char place_type);

    int get_new_place_id() {
        int id = next_place_id;
        ++(next_place_id);
        return id;
    }

    void setup_households();
    void setup_classrooms();
    void assign_teachers();
    void setup_offices();
    Place * get_random_workplace();
    void print_household_size_distribution(char * dir, char * date_string, int run);
    void end_of_run();

    int get_number_of_demes() {
        return number_of_demes;
    }

    void get_cell_data(int disease_id, char place_type, int output_code);
    void get_cell_data_from_households(int disease_id, int output_code) {
        get_cell_data(disease_id, HOUSEHOLD, output_code);
    }
    //void get_cell_data_from_schools(int disease_id, int output_code);
    //void get_cell_data_from_workplaces(int disease_id, int output_code);
    //void get_cell_data_from_neighborhoods(int disease_id, int output_code);

  private:

    void read_household_file(unsigned char deme_id, char * location_file,
                             InitSetT & pids);

    void read_workplace_file(unsigned char deme_id, char * location_file,
                             InitSetT & pids);

    void read_school_file(unsigned char deme_id, char * location_file,
                          InitSetT & pids);

    void read_group_quarters_file(unsigned char deme_id, char * location_file,
                                  InitSetT & pids);

    int number_of_demes;

    void set_number_of_demes(int n) {
        number_of_demes = n;
    }

    phil::geo min_lat, max_lat, min_lon, max_lon;

    void delete_place_label_map();

    void parse_lines_from_stream(std::istream & stream,
                                 std::vector< Place_Init_Data > & pids);

    TypeNameMapT place_type_name_lookup_map;

    void init_place_type_name_lookup_map() {
        place_type_name_lookup_map[ NEIGHBORHOOD ]  = "NEIGHBORHOOD";
        place_type_name_lookup_map[ HOUSEHOLD ]     = "HOUSEHOLD";
        place_type_name_lookup_map[ SCHOOL ]        = "SCHOOL";
        place_type_name_lookup_map[ CLASSROOM ]     = "CLASSROOM";
        place_type_name_lookup_map[ WORKPLACE ]     = "WORKPLACE";
        place_type_name_lookup_map[ OFFICE ]        = "OFFICE";
        place_type_name_lookup_map[ HOSPITAL ]      = "HOSPITAL";
        place_type_name_lookup_map[ COMMUNITY ]     = "COMMUNITY";
    }

    string lookup_place_type_name(char place_type) {
        assert(place_type_name_lookup_map.find(place_type) !=
               place_type_name_lookup_map.end());
        return place_type_name_lookup_map[ place_type ];
    }

    bool add_place(Place * p);

    template< typename Place_Type >
    void add_preallocated_places(char place_type, Place::Allocator< Place_Type > & pal) {
        // make sure that the expected number of places were allocated
        assert(pal.get_number_of_contiguous_blocks_allocated() == 1);
        assert(pal.get_number_of_remaining_allocations() == 0);

        int places_added = 0;
        Place_Type * place = pal.get_base_pointer();
        int places_allocated = pal.size();

        for (int i = 0; i < places_allocated; ++i) {
            if (add_place(place)) {
                ++(places_added);
            }
            ++(place);
        }
        PHIL_STATUS(0, "Added %7d %16s places to Place_List\n",
                    places_added, lookup_place_type_name(place_type).c_str());
        PHIL_CONDITIONAL_WARNING(places_added != places_allocated,
                                 "%7d %16s places were added to the Place_List, but %7d were allocated\n",
                                 places_added, lookup_place_type_name(place_type).c_str(),
                                 places_allocated);
        // update/set place_type_counts for this place_type
        place_type_counts[ place_type ] = places_added;
    }

    // map to hold counts for each place type
    TypeCountsMapT place_type_counts;

    int next_place_id;

    char locfile[80];

    vector <Place *> places;
    vector <Place *> workplaces;

    LabelMapT * place_label_map;

};

struct Place_Init_Data {

    char s[ 200 ];
    char place_type;
    int income;
    unsigned char deme_id;
    phil::geo lat, lon;
    bool is_group_quarters;
    string census_block;

    void setup(char _s[], char _place_type, const char * _lat, const char * _lon,
               unsigned char _deme_id, string _census_block, const char * _income,
               bool _is_group_quarters) {
        place_type = _place_type;
        strcpy(s, _s);
        sscanf(_lat, "%f", &lat);
        sscanf(_lon, "%f", &lon);
        sscanf(_income, "%d", & income);
        census_block = _census_block;

        if (!(lat >= -90 && lat <= 90) || !(lon >= -180 && lon <= 180)) {
            printf("BAD LAT-LON: type = %c lat = %f  lon = %f  inc = %d  s = %s\n", place_type, lat, lon, income, s);
            lat = 34.999999;
        }
        assert(lat >= -90 && lat <= 90);
        assert(lon >= -180 && lon <= 180);

        is_group_quarters = _is_group_quarters;
    };


    Place_Init_Data(char _s[], char _place_type, const char * _lat, const char * _lon,
                    unsigned char _deme_id, string _census_block = "",
                    const char * _income = "0", bool _is_group_quarters = false) {
        setup(_s, _place_type, _lat, _lon, _deme_id, _census_block, _income, _is_group_quarters);
    }

    bool operator< (const Place_Init_Data & other) const {

        if (place_type != other.place_type) {
            return place_type < other.place_type;
        } else if (strcmp(s, other.s) < 0) {
            return true;
        } else {
            return false;
        }
    }

    const std::string to_string() const {
        std::stringstream ss;
        ss << "Place Init Data ";
        ss << place_type << " ";
        ss << lat << " ";
        ss << lon << " ";
        ss << census_block << " ";
        ss << s << " ";
        ss << int(deme_id) << std::endl;
        return ss.str();
    }

};

#endif // _PHIL_PLACE_LIST_H

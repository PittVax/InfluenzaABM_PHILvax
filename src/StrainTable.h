//
//
// File: StrainTable.h
//

#ifndef _PHIL_StrainTable_H
#define _PHIL_StrainTable_H

#include <vector>
#include <map>
#include <fstream>

#include "Global.h"
#include "Strain.h"

class Strain;
class Disease;

using namespace std;

class Disease;
class Strain;

class StrainTable {
  public:
    StrainTable();
    ~StrainTable();

    void setup(Disease * d);   // Initial strains
    void reset();

    void add_root_strain(int num_elements);

    void add(Strain * s);

    int add(Strain * child_strain, double transmissibility);
    int add(Strain * child_strain, double transmissibility, int parent_strain_id);

    double get_transmissibility(int id);

    int get_num_strains() {
        phil::Spin_Lock lock(mutex);
        return strains.size();
    }

    int get_num_strain_data_elements(int strain);
    int get_strain_data_element(int strain, int i);
    const Strain_Data & get_strain_data(int strain);

    const Strain & get_strain(int strain_id) {
        return * strains[ strain_id ];
    }

    void printStrain(int strain_id, stringstream &out);
    std::string get_strain_data_string(int strain_id);

  private:
    phil::Spin_Mutex mutex;
    Disease * disease;
    std::vector < Strain * > strains;
    std::map< std::string, int > strain_genotype_map;
};

#endif // _PHIL_StrainTable_H

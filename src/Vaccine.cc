//
//
// File: Vaccine.cc
//
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

#include "Vaccine.h"
#include "Vaccine_Dose.h"
#include "Timestep_Map.h"

Vaccine::Vaccine(string _name, int _id, int _disease,
                 int _total_avail, int _additional_per_day,
                 int _start_day, int _num_strains, int *_strains,
                 Timestep_Map * _production) {
    name =               _name;
    id =                 _id;
    disease =             _disease;
    additional_per_day = _additional_per_day;
    start_day =          _start_day;
    num_strains =        _num_strains;
    strains =            new int[_num_strains];

    for (int i=0; i<_num_strains; ++i) {
        strains[i] = _strains[i];
    }

    production =         _production;

    initial_stock = 0;
    stock = 0;
    reserve = _total_avail;
    total_avail = _total_avail;
    number_delivered = 0;
    number_effective = 0;

    //production->print();
}
//Params::get_indexed_param("vaccine_additional_per_day",iv,&apd);
//Params::get_indexed_param("vaccine_starting_day",iv,&std);

Vaccine::~Vaccine() {
    for (unsigned int i = 0; i < doses.size(); i++) delete doses[i];
    delete strains;
}

void Vaccine::add_dose(Vaccine_Dose* _vaccine_dose) {
    doses.push_back(_vaccine_dose);
}

void Vaccine::print() const {
    cout << "Name = \t\t\t\t" <<name << "\n";
    cout << "Applied to disease \t\t" << disease << "\n";
    cout << "Initial Stock = \t\t" << initial_stock << "\n";
    cout << "Total Available = \t\t"<< total_avail << "\n";
    cout << "Amount left to system = \t" << reserve << "\n";
    if (production != NULL)
        production->print();
    else {
        cout << "Additional Stock per day =\t" << additional_per_day << "\n";
        cout << "Starting on day = \t\t" << start_day << "\n";
    }
    cout << "Dose Information\n";
    for (unsigned int i=0; i<doses.size(); i++) {
        cout <<"Dose #"<<i+1 << "\n";
        doses[i]->print();
    }
}

int Vaccine::get_additional_per_day(int day) {
    //printf("Vaccine %s producing %d on day %d\n",name.c_str(),production->get_value_for_timestep(day,Global::Vaccine_offset),day);
    if (production == NULL) {
        if (day >= start_day) {
            return additional_per_day;
        } else {
            return 0.0;
        }
    } else {
        return production->get_value_for_timestep(day,Global::Vaccine_offset); // I have no clue what Vaccine_offset is???
    }
}

void Vaccine::reset() {
    stock = 0;
    reserve = total_avail;
}

void Vaccine::update(int day) {
    add_stock(get_additional_per_day(day));
}


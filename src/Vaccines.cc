//
//
// File: Vaccines.cc
//


#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

#include "Vaccines.h"
#include "Vaccine.h"
#include "Vaccine_Dose.h"
#include "Random.h"
#include "Age_Map.h"
#include "Timestep_Map.h"

void Vaccines::setup(void) {

    int number_vacc;
    Params::get_param_from_string("number_of_vaccines",&number_vacc);

    for (int iv=0; iv<number_vacc; iv++) {
        int ta;
        int apd;
        int std;
        int tbd;
        int num_doses;

        // New Vaccine Production Schedule rather than the even vaccine total per day
        stringstream vacprodD;
        vacprodD << "vaccine_production_file[" << iv << "]";

        Timestep_Map * production;
        if (Params::does_param_exist(vacprodD.str())) {
            stringstream vacprodC;
            vacprodC << "vaccine_production[" << iv << "]";
            production = new Timestep_Map(vacprodC.str());
            production->read_map();
            apd = -1.0;
            std = -1.0;
        } else {
            production = NULL;
            Params::get_indexed_param("vaccine_additional_per_day",iv,&apd);
            Params::get_indexed_param("vaccine_starting_day",iv,&std);
        }

        Params::get_indexed_param("vaccine_number_of_doses",iv,&num_doses);
        Params::get_indexed_param("vaccine_total_avail",iv,&ta);

        int nstrains;
        int *strains;

        if (!Global::Enable_Simple_Strain_Model) {
            Params::get_indexed_param((char *)"vaccine_strains", iv, &nstrains);
            strains = new int[nstrains];
            Params::get_indexed_param_vector<int>("vaccine_strains", iv, strains);
        }
        else {
            char vaccine_simple_strains[128];
            sprintf(vaccine_simple_strains, "vaccine_simple_strains[%d]", iv);
            std::vector<int> strains_vector;
            nstrains = Params::get_param_vector(vaccine_simple_strains, strains_vector);  
            strains = new int[strains_vector.size()];
            cout << "simple_strains\n";
            for (int i=0; i<strains_vector.size(); ++i) {
                strains[i] = strains_vector[i];
                cout << strains[i] << "\n";

            }
        }

        stringstream name;
        name << "Vaccine#"<<iv+1;
        vaccines.push_back(new Vaccine(name.str(),iv,0,ta,apd,std,nstrains,strains,production));

        for (int id=0; id<num_doses; id++) {
            Age_Map* efficacy_map = new Age_Map("Dose Efficacy");
            Age_Map* efficacy_delay_map = new Age_Map("Dose Efficacy Delay");
            Params::get_double_indexed_param("vaccine_next_dosage_day",iv,id,&tbd);
            efficacy_map->read_from_input("vaccine_dose_efficacy",iv,id);
            efficacy_delay_map->read_from_input("vaccine_dose_efficacy_delay",iv,id);
            vaccines[iv]->add_dose(new Vaccine_Dose(efficacy_map,efficacy_delay_map,tbd));
        }
    }
}

void Vaccines::print() const {
    cout <<"Vaccine Package Information\n";
    cout <<"There are "<<vaccines.size() <<" vaccines in the package\n";
    fflush(stdout);
    for (unsigned int i=0; i<vaccines.size(); i++) {
        vaccines[i]->print();
    }
    fflush(stdout);
}

void Vaccines::print_current_stocks() const {
    cout << "Vaccine Stockk Information\n";
    cout << "\nVaccines# " << "Current Stock      " << "Current Reserve    \n";
    for (unsigned int i=0; i<vaccines.size(); i++) {
        cout << setw(10) << i+1 << setw(20) << vaccines[i]->get_current_stock()
             << setw(20) << vaccines[i]->get_current_reserve() << "\n";
    }
}

void Vaccines::update(int day) {
    for (unsigned int i=0; i<vaccines.size(); i++) {
        vaccines[i]->update(day);
    }
}

void Vaccines::reset() {
    for (unsigned int i=0; i<vaccines.size(); i++) {
        vaccines[i]->reset();
    }
}


int Vaccines::pick_from_applicable_vaccines(int age) const {
    vector <int> app_vaccs;
    for (unsigned int i=0; i<vaccines.size(); i++) {
        // if first dose is applicable, add to vector.
        if (vaccines[i]->get_dose(0)->is_within_age(age) &&
                vaccines[i]->get_current_stock() > 0) {
            app_vaccs.push_back(i);
        }
    }

    if (app_vaccs.size() == 0) {
        return -1;
    }

    int randnum = 0;
    if (app_vaccs.size() > 1) {
        randnum = (int)(RANDOM()*app_vaccs.size());
    }
    return app_vaccs[randnum];
}

int Vaccines::get_total_vaccines_avail_today() const {
    int total=0;
    for (unsigned int i=0; i<vaccines.size(); i++) {
        total += vaccines[i]->get_current_stock();
    }
    return total;
}

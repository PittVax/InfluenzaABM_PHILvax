/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Person.h
//

#ifndef _FRED_PERSON_H
#define _FRED_PERSON_H

#include "Global.h"
using namespace std;

class Place;
class Disease;
class Infection;
class Population;
class Transmission;

#include "Demographics.h"
#include "Health.h"
#include "Activities.h"
#include "Vaccine.h"
#include "Report.h"
#include "json.h"

using nlohmann::json;

class Person {
  public:

    Person();
    ~Person();

    void become_unsusceptible(Disease *disease) {
        health.become_unsusceptible(this, disease);
    }

    void become_exposed(Disease *disease, Transmission & transmission) {
        health.become_exposed(this, disease, transmission);
    }

    void become_immune(Disease *disease);
    void print(FILE *fp, int disease);

    int is_new_case(int day, int disease) const {
        return (health.get_exposure_date(disease) == day);
    }

    int addInfected(int disease, vector<int> strains);
    void infect(Person *infectee, int disease, Transmission & transmission);
    void set_changed();
    void update_demographics(int day) {
        demographics.update(day);
    }
    void update_health(int day) {
        health.update(this, day);
    }
    void update_health_interventions(int day) {
        health.update_interventions(this, day);
    }
    void prepare_activities() {
        activities.prepare();
    }
    void update_activity_profile() {
        activities.update_profile(this);
    }
    void update_household_mobility() {
        activities.update_household_mobility(this);
    }
    void become_susceptible(Disease * disease) {
        health.become_susceptible(this, disease);
    }

    void update_household_counts(int day, int disease_id) {
        health.update_place_counts(this, day, disease_id, get_household());
    }

    void update_school_counts(int day, int disease_id) {
        health.update_place_counts(this, day, disease_id, get_school());
    }

    void become_infectious(Disease * disease) {
        health.become_infectious(this, disease);
    }

    void become_symptomatic(Disease *disease) {
        health.become_symptomatic(this, disease);
    }

    void recover(Disease * disease) {
        health.recover(this, disease);
    }

    Person * give_birth(int day);

    void assign_classroom() {
        activities.assign_classroom(this);
    }

    void assign_office() {
        activities.assign_office(this);
    }

    /**
     * Will print out a person in a format similar to that read from population file
     * (with additional run-time values inserted (denoted by *)):<br />
     * (i.e Label *ID* Age Sex Married Occupation Household School *Classroom* Workplace *Office*)
     * @return a string representation of this Person object
     */
    string to_string();

    // access functions:
    int get_id() const {
        return id;
    }

    Demographics * get_demographics() {
        return &demographics;
    }

    int get_age() {
        return demographics.get_age();
    }
    int get_birth_year() {
        return demographics.get_birth_year();
    }
    int get_birth_day_of_year() {
        return demographics.get_birth_day_of_year();
    }
    int get_init_age() const {
        return demographics.get_init_age();
    }
    double get_real_age() const {
        return demographics.get_real_age();
    }
    int get_age_in_days() const {
        return demographics.get_age_in_days();
    }
    char get_sex() const {
        return demographics.get_sex();
    }
    int get_race() {
        return demographics.get_race();
    }
    int get_relationship() {
        return demographics.get_relationship();
    }
    void set_relationship(int rel) {
        demographics.set_relationship(rel);
    }
    bool is_deceased() {
        return demographics.is_deceased();
    }
    bool is_adult() {
        return demographics.get_age() >= Global::ADULT_AGE;
    }
    bool is_child() {
        return demographics.get_age() < Global::ADULT_AGE;
    }
    Health * get_health() {
        return &health;
    }
    int is_symptomatic() {
        return health.is_symptomatic();
    }
    bool is_susceptible(int dis) {
        return health.is_susceptible(dis);
    }

    bool is_infectious(int dis) {
        return health.is_infectious(dis);
    }

    bool is_infected(int dis) {
        return health.is_infected(dis);
    }

    double get_susceptibility(int disease) const {
        return health.get_susceptibility(disease);
    }

    double get_infectivity(int disease, int day) const {
        return health.get_infectivity(disease, day);
    }

    void advance_seed_infection(int disease_id, int days_to_advance) {
        health.advance_seed_infection(disease_id, days_to_advance);
    }

    int get_exposure_date(int disease) const {
        return health.get_exposure_date(disease);
    }

    int get_infectious_date(int disease) const {
        return health.get_infectious_date(disease);
    }

    int get_recovered_date(int disease) const {
        return health.get_recovered_date(disease);
    }

    Person * get_infector(int disease) const {
        return health.get_infector(disease);
    }

    int get_infected_place_id(int disease) const {
        return health.get_infected_place_id(disease);
    }
    Place * get_infected_place(int disease) const {
        return health.get_infected_place(disease);
    }

    char * get_infected_place_label(int disease) const {
        return health.get_infected_place_label(disease);
    }

    char get_infected_place_type(int disease) const {
        return health.get_infected_place_type(disease);
    }

    int get_infectees(int disease) const {
        return health.get_infectees(disease);
    }

    Activities * get_activities() {
        return &activities;
    }

    Place * get_neighborhood() {
        return activities.get_neighborhood();
    }

    Place * get_household() {
        return activities.get_household();
    }

    Place * get_permanent_household() {
        return activities.get_permanent_household();
    }

    unsigned char get_deme_id() {
        return activities.get_deme_id();
    }

    bool is_householder() {
        return demographics.is_householder();
    }

    void make_householder() {
        demographics.make_householder();
    }

    /* void set_household_census_block(string * census_block_){ */
    /*   activities.set_household_census_block(census_block_); */
    /* } */

    /* string get_household_census_block(){ */
    /*   return *(activities.get_household_census_block()); */
    /* } */
    /* string get_census_block(){ */
    //string* get_census_block(){ return (Household*)(activities.get_household())->get_census_block(); }

    /**
     * @return a pointer to this Person's School
     * @see Activities::get_school()
     */
    Place * get_school() {
        return activities.get_school();
    }

    Place * get_classroom() {
        return activities.get_classroom();
    }

    Place * get_workplace() {
        return activities.get_workplace();
    }

    Place * get_office() {
        return activities.get_office();
    }

    int get_degree() {
        return activities.get_degree();
    }

    int get_household_size() {
        return activities.get_group_size(HOUSEHOLD_ACTIVITY);
    }

    int get_neighborhood_size() {
        return activities.get_group_size(NEIGHBORHOOD_ACTIVITY);
    }

    int get_school_size() {
        return activities.get_group_size(SCHOOL_ACTIVITY);
    }

    int get_classroom_size() {
        return activities.get_group_size(CLASSROOM_ACTIVITY);
    }

    int get_workplace_size() {
        return activities.get_group_size(WORKPLACE_ACTIVITY);
    }

    int get_office_size() {
        return activities.get_group_size(OFFICE_ACTIVITY);
    }

    void start_traveling(Person *visited) {
        activities.start_traveling(this, visited);
    }

    void stop_traveling() {
        activities.stop_traveling(this);
    }

    bool get_travel_status() {
        return activities.get_travel_status();
    }

    int get_num_past_infections(int disease) {
        return health.get_num_past_infections(disease);
    }

    Past_Infection *get_past_infection(int disease, int i) {
        return health.get_past_infection(disease, i);
    }

    void clear_past_infections(int disease) {
        health.clear_past_infections(disease);
    }

    //void add_past_infection(int d, Past_Infection *pi){ health.add_past_infection(d, pi); }
    void add_past_infection(int strain_id, int recovery_date, int age_at_exposure, Disease * dis) {
        health.add_past_infection(strain_id, recovery_date, age_at_exposure, dis);
    }

    void take_vaccine(Vaccine *vacc, int day, Vaccine_Manager* vm) {

        assert(refuse_vaccine_until <= day);

        health.take_vaccine(this, vacc, day, vm);

        refuse_vaccine_until = day + 180;

        json j = {
            {"event", "vaccination"},
            {"vaccine_day", day},
            {"person", get_id()},
            {"vaccine", vacc->get_ID()}
        };

        Global::Rpt.append(j);
    }

    bool acceptance_of_vaccine(int day) {
        //return behavior.acceptance_of_vaccine();
        if (day >= refuse_vaccine_until || !Global::Enable_Simple_Strain_Model) {
            return true;
        }
        else {
            return false;
        }
    }
    bool acceptance_of_another_vaccine_dose(int day) {
        ///return behavior.acceptance_of_another_vaccine_dose();
        return acceptance_of_vaccine(day);
    }
    bool child_acceptance_of_vaccine(int day) {
        ///return behavior.child_acceptance_of_vaccine();
        return acceptance_of_vaccine(day);
    }
    bool child_acceptance_of_another_vaccine_dose(int day) {
        ///return behavior.child_acceptance_of_another_vaccine_dose();
        return acceptance_of_vaccine(day);
    }

    bool is_sick_leave_available() {
        return activities.is_sick_leave_available();
    }

    void terminate();

    void die() {
        health.die();
    }

    void set_pop_index(int idx) {
        index = idx;
    }

    int get_pop_index() {
        return index;
    }

    void birthday(int day) {
        demographics.birthday(this, day);
    }

    void update_births(int day) {
        demographics.update_births(this, day);
    }

    void update_deaths(int day) {
        demographics.update_deaths(this, day);
    }

    bool become_a_teacher(Place *school) {
        return activities.become_a_teacher(this, school);
    }
    bool is_teacher() {
        return activities.is_teacher();
    }
    bool is_student() {
        return activities.is_student();
    }

  private:

    // id: Person's unique identifier (never reused)
    int id;
    // index: Person's location in population container; once set, will be unique at any given time,
    // but can be reused over the course of the simulation for different people (after death/removal)
    int index;
    int refuse_vaccine_until;
    Health health;
    Demographics demographics;
    Activities activities;

  protected:

    friend class Population;
    /**
     * Constructor that sets all of the attributes of a Person object
     * @param index the person id
     * @param age
     * @param sex (M or F)
     * @param house pointer to this Person's Household
     * @param school pointer to this Person's School
     * @param work pointer to this Person's Workplace
     * @param day the simulation day
     * @param today_is_birthday true if this is a newborn
     */
    void setup(int index, int id, int age, char sex, int race, int rel, Place *house,
               Place *school, Place *work, int day, bool today_is_birthday);


};

#endif // _FRED_PERSON_H

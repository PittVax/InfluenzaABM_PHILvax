//
//
// File: Health.h
//

#ifndef _PHIL_HEALTH_H
#define _PHIL_HEALTH_H

#include <vector>
#include <bitset>
using namespace std;

#include "Infection.h"
#include "Disease.h"
#include "Global.h"
#include "Past_Infection.h"

class Person;
class Disease;
class Antiviral;
class Antivirals;
class AV_Manager;
class AV_Health;
class Vaccine;
class Vaccine_Health;
class Vaccine_Manager;
class Place;

class Health {

    static int nantivirals;

  public:

    Health(Person * person);

    ~Health();

    void update(Person * self, int day);

    void update_interventions(Person * self, int day);

    void become_susceptible(Person * self, int disease_id);

    void become_susceptible(Person * self, Disease * disease);

    void become_unsusceptible(Person * self, Disease * disease);

    void become_infectious(Person * self, Disease * disease);

    void become_symptomatic(Person * self, Disease * disease);

    void become_immune(Person * self, Disease* disease);

    void become_removed(Person * self, int disease_id);

    void declare_at_risk(Disease* disease);

    void recover(Person * self, Disease * disease);

    bool is_susceptible(int disease_id) const {
        return susceptible.test(disease_id);
    }

    bool is_infectious(int disease_id) const {
        return (infectious.test(disease_id));
    }

    bool is_infected(int disease_id) const {
        return active_infections.test(disease_id);
    }

    bool is_recovered(int disease_id) const {
        return this->recovered_today.test(disease_id);
    }

    bool is_symptomatic() const {
        return symptomatic.any();
    }
    bool is_symptomatic(int disease_id) {
        return symptomatic.test(disease_id);
    }

    bool is_immune(Disease* disease) const {
        return immunity.test(disease->get_id());
    }

    bool is_immune(int disease_id) const {
        return immunity.test(disease_id);
    }

    bool is_at_risk(Disease* disease) const {
        return at_risk.test(disease->get_id());
    }

    bool is_at_risk(int disease_id) const {
        return at_risk.test(disease_id);
    }

    void advance_seed_infection(int disease_id, int days_to_advance);

    int get_exposure_date(int disease_id) const;

    int get_infectious_date(int disease_id) const;

    int get_recovered_date(int disease_id) const;

    int get_symptomatic_date(int disease_id) const;

    Person * get_infector(int disease_id) const;

    Place * get_infected_place(int disease_id) const;

    int get_infected_place_id(int disease_id) const;

    char * get_infected_place_label(int disease_id) const;

    char get_infected_place_type(int disease_id) const;

    int get_infectees(int disease_id) const;

    double get_susceptibility(int disease_id) const;

    double get_infectivity(int disease_id, int day) const;

    Infection* get_infection(int disease_id) const {
        return infection[disease_id];
    }

    bool is_on_av_for_disease(int day, int disease_id) const;

    void infect(Person * self, Person *infectee, int disease_id, Transmission & transmission);

    void become_exposed(Person * self, Disease *disease, Transmission & transmission);

    void take_vaccine(Person * self, Vaccine *vacc, int day, Vaccine_Manager* vm);

    void take(Person* self, Antiviral *av, int day);

    int get_number_av_taken() const {
        if (av_health) {
            return av_health->size();
        } else {
            return 0;
        }
    }

    int get_checked_for_av(int s) const {
        assert(checked_for_av != NULL);
        return (*checked_for_av)[ s ];
    }

    void flip_checked_for_av(int s) {
        if (checked_for_av == NULL) {
            checked_for_av = new checked_for_av_type();
            checked_for_av->assign(nantivirals, false);
        }
        (*checked_for_av)[ s ] = 1;
    }

    bool is_vaccinated() const {
        if (vaccine_health) {
            return vaccine_health->size();
        } else {
            return 0;
        }
    }

    int get_number_vaccines_taken() const {
        if (vaccine_health) {
            return vaccine_health->size();
        } else {
            return 0;
        }
    }

    AV_Health * get_av_health(int i) const {
        assert(av_health != NULL);
        return (*av_health)[ i ];
    }

    int get_av_start_day(int i) const;

    Vaccine_Health * get_vaccine_health(int i) const {
        if (vaccine_health) {
            return (*vaccine_health)[ i ];
        } else {
            return NULL;
        }
    }

    void modify_susceptibility(int disease_id, double multp);

    void modify_infectivity(int disease_id, double multp);

    void modify_infectious_period(int disease_id, double multp, int cur_day);

    void modify_symptomatic_period(int disease_id, double multp, int cur_day);

    void modify_asymptomatic_period(int disease_id, double multp, int cur_day);

    void modify_develops_symptoms(int disease_id, bool symptoms, int cur_day);

    void terminate(Person * self);

    int get_num_past_infections(int disease) {
        return past_infections[ disease ].size();
    }

    Past_Infection * get_past_infection(int disease, int i) {
        return &(past_infections[ disease ].at(i));
    }

    void clear_past_infections(int disease) {
        past_infections[ disease ].clear();
    }

    //void add_past_infection(int d, Past_Infection *pi){ past_infections[d].push_back(pi); }
    void add_past_infection(int strain_id, int recovery_date, int age_at_exposure, Disease * dis) {
        past_infections[ dis->get_id() ].push_back(Past_Infection(strain_id, recovery_date, age_at_exposure));
    }

    void update_place_counts(Person * self, int day, int disease_id, Place * place);

    bool is_newly_infected(int day, int disease_id) {
        return day == get_exposure_date(disease_id);
    }

    bool is_newly_symptomatic(int day, int disease_id) {
        return day == get_symptomatic_date(disease_id);
    }


    void die() {
        PHIL_VERBOSE(2, "Killing Agent");
        alive = false;
    }

  private:

    // The index of the person in the Population
    //int person_index;

    // The alive bool could probably be eliminated
    bool alive;

    // TODO (JVD): The infection vector & bitset should be combined into a little
    // helper class to make sure that they're always synched up.
    // There would be just a little overhead in doing this but probably well worth it.
    // Until then make sure that they're always changed together.
    Infection * infection[ Global::MAX_NUM_DISEASES ];
    // bitset removes need to check each infection in above array to
    // find out if any are not NULL
    phil::disease_bitset active_infections;
    phil::disease_bitset immunity;
    phil::disease_bitset at_risk;  // Agent is/isn't at risk for severe complications
    // Per-disease health status flags
    phil::disease_bitset susceptible;
    phil::disease_bitset infectious;
    phil::disease_bitset symptomatic;
    phil::disease_bitset recovered_today;
    phil::disease_bitset evaluate_susceptibility;

    phil::simple_strains_bitset * strain_susceptible[Global::MAX_NUM_DISEASES];

    // per-disease susceptibility multiplier
    double susceptibility_multp[ Global::MAX_NUM_DISEASES ];

    // Antivirals.  These are all dynamically allocated to save space
    // when not in use
    typedef vector < bool > checked_for_av_type;
    checked_for_av_type * checked_for_av;
    typedef vector < AV_Health * > av_health_type;
    av_health_type * av_health;
    typedef vector < AV_Health * >::iterator av_health_itr;

    // Vaccines.  These are all dynamically allocated to save space
    // when not in use
    typedef vector < Vaccine_Health * > vaccine_health_type;
    vaccine_health_type * vaccine_health;
    typedef vector < Vaccine_Health * >::iterator vaccine_health_itr;

    // Define a bitset type to hold health flags
    // Enumeration corresponding to positions in health
    // intervention_flags bitset is defined in implementation file
    typedef std::bitset< 2 > intervention_flags_type;
    intervention_flags_type intervention_flags;

    // Past_Infections used for reignition
    vector < Past_Infection > past_infections[ Global::MAX_NUM_DISEASES ];

    int infectee_count[ Global::MAX_NUM_DISEASES ];
    int susceptible_date[ Global::MAX_NUM_DISEASES ];

  protected:

    friend class Person;
    Health() { }
    void setup(Person * self);
};

#endif // _PHIL_HEALTH_H

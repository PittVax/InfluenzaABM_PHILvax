//
//
// File: Health.cc
//

#include <new>
#include <stdexcept>

#include "Health.h"
#include "Place.h"
#include "Person.h"
#include "Disease.h"
#include "Evolution.h"
#include "Infection.h"
#include "Antiviral.h"
#include "Population.h"
#include "Random.h"
#include "Manager.h"
#include "AV_Manager.h"
#include "AV_Health.h"
#include "Vaccine.h"
#include "Vaccine_Dose.h"
#include "Vaccine_Health.h"
#include "Vaccine_Manager.h"
#include "Transmission.h"
#include "Past_Infection.h"
#include "Utils.h"
#include "Tracker.h"
#include "Household.h"

int Health::nantivirals = -1;
char dummy_label[8];

// Enumerate flags corresponding to positions in
// health intervention_flags bitset
enum {
    takes_vaccine,
    takes_av
};

void Health::setup(Person * person) {
    alive = true;
    intervention_flags = intervention_flags_type();
    // infection pointers stored in statically allocated array (length of which
    // is determined by static constant Global::MAX_NUM_DISEASES)
    active_infections = phil::disease_bitset();
    susceptible = phil::disease_bitset();
    infectious = phil::disease_bitset();
    symptomatic = phil::disease_bitset();
    recovered_today = phil::disease_bitset();
    evaluate_susceptibility = phil::disease_bitset();
    immunity = phil::disease_bitset();
    // Determines if the agent is at risk
    at_risk = phil::disease_bitset();

    for (int disease_id = 0; disease_id < Global::Diseases; disease_id++) {
        infection[ disease_id ] = NULL;
        infectee_count[ disease_id ] = 0;
        susceptibility_multp[ disease_id ] = 1.0;
        susceptible_date[ disease_id ] = -1;
        become_removed(person, disease_id);
        become_susceptible(person, disease_id);
        Disease * disease = Global::Pop.get_disease(disease_id);
        if (!disease->get_at_risk()->is_empty()) {
            double at_risk_prob = disease->get_at_risk()->find_value(person->get_age());
            if (RANDOM() < at_risk_prob) {   // Now a probability <=1.0
                declare_at_risk(disease);
            }
        }
        if (Global::Enable_Simple_Strain_Model) {
            strain_susceptible[disease_id] = new phil::simple_strains_bitset;
            strain_susceptible[disease_id]->set();
        }
    }

    vaccine_health = NULL;
    av_health = NULL;
    checked_for_av = NULL;

    if (Health::nantivirals == -1) {
        Params::get_param_from_string("number_antivirals", &Health::nantivirals);
    }
}

Health::~Health() {
    // delete Infection objects pointed to
    for (size_t i = 0; i < Global::Diseases; ++i) {
        delete infection[i];
    }

    if (vaccine_health) {
        for (unsigned int i=0; i < vaccine_health->size(); i++) {
            delete(*vaccine_health)[ i ];
        }
        vaccine_health->clear();
        delete vaccine_health;
    }

    if (av_health) {
        for (unsigned int i=0; i < av_health->size(); i++) {
            delete(*av_health)[ i ];
        }
        av_health->clear();
        delete av_health;
    }

    if (checked_for_av) {
        delete checked_for_av;
    }
}

void Health::become_susceptible(Person * self, int disease_id) {
    if (susceptible.test(disease_id))
        return;
    assert(!(active_infections.test(disease_id)));
    susceptibility_multp[disease_id] = 1.0;
    susceptible.set(disease_id);
    evaluate_susceptibility.reset(disease_id);
    Disease * disease = Global::Pop.get_disease(disease_id);
    disease->become_susceptible(self);
    PHIL_STATUS(1, "person %d is now SUSCEPTIBLE for disease %d\n", self->get_id(), disease_id);
}

void Health::become_susceptible(Person * self, Disease * disease) {
    become_susceptible(self, disease->get_id());
}

void Health::become_exposed(Person * self, Disease *disease, Transmission & transmission) {
    int disease_id = disease->get_id();
    infectious.reset(disease_id);
    symptomatic.reset(disease_id);

    bool susceptible_to_strain = true;
    std::vector<int> strains;
    Person * infector = transmission.get_infector();

    if (Global::Enable_Simple_Strain_Model) {
        if (infector != NULL) {
            infector->get_health()->get_infection(disease_id)->get_strains(strains);
            for (int s=0; s<strains.size(); ++s) {
                PHIL_STATUS(1, "Person %d Evaluating strain %d\n", self->get_id(), strains[s]);
                if (strain_susceptible[disease_id]->test(strains[s])) {
                    PHIL_STATUS(1, "Person %d Susceptible to strain %d\n", self->get_id(), strains[s]);
                }
                else {
                    PHIL_STATUS(1, "Person %d Immune to strain %d\n", self->get_id(), strains[s]);
                    susceptible_to_strain = false;
                }
            }
        }
    }

    Infection *new_infection;
    if (susceptible_to_strain) {
        new_infection = disease->get_evolution()->transmit(infection[disease_id], transmission, self);
    } else {
        new_infection = NULL;
    }

    if (new_infection != NULL) {   // Transmission succeeded
        active_infections.set(disease_id);
        if (infection[ disease_id ] == NULL) {
            self->become_unsusceptible(disease);
            disease->become_exposed(self);
        }
        infection[disease_id] = new_infection;
        susceptible_date[disease_id] = -1;
        if (Global::Verbose > 1) {
            if (!(transmission.get_infected_place())) {
                PHIL_STATUS(1, "SEEDED person %d with disease %d\n", self->get_id(), disease->get_id());
            } else {
                PHIL_STATUS(1, "EXPOSED person %d to disease %d\n", self->get_id(), disease->get_id());
            }
        }
        if (Global::Report_Epidemic_Data_By_Census_Block) {
            Household *house = (Household *) self->get_household();
            string block = house->get_census_block();
            Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"S",int(-1));
            Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"C",int(1));
            Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"E",int(1));
        }
        if (Global::Enable_Simple_Strain_Model) {
            if (infector != NULL) {
                for (int s=0; s<strains.size(); ++s) {
                    if (strains[s] >= Global::SimpleStrains) {
                        Utils::phil_abort("Strain ID %d >= Global::SimpleStrains", strains[s]);
                    } else {
                        strain_susceptible[disease_id]->reset(strains[s]);
                    }
                }
            }
        }
    }
}

void Health::become_unsusceptible(Person * self, Disease * disease) {
    int disease_id = disease->get_id();
    if (susceptible.test(disease_id) == false)
        return;
    susceptible.reset(disease_id);
    disease->become_unsusceptible(self);
    PHIL_STATUS(1, "person %d is now UNSUSCEPTIBLE for disease %d\n", self->get_id(), disease_id);
}

void Health::become_infectious(Person * self, Disease * disease) {
    int disease_id = disease->get_id();
    assert(active_infections.test(disease_id));
    infectious.set(disease_id);
    disease->become_infectious(self);
    if (Global::Report_Epidemic_Data_By_Census_Block) {
        Household *house = (Household *) self->get_household();
        string block = house->get_census_block();
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"E",int(-1));
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"I",int(1));
    }
    PHIL_STATUS(1, "person %d is now INFECTIOUS for disease %d\n", self->get_id(), disease_id);
}

void Health::become_symptomatic(Person * self, Disease * disease) {
    int disease_id = disease->get_id();
    assert(active_infections.test(disease_id));
    if (symptomatic.test(disease_id))
        return;
    symptomatic.set(disease_id);
    disease->become_symptomatic(self);
    if (Global::Report_Epidemic_Data_By_Census_Block) {
        Household *house = (Household *) self->get_household();
        string block = house->get_census_block();
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"Cs",int(1));
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"Is",int(1));
    }
    PHIL_STATUS(1, "person %d is now SYMPTOMATIC for disease %d\n", self->get_id(), disease_id);
}

void Health::recover(Person * self, Disease * disease) {
    int disease_id = disease->get_id();
    assert(active_infections.test(disease_id));
    PHIL_STATUS(1, "person %d is now RECOVERED for disease %d\n", self->get_id(), disease_id);
    if (Global::Report_Epidemic_Data_By_Census_Block) {
        Household *house = (Household *) self->get_household();
        string block = house->get_census_block();
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"I",int(-1));
        if (self->is_symptomatic()) Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"Is",int(-1));
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"R",int(1));
    }
    become_removed(self, disease_id);
    recovered_today.set(disease_id);

}

void Health::become_removed(Person * self, int disease_id) {
    Disease * disease = Global::Pop.get_disease(disease_id);
    disease->become_removed(self,susceptible.test(disease_id),
                            infectious.test(disease_id),
                            symptomatic.test(disease_id));
    susceptible.reset(disease_id);
    infectious.reset(disease_id);
    symptomatic.reset(disease_id);
    PHIL_STATUS(1, "person %d is now REMOVED for disease %d\n", self->get_id(), disease_id);
}

void Health::become_immune(Person * self, Disease *disease) {
    int disease_id = disease->get_id();
    disease->become_immune(self,susceptible.test(disease_id),
                           infectious.test(disease_id),
                           symptomatic.test(disease_id));
    if (Global::Report_Epidemic_Data_By_Census_Block && Global::Block_Tracker_Initialized) {
        Household *house = (Household *) self->get_household();
        string block = house->get_census_block();
        if (susceptible.test(disease_id)) Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"S",int(-1));
        if (infectious.test(disease_id)) Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"I",int(-1));
        if (symptomatic.test(disease_id)) Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"Is",int(-1));
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"M",int(1));
    }
    susceptible.reset(disease_id);
    infectious.reset(disease_id);
    symptomatic.reset(disease_id);

    PHIL_STATUS(1, "person %d is now IMMUNE for disease %d\n", self->get_id(), disease_id);

}

void Health::update(Person * self, int day) {
    // if deceased, health status should have been cleared during population
    // update (by calling Person->die(), then Health->die(), which will reset (bool) alive
    if (!(alive)) {
        return;
    }
    // set disease-specific flags in bitset to detect calls to recover()
    recovered_today.reset();
    // if any disease has an active infection, then loop through and check
    // each disease infection
    if (active_infections.any()) {
        for (int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
            // update the infection (if it exists)
            // the check if agent has symptoms is performed by Infection->update (or one of the
            // methods called by it).  This sets the relevant symptomatic flag used by 'is_symptomatic()'
            if (active_infections.test(disease_id)) {
                infection[ disease_id ]->update(day);
                // This can only happen if the infection[disease_id] exists.
                // If the infection_update called recover(), it is now safe to
                // collect the susceptible date and delete the Infection object
                if (recovered_today.test(disease_id)) {
                    susceptible_date[ disease_id ] = infection[ disease_id ]->get_susceptible_date();
                    evaluate_susceptibility.set(disease_id);
                    if (infection[ disease_id ]->provides_immunity()) {
                        std::vector< int > strains;
                        infection[ disease_id ]->get_strains(strains);
                        std::vector< int >::iterator itr = strains.begin();
                        for (; itr != strains.end(); ++itr) {
                            int strain = *itr;
                            int recovery_date = infection[ disease_id ]->get_recovery_date();
                            int age_at_exposure = infection[ disease_id ]->get_age_at_exposure();
                            past_infections[ disease_id ].push_back(
                                Past_Infection(strain, recovery_date, age_at_exposure));
                        }
                    }
                    delete infection[ disease_id ];
                    active_infections.reset(disease_id);
                    infection[ disease_id ] = NULL;
                }
            }
        }
    }
    // First check to see if we need to evaluate susceptibility
    // for any diseases; if so check for susceptibility due to loss of immunity
    // The evaluate_susceptibility bit for that disease will be reset in the
    // call to become_susceptible
    if (evaluate_susceptibility.any()) {
        for (int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
            if (day == susceptible_date[disease_id]) {
                become_susceptible(self, disease_id);
            }
        }
    } else if (active_infections.none()) {
        // no active infections, no need to evaluate susceptibility so we no longer
        // need to update this Person's Health
        Global::Pop.clear_mask_by_index(phil::Update_Health, self->get_pop_index());
    }
} // end Health::update //

void Health::update_interventions(Person * self, int day) {
    // if deceased, health status should have been cleared during population
    // update (by calling Person->die(), then Health->die(), which will reset (bool) alive
    if (!(alive)) {
        return;
    }
    bool no_more_vaccine_updates_needed = true;
    bool no_more_antiviral_updates_needed = true;

    if (intervention_flags.any()) {
        // update vaccine status
        if (intervention_flags[ takes_vaccine ]) {
            int size = (int) vaccine_health->size();
            for (int i = 0; i < size; i++) {
                (*vaccine_health)[ i ]->update(day, self->get_age());
                if ((*vaccine_health)[ i ]->more_updates_needed(day)) {
                    no_more_vaccine_updates_needed = false;
                }
            }
        }
        // update antiviral status
        if (intervention_flags[ takes_av ]) {
            for (av_health_itr i = av_health->begin(); i != av_health->end(); ++i) {
                (*i)->update(day);
                if ((*i)->get_av_end_day() > day) {
                    no_more_antiviral_updates_needed = false;
                }
            }
        }
    }
    if (no_more_vaccine_updates_needed) {
        intervention_flags[takes_vaccine] = false;
    }
    if (no_more_antiviral_updates_needed) {
        intervention_flags[takes_av] = false;
    }
    update(self, day);
    if (no_more_vaccine_updates_needed && no_more_antiviral_updates_needed && evaluate_susceptibility.none()) {
        Global::Pop.clear_mask_by_index(phil::Update_Interventions, self->get_pop_index());
    }
} // end Health::update_interventions

void Health::declare_at_risk(Disease* disease) {
    int disease_id = disease->get_id();
    at_risk.set(disease_id);
}

void Health::advance_seed_infection(int disease_id, int days_to_advance) {
    assert(active_infections.test(disease_id));
    assert(infection[ disease_id ] != NULL);
    infection[ disease_id ]->advance_seed_infection(days_to_advance);
}

int Health::get_exposure_date(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return -1;
    else
        return infection[disease_id]->get_exposure_date();
}

int Health::get_infectious_date(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return -1;
    else
        return infection[disease_id]->get_infectious_date();
}

int Health::get_recovered_date(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return -1;
    else
        return infection[disease_id]->get_recovery_date();
}

int Health:: get_symptomatic_date(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return -1;
    else
        return infection[disease_id]->get_symptomatic_date();
}

Person * Health::get_infector(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return NULL;
    else
        return infection[disease_id]->get_infector();
}

Place * Health::get_infected_place(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return NULL;
    else
        return infection[disease_id]->get_infected_place();
}

int Health::get_infected_place_id(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return -1;
    else if (infection[disease_id]->get_infected_place() == NULL)
        return -1;
    else
        return infection[disease_id]->get_infected_place()->get_id();
}

char Health::get_infected_place_type(int disease_id) const {
    if (!(active_infections.test(disease_id)))
        return 'X';
    else if (infection[disease_id]->get_infected_place() == NULL)
        return 'X';
    else
        return infection[disease_id]->get_infected_place()->get_type();
}

char * Health::get_infected_place_label(int disease_id) const {
    if (!(active_infections.test(disease_id))) {
        strcpy(dummy_label, "-");
        return dummy_label;
    } else if (infection[disease_id]->get_infected_place() == NULL) {
        strcpy(dummy_label, "X");
        return dummy_label;
    } else
        return infection[disease_id]->get_infected_place()->get_label();
}

int Health::get_infectees(int disease_id) const {
    return infectee_count[disease_id];
}

double Health::get_susceptibility(int disease_id) const {
    double suscep_multp = susceptibility_multp[disease_id];

    if (!(active_infections.test(disease_id)))
        return suscep_multp;
    else
        return infection[disease_id]->get_susceptibility() * suscep_multp;
}

double Health::get_infectivity(int disease_id, int day) const {
    if (!(active_infections.test(disease_id))) {
        return 0.0;
    } else {
        return infection[disease_id]->get_infectivity(day);
    }
}

//Modify Operators
void Health::modify_susceptibility(int disease_id, double multp) {
    susceptibility_multp[disease_id] *= multp;
}

void Health::modify_infectivity(int disease_id, double multp) {
    if (active_infections.test(disease_id)) {
        infection[disease_id]->modify_infectivity(multp);
    }
}

void Health::modify_infectious_period(int disease_id, double multp, int cur_day) {
    if (active_infections.test(disease_id)) {
        infection[disease_id]->modify_infectious_period(multp, cur_day);
    }
}

void Health::modify_asymptomatic_period(int disease_id, double multp, int cur_day) {
    if (active_infections.test(disease_id)) {
        infection[disease_id]->modify_asymptomatic_period(multp, cur_day);
    }
}

void Health::modify_symptomatic_period(int disease_id, double multp, int cur_day) {
    if (active_infections.test(disease_id)) {
        infection[disease_id]->modify_symptomatic_period(multp, cur_day);
    }
}

void Health::modify_develops_symptoms(int disease_id, bool symptoms, int cur_day) {
    if (active_infections.test(disease_id) &&
            ((infection[disease_id]->is_infectious() && !infection[disease_id]->is_symptomatic()) ||
             !infection[disease_id]->is_infectious())) {

        infection[disease_id]->modify_develops_symptoms(symptoms, cur_day);
        symptomatic.set(disease_id);
    }
}

//Medication operators
void Health::take_vaccine(Person * self, Vaccine* vaccine, int day, Vaccine_Manager* vm) {
    // Compliance will be somewhere else
    int age = self->get_age();
    // Is this our first dose?
    Vaccine_Health * vaccine_health_for_dose = NULL;

    if (Global::Report_Epidemic_Data_By_Census_Block) {
        Household *house = (Household *) self->get_household();
        string block = house->get_census_block();
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"V",int(1));
    }
    if (vaccine_health == NULL) {
        vaccine_health = new vaccine_health_type();
    }

    for (unsigned int ivh = 0; ivh < vaccine_health->size(); ivh++) {
        if ((*vaccine_health)[ ivh ]->get_vaccine() == vaccine) {
            vaccine_health_for_dose = (*vaccine_health)[ ivh ];
        }
    }

    if (vaccine_health_for_dose == NULL) {   // This is our first dose of this vaccine
        vaccine_health_for_dose = new Vaccine_Health(day, vaccine, age, self, vm);
        vaccine_health->push_back(vaccine_health_for_dose);
        intervention_flags[ takes_vaccine ] = true;
    } else { // Already have a dose, need to take the next dose
        vaccine_health_for_dose->update_for_next_dose(day,age);
    }

    if (Global::VaccineTracefp != NULL) {
        fprintf(Global::VaccineTracefp," id %7d vaccid %3d",
                self->get_id(),(*vaccine_health)[ vaccine_health->size() - 1 ]->get_vaccine()->get_ID());
        (*vaccine_health)[ vaccine_health->size() - 1 ]->printTrace();
        fprintf(Global::VaccineTracefp,"\n");
    }

    if (Global::Enable_Simple_Strain_Model) {
        int nstrains = vaccine_health_for_dose->get_vaccine()->get_num_strains();
        int * strains = vaccine_health_for_dose->get_vaccine()->get_strains();
        int disease_id = vaccine_health_for_dose->get_vaccine()->get_disease();
        Disease * disease = Global::Pop.get_disease(disease_id);
        for (int i=0; i<nstrains; ++i) {
            strain_susceptible[disease_id]->reset(strains[i]);
        }
        susceptible_date[disease_id] = day + disease->get_days_recovered();
        evaluate_susceptibility.set(disease_id);
        PHIL_STATUS(1, "Simple Strain Model: person %d is temporarily immune to disease %d until %d\n",
                self->get_id(), disease_id, susceptible_date[disease_id]);
    }
    Global::Pop.set_mask_by_index(phil::Update_Interventions, self->get_pop_index());

    return;
}

void Health::take(Person * p, Antiviral* av, int day) {
    if (checked_for_av == NULL) {
        checked_for_av = new checked_for_av_type();
        checked_for_av->assign(nantivirals, false);
    }
    if (av_health == NULL) {
        av_health = new av_health_type();
    }
    if (Global::Report_Epidemic_Data_By_Census_Block) {
        Household *house = (Household *) p->get_household();
        string block = house->get_census_block();
        Global::Block_Epi_Day_Tracker->increment_index_key_pair(block,"Av",int(1));
    }
    av_health->push_back(new AV_Health(day,av,this));
    intervention_flags[ takes_av ] = true;
    Global::Pop.set_mask_by_index(phil::Update_Interventions, p->get_pop_index());
    return;
}

bool Health::is_on_av_for_disease(int day, int d) const {
    for (unsigned int iav = 0; iav < av_health->size(); iav++)
        if ((*av_health)[ iav ]->get_disease() == d && (*av_health)[ iav ]->is_on_av(day))
            return true;
    return false;
}

int Health::get_av_start_day(int i) const {
    assert(av_health != NULL);
    return (*av_health)[ i ]->get_av_start_day();
}


void Health::infect(Person * self, Person *infectee, int disease_id, Transmission & transmission) {
    // 'infect' call chain:
    // Person::infect => Health::infect => Infection::transmit [Create transmission
    // and expose infectee]
    Disease * disease = Global::Pop.get_disease(disease_id);
    infection[ disease_id ]->transmit(infectee, transmission);

    #pragma omp atomic
    ++(infectee_count[ disease_id ]);

    disease->increment_cohort_infectee_count(infection[disease_id]->get_exposure_date());

    PHIL_STATUS(1, "person %d infected person %d infectees = %d\n",
                self->get_id(), infectee->get_id(), infectee_count[disease_id]);
}

void Health::update_place_counts(Person * self, int day, int disease_id, Place * place) {
    if (place == NULL) return;
    if (is_infected(disease_id)) {
        // printf("DAY %d person %d place %s ", day, self->get_id(), place->get_label());
        if (is_newly_infected(day, disease_id)) {
            place->add_new_infection(disease_id);
            // printf("NEWLY ");
        }
        place->add_current_infection(disease_id);
        // printf("INFECTED ");

        if (is_symptomatic(disease_id)) {
            if (is_newly_symptomatic(day, disease_id)) {
                place->add_new_symptomatic_infection(disease_id);
                // printf("NEWLY ");
            }
            place->add_current_symptomatic_infection(disease_id);
            // printf("SYMPTOMATIC");
        }
        // printf("\n");
    }
}

void Health::terminate(Person * self) {
    for (int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
        become_removed(self, disease_id);
    }
}


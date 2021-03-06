//
//
// File: Vaccine_Manager.cpp
//

#include "Manager.h"
#include "Vaccine_Manager.h"
#include "Policy.h"
#include "Vaccine_Priority_Policies.h"
#include "Population.h"
#include "Vaccines.h"
#include "Vaccine.h"
#include "Person.h"
#include "Health.h"
#include "Activities.h"
#include "Params.h"
#include "Random.h"
#include "Global.h"
#include "Timestep_Map.h"
#include "Age_Map.h"
#include "Utils.h"

#include <algorithm>

Vaccine_Manager::Vaccine_Manager() {
    vaccine_package = NULL;
    vaccine_priority_age_low = -1;
    vaccine_priority_age_high = -1;
    current_vaccine_capacity = -1;
    vaccine_priority_only = false;
    vaccination_capacity_map = NULL;
    vaccine_coverage_by_age = false;
    vaccine_coverage_age_maps = NULL;
    do_vacc = false;
}

Vaccine_Manager::Vaccine_Manager(Population *_pop):
    Manager(_pop) {

    pop = _pop;

    vaccine_package = new Vaccines();
    int num_vaccs = 0;
    Params::get_param_from_string("number_of_vaccines",&num_vaccs);
    if (num_vaccs > 0) {
        vaccine_package->setup();
        vaccine_package->print();
        do_vacc = 1;
    } else { // No vaccination specified.
        vaccine_priority_age_low = -1;
        vaccine_priority_age_high = -1;
        vaccination_capacity_map = NULL;
        current_vaccine_capacity = -1;
        vaccine_dose_priority = -1;
        vaccine_priority_only = false;
        do_vacc = false;
        return;
    }
    // ACIP Priority takes precidence
    int do_acip_priority;
    current_policy = VACC_NO_PRIORITY;
    Params::get_param_from_string("vaccine_prioritize_acip",&do_acip_priority);
    if (do_acip_priority == 1) {
        cout << "Vaccination Priority using ACIP recommendations\n";
        cout << "   Includes: \n";
        cout << "        Ages 0 to 24\n";
        cout << "        Pregnant Women\n";
        cout << "        Persons at risk for complications\n";
        current_policy = VACC_ACIP_PRIORITY;
        vaccine_priority_age_low = 0;
        vaccine_priority_age_high = 24;
    } else {
        int do_age_priority;
        Params::get_param_from_string("vaccine_prioritize_by_age",&do_age_priority);
        if (do_age_priority) {
            cout <<"Vaccination Priority by Age\n";
            current_policy = VACC_AGE_PRIORITY;
            Params::get_param_from_string("vaccine_priority_age_low",&vaccine_priority_age_low);
            Params::get_param_from_string("vaccine_priority_age_high",&vaccine_priority_age_high);
            cout <<"      Between Ages "<< vaccine_priority_age_low << " and "
                 << vaccine_priority_age_high << "\n";
        } else {
            vaccine_priority_age_low = 0;
            vaccine_priority_age_high = 110;
        }
    }

    // should we vaccinate anyone outside of the priority class
    int vacc_pri_only;
    vaccine_priority_only = false;
    Params::get_param_from_string("vaccine_priority_only",&vacc_pri_only);
    if (vacc_pri_only) {
        vaccine_priority_only = true;
        cout << "      Vaccinating only the priority groups\n";
    }

    // should we exclude people that have had symptomatic infections?
    int vacc_sympt_exclude;
    vaccinate_symptomatics = false;
    Params::get_param_from_string("vaccinate_symptomatics",&vacc_sympt_exclude);
    if (vacc_sympt_exclude) {
        vaccinate_symptomatics = true;
        cout << "      Vaccinating symptomatics\n";
    }

    // get vaccine_dose_priority
    Params::get_param_from_string("vaccine_dose_priority",&vaccine_dose_priority);
    assert(vaccine_dose_priority < 4);
    //get_param((char*)"vaccination_capacity",&vaccination_capacity);
    vaccination_capacity_map = new Timestep_Map("vaccination_capacity");
    vaccination_capacity_map->read_map();
    if (Global::Verbose > 1)
        vaccination_capacity_map->print();

    // Adding an age specific vaccine compliance
    int vacc_cov_by_age;
    vaccine_coverage_by_age = false;

    Params::get_param_from_string("vaccine_coverage_by_age",&vacc_cov_by_age);
    if (vacc_cov_by_age) {
        vaccine_coverage_by_age = true;
        //current_policy = VACC_AGE_SPEC_COVERAGE_PRIORITY;
        //vaccine_priority_only = true;
        current_policy = VACC_AGE_SPEC_COVERAGE_MULTI_VACCINE_PRIORITY;
        vaccine_priority_only = true;
        vaccine_coverage_age_maps = new Age_Map*[vaccine_package->get_num_vaccines()];
        for (int i=0; i<vaccine_package->get_num_vaccines(); ++i) {
            char vaccine_coverage_age_map_param[128];
            sprintf(vaccine_coverage_age_map_param, "Vaccine %d Coverage", i);
            vaccine_coverage_age_maps[i] = new Age_Map(vaccine_coverage_age_map_param);
            vaccine_coverage_age_maps[i]->read_from_input("vaccine_coverage_age_map", i);
            if (Global::Verbose > 1) {
                vaccine_coverage_age_maps[0]->print();
            }
        }
    }

    // Need to fill the Vaccine_Manager Policies
    policies.push_back(new Vaccine_Priority_Policy_No_Priority(this));
    policies.push_back(new Vaccine_Priority_Policy_Specific_Age(this));
    policies.push_back(new Vaccine_Priority_Policy_ACIP(this));
    policies.push_back(new Vaccine_Priority_Policy_Age_Specific_Coverage(this));
    policies.push_back(new Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage(this));

};

Vaccine_Manager::~Vaccine_Manager() {
    if (vaccine_package != NULL) delete vaccine_package;
    if (vaccination_capacity_map != NULL) delete vaccination_capacity_map;
}

void Vaccine_Manager::fill_queues() {

    if (!do_vacc) return;
    // We need to loop over the entire population that the Manager oversees to put them in a queue.
    int popsize = pop->get_pop_size();

    for (int ip = 0; ip < popsize; ip++) {
        Person* current_person = pop->get_person_by_index(ip);
        if (policies[current_policy]->choose_first_positive(current_person,0,0)==true)
            priority_queue.push_back(current_person);
        else if (vaccine_priority_only == false)
            queue.push_back(current_person);
    }

    vector <Person *> random_queue(queue.size());
    copy(queue.begin(),queue.end(),random_queue.begin());
    FYShuffle < Person *>(random_queue);
    copy(random_queue.begin(), random_queue.end(),queue.begin());

    vector <Person *> random_priority_queue(priority_queue.size());
    copy(priority_queue.begin(),priority_queue.end(),random_priority_queue.begin());
    FYShuffle <Person *> (random_priority_queue);
    copy(random_priority_queue.begin(),random_priority_queue.end(),priority_queue.begin());

    if (Global::Verbose >= 0) {
        cout << "Vaccine Queue Stats \n";
        cout << "   Number in Priority Queue      = " << priority_queue.size() << "\n";
        cout << "   Number in Regular Queue       = " << queue.size() << "\n";
        cout << "   Total Agents in Vaccine Queue = " << priority_queue.size() + queue.size() << "\n";
    }
}

void Vaccine_Manager::add_to_queue(Person* person) {
    if (policies[current_policy]->choose_first_positive(person,0,0)==true) {
        add_to_priority_queue_random(person);
    } else {
        if (vaccine_priority_only == false) {
            add_to_regular_queue_random(person);
        }
    }
}

void Vaccine_Manager::remove_from_queue(Person* person) {
    // remove the person from the queue if they are in there
    list<Person *>::iterator pq = find(priority_queue.begin(),priority_queue.end(),person);
    if (pq != priority_queue.end()) {
        priority_queue.erase(pq);
        return;
    }
    pq = find(queue.begin(),queue.end(),person);
    if (pq != queue.end()) {
        queue.erase(pq);
    }
}

void Vaccine_Manager::add_to_priority_queue_random(Person* person) {
    // Find a position to put the person in
    int size = priority_queue.size();
    int position = (int)(RANDOM()*size);

    list<Person*>::iterator pq = priority_queue.begin();
    for (int i = 0; i < position; i++) ++pq;
    priority_queue.insert(pq,person);
}

void Vaccine_Manager::add_to_regular_queue_random(Person* person) {
    // Find a position to put the person in
    int size = queue.size();
    int position = (int)(RANDOM()*size);

    list<Person*>::iterator pq = queue.begin();
    for (int i = 0; i < position; i++) ++pq;
    queue.insert(pq,person);
}

void Vaccine_Manager::add_to_priority_queue_begin(Person* person) {
    priority_queue.push_front(person);
}

void Vaccine_Manager::add_to_priority_queue_end(Person* person) {
    priority_queue.push_back(person);
}

string Vaccine_Manager::get_vaccine_dose_priority_string() const {
    switch (vaccine_dose_priority) {
    case VACC_DOSE_NO_PRIORITY:
        return "No Priority";
    case VACC_DOSE_FIRST_PRIORITY:
        return "Priority, Place at Beginning of Queue";
    case VACC_DOSE_RAND_PRIORITY:
        return "Priority, Place with other Priority";
    case VACC_DOSE_LAST_PRIORITY:
        return "Priority, Place at End of Queue";
    default:
        PHIL_WARNING("Unrecognized Vaccine Dose Priority\n");
        return "";
    }
    PHIL_WARNING("Unrecognized Vaccine Dose Priority\n");
    return "";
}


void Vaccine_Manager::update(int day) {
    if (do_vacc == 1) {
        vaccine_package->update(day);
        // Update the current vaccination capacity
        current_vaccine_capacity = vaccination_capacity_map->get_value_for_timestep(day, Global::Vaccine_offset);
        cout << "Current Vaccine Stock = " << vaccine_package->get_total_vaccines_avail_today()  << "\n";
        vaccinate(day);
    }
}

void Vaccine_Manager::reset() {
    priority_queue.clear();
    queue.clear();
    if (do_vacc) {
        fill_queues();
        vaccine_package->reset();
    }
}

void Vaccine_Manager::print() {
    vaccine_package->print();
}


void Vaccine_Manager::vaccinate(int day) {
    if (do_vacc)
        cout << "Vaccinating!\n";
    else {
        cout << "Not vaccinating!\n";
        return;
    }

    process_vaccine_queue("priority", day);
    process_vaccine_queue("regular", day);
}

/////////////////////


void Vaccine_Manager::process_vaccine_queue(const std::string& queue_name, int day) {

    std::list<Person*> *this_queue;
    if (queue_name.compare("priority")==0) {
        this_queue = &priority_queue;
    }
    else if (queue_name.compare("regular")==0) {
        this_queue = &queue;
    }
    else {
        Utils::phil_abort("Invalid queue specified!");
    }

    int number_vaccinated = 0;
    int n_p_vaccinated = 0;
    int n_r_vaccinated = 0;
    // Figure out the total number of vaccines we can hand out today
    int total_vaccines_avail = vaccine_package->get_total_vaccines_avail_today();

    if (Global::Verbose >= 0) {
        cout << "Vaccine Capacity on Day " << day << " = " << current_vaccine_capacity << "\n";
        cout << queue_name << " queue state at beginning of vaccination: " << this_queue->size() << "\n";
    }
    if (total_vaccines_avail == 0 || current_vaccine_capacity == 0) {
        if (Global::Verbose >= 0) {
            cout << "No Vaccine Available on Day " << day << "\n";
        }
        return;
    }

    // Start vaccinating Priority
    list < Person* >:: iterator ip;
    ip = this_queue->begin();
    int accept_count = 0;
    int reject_count = 0;

    while (ip!=this_queue->end()) {
        Person* current_person = *ip;

        int vacc_app = vaccine_package->pick_from_applicable_vaccines(current_person->get_age());
        if (vacc_app > -1) {
            bool accept_vaccine = false;
            // STB need to refactor to work with multiple diseases
            if ((vaccinate_symptomatics==true) &&
                    (current_person->get_health()->get_symptomatic_date(0) != -1) &&
                    (day >= current_person->get_health()->get_symptomatic_date(0))) {
                accept_vaccine = false;
                reject_count++;
            }
            else {
                if (current_person->get_health()->is_vaccinated()) {
                    accept_vaccine = current_person->acceptance_of_another_vaccine_dose(day);
                    if (accept_vaccine && vaccine_coverage_by_age && Global::Enable_Simple_Strain_Model) {
                        accept_vaccine = policies[current_policy]->reevaluate_check(current_person, 0, day, vacc_app);
                    }
                }
                else {
                    accept_vaccine = current_person->acceptance_of_vaccine(day);
                    if (accept_vaccine && vaccine_coverage_by_age && Global::Enable_Simple_Strain_Model) {
                        accept_vaccine = policies[current_policy]->reevaluate_check(current_person, 0, day, vacc_app);
                    }
                }
            }
            if (accept_vaccine==true) {
                accept_count++;
                number_vaccinated++;
                current_vaccine_capacity--;
                n_p_vaccinated++;
                Vaccine* vacc = vaccine_package->get_vaccine(vacc_app);
                vacc->remove_stock(1);
                total_vaccines_avail--;
                current_person->take_vaccine(vacc, day, this);
                if (vaccine_coverage_by_age && Global::Enable_Simple_Strain_Model) {
                    assert(policies[current_policy]->reevaluate_commit(current_person, 0, day, vacc_app));
                    ++ip;
                }
                else {
                    ip = this_queue->erase(ip);  // remove a vaccinated person
                }
            }
            else {
                reject_count++;
                if (vaccine_coverage_by_age && Global::Enable_Simple_Strain_Model) {
                    ++ip;
                }
                else {
                    ip = this_queue->erase(ip);
                }
            }
        } else {
            if (Global::Verbose > 1) {
                cout << "Vaccine not applicable for agent "<<current_person->get_id() << " " \
                     << current_person->get_age() << "\n";
            }
            ++ip;
        }

        if (total_vaccines_avail == 0) {
            if (Global::Verbose >= 0) {
                cout << "Vaccinated " << queue_name << " to stock out " << n_p_vaccinated << " agents, for a total of "
                     << number_vaccinated << " on day " << day << "\n";
                cout << "Left in " << queue_name << " queue:" << this_queue->size() << "\n";
                cout << "Number of acceptances: " << accept_count << " ";
                cout << "Number of rejections: " << reject_count << "\n";
            }
            return;
        }
        if (current_vaccine_capacity == 0) {
            if (Global::Verbose >= 0) {
                cout << "Vaccinated " << queue_name << " to capacity "<< n_p_vaccinated << " agents, for a total of "
                     << number_vaccinated << " on day " << day << "\n";
                cout << "Left in " << queue_name << " queue: " << this_queue->size() << "\n";
                cout << "Number of acceptances: " << accept_count << " ";
                cout << "Number of rejections: " << reject_count << "\n";            }
            return;
        }
    }


    if (Global::Verbose >= 0)
        cout << "Vaccinated " << queue_name << "  to population " << n_p_vaccinated
             << " agents, for a total of "<< number_vaccinated << " on day "
             << day << "\n";

}



//
//
// File: Policy.h
//



#ifndef _PHIL_POLICY_H
#define _PHIL_POLICY_H

#include <iostream>
#include <string>
#include <vector>

class Manager;
class Decision;
class Person;

using namespace std;

/**
 * A Policy is a class that is accessed by the manager to decide something.
 * Will be used for mitigation strategies.
 */
class Policy {

  public:

    /**
     * Default constructor
     */
    Policy();

    /**
     * Constructor that sets this Policy's manager.
     *
     * @param mgr the manager of this Policy
     */
    Policy(Manager* mgr);

    ~Policy();

    /**
     * @param person a pointer to a person object
     * @param disease the disease
     * @param current_day the simulation day
     *
     * @return
     */
    virtual int choose(Person* person, int disease, int current_day);

    virtual bool choose_first_positive(Person* person, int disease, int current_day);
    virtual bool choose_first_negative(Person* person, int disease, int current_day);
    // decision will return -1 if the decision is no
    // or the integer result of the policies in the decision
    virtual bool reevaluate_check(Person * person, int disease_id, int day, int vaccine_id) {return true;};
    virtual bool reevaluate_commit(Person * person, int disease_id, int day, int vaccine_id) {return true;};

    /**
     * @return a pointer to this Policy's Manager object
     */
    Manager* get_manager() const {
        return manager;
    }

    /**
     * Put this object back to its original state
     */
    void reset();

    /**
     * Print out information about this object
     */
    void print() const;

  protected:
    vector < Decision * > decision_list;
    string Name;
    Manager* manager;

};

#endif

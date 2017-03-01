//
//
// File: Decision.h
//

#ifndef _PHIL_DECISION_H
#define _PHIL_DECISION_H

#include <iostream>
#include <string>
#include <list>

class Policy;
class Person;

using namespace std;

class Decision {

  protected:
    string name;
    string type;
    Policy *policy;  // This is the policy that the decision belongs to

  public:
    Decision();
    Decision(Policy *p);
    ~Decision();

    /**
     * @return the name of this Decision
     */
    string get_name() const {
        return name;
    }

    /**
     * @return the type of this Decision
     */
    string get_type() const {
        return type;
    }

    /**
     * Evaluate the Decision for an agent and disease on a given day
     *
     * @param person a pointer to a Person object
     * @param disease the disease to evaluate for
     * @param current_day the simulation day
     *
     * @return the evaluation value
     */
    virtual int evaluate(Person* person, int disease, int current_day) = 0;
    virtual int reevaluate(Person* person, int disease_id, int current_day, int vaccine_id, bool commit) {
        return true;
    }
};
#endif

//
// File: Vaccine_Priority_Decision.h
//

#ifndef _PHIL_VACCINE_PRIORITY_DECISIONS_H
#define _PHIL_VACCINE_PRIORITY_DECISIONS_H


#include "Decision.h"
#include <vector>
#include <map>

class Policy;
class Person;

class Vaccine_Priority_Decision_Specific_Age: public Decision {
  public:
    Vaccine_Priority_Decision_Specific_Age(Policy* p);
    Vaccine_Priority_Decision_Specific_Age();
    int evaluate(Person* person, int disease, int day);
};

class Vaccine_Priority_Decision_Pregnant: public Decision {
  public:
    Vaccine_Priority_Decision_Pregnant(Policy* p);
    Vaccine_Priority_Decision_Pregnant();
    int evaluate(Person* person, int disease, int day);
};

class Vaccine_Priority_Decision_At_Risk: public Decision {
  public:
    Vaccine_Priority_Decision_At_Risk(Policy* p);
    Vaccine_Priority_Decision_At_Risk();
    int evaluate(Person* person, int disease, int day);
};

class Vaccine_Priority_Decision_No_Priority: public Decision {
  public:
    Vaccine_Priority_Decision_No_Priority(Policy *p);
    Vaccine_Priority_Decision_No_Priority();
    int evaluate(Person* person, int disease, int day);
};

class Vaccine_Priority_Decision_AgeMap_Coverage: public Decision {
  public:
    Vaccine_Priority_Decision_AgeMap_Coverage(Policy *p);
    Vaccine_Priority_Decision_AgeMap_Coverage();
    int evaluate(Person* person, int disease, int day);
    int evaluate(Person* person, int disease, int day, int vaccine_id) {
        return 1;
    }
};

class Vaccine_Priority_Decision_MultiVaccine_AgeMap_Coverage: public Decision {
  public:
    Vaccine_Priority_Decision_MultiVaccine_AgeMap_Coverage(Policy *p);
    Vaccine_Priority_Decision_MultiVaccine_AgeMap_Coverage();
    int evaluate(Person* person, int disease, int day);
    int reevaluate(Person* person, int disease, int day, int vaccine_id, bool commit);
  private:
    std::vector< std::map<int, double > > vaccine_counts_by_bin;
    std::vector< double > vaccine_total_counts;
    bool noise;
};

#endif







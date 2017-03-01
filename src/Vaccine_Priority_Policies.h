//
//
// File: Vaccine_Priority_Policies.h
//

#ifndef _PHIL_VACCINE_PRIORITY_POLICIES_H
#define _PHIL_VACCINE_PRIORITY_POLICIES_H

#include <iostream>
#include <string>

#include "Policy.h"

class Decision;
class Person;
class Vaccines;
class Vaccine_Manager;
class Manager;

using namespace std;

class Vaccine_Priority_Policy_No_Priority: public Policy {
    Vaccine_Manager *vacc_manager;

  public:
    Vaccine_Priority_Policy_No_Priority() { }
    Vaccine_Priority_Policy_No_Priority(Vaccine_Manager* vcm);
};

class Vaccine_Priority_Policy_Specific_Age:public Policy {
    Vaccine_Manager *vacc_manager;

  public:
    Vaccine_Priority_Policy_Specific_Age();
    Vaccine_Priority_Policy_Specific_Age(Vaccine_Manager* vcm);
};

class Vaccine_Priority_Policy_ACIP:public Policy {
    Vaccine_Manager *vacc_manager;

  public:
    Vaccine_Priority_Policy_ACIP();
    Vaccine_Priority_Policy_ACIP(Vaccine_Manager* vcm);
};

class Vaccine_Priority_Policy_Age_Specific_Coverage:public Policy {
    Vaccine_Manager *vacc_manager;

  public:
    Vaccine_Priority_Policy_Age_Specific_Coverage();
    Vaccine_Priority_Policy_Age_Specific_Coverage(Vaccine_Manager* vcm);
};

class Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage:public Policy {
    Vaccine_Manager *vacc_manager;

  public:
    Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage();
    Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage(Vaccine_Manager* vcm);
    bool reevaluate_check(Person * person, int disease_id, int day, int vaccine_id);
    bool reevaluate_commit(Person * person, int disease_id, int day, int vaccine_id);
};

#endif




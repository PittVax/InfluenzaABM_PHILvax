//
//
// File: Vaccine_Priority_Policies.cc
//

#include <iostream>
#include <vector>

#include "Decision.h"
#include "Vaccine_Priority_Policies.h"
#include "Vaccine_Priority_Decisions.h"
#include "Policy.h"
#include "Manager.h"
#include "Vaccine_Manager.h"

Vaccine_Priority_Policy_No_Priority::Vaccine_Priority_Policy_No_Priority(Vaccine_Manager *vcm):
    Policy(dynamic_cast <Manager *>(vcm)) {

    Name = "Vaccine Priority Policy - No Priority";

    decision_list.push_back(new Vaccine_Priority_Decision_No_Priority(this));
}

Vaccine_Priority_Policy_Specific_Age::Vaccine_Priority_Policy_Specific_Age(Vaccine_Manager *vcm):
    Policy(dynamic_cast <Manager *>(vcm)) {

    Name = "Vaccine Priority Policy - Sepcific Age Group";
    decision_list.push_back(new Vaccine_Priority_Decision_Specific_Age(this));
}

Vaccine_Priority_Policy_ACIP::Vaccine_Priority_Policy_ACIP(Vaccine_Manager *vcm):
    Policy(dynamic_cast <Manager *>(vcm)) {

    Name = "Vaccine Priority Policy - ACIP Priority";
    decision_list.push_back(new Vaccine_Priority_Decision_Specific_Age(this));
    decision_list.push_back(new Vaccine_Priority_Decision_Pregnant(this));
    decision_list.push_back(new Vaccine_Priority_Decision_At_Risk(this));
}

Vaccine_Priority_Policy_Age_Specific_Coverage::Vaccine_Priority_Policy_Age_Specific_Coverage(Vaccine_Manager *vcm):
    Policy(dynamic_cast <Manager *>(vcm)) {

    Name = "Vaccine Priority Policy - Age Specific Coverage";
    decision_list.push_back(new Vaccine_Priority_Decision_AgeMap_Coverage(this));
}

Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage::Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage(Vaccine_Manager *vcm):
    Policy(dynamic_cast <Manager *>(vcm)) {

    Name = "Vaccine Priority Policy - Multi Vaccine Age Specific Coverage";
    decision_list.push_back(new Vaccine_Priority_Decision_MultiVaccine_AgeMap_Coverage(this));
}

bool Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage::reevaluate_check(Person * person, int disease_id, int day, int vaccine_id) {
    return decision_list[0]->reevaluate(person, disease_id, day, vaccine_id, false);
}

bool Vaccine_Priority_Policy_Multi_Vaccine_Age_Specific_Coverage::reevaluate_commit(Person * person, int disease_id, int day, int vaccine_id) {
    return decision_list[0]->reevaluate(person, disease_id, day, vaccine_id, true);
}


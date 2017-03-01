//
//
// File: Decision.cpp
//

#include "Policy.h"
#include "Decision.h"

Decision::Decision() {
    name = "";
    type = "";
    policy = NULL;
}

Decision::~Decision() { }

Decision::Decision(Policy *p) {
    policy = p;
    name = "Generic Decision";
    type = "Generic";
}



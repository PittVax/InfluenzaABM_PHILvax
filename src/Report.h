//
// File: Report.h
//

#ifndef _PHIL_REPORT_H
#define _PHIL_REPORT_H

#include <new>
#include <vector>
#include <deque>
#include <map>
#include <iostream>

#include "Random.h"
#include "Global.h"
#include "State.h"
#include "json.h"

using nlohmann::json;

struct Report_State {

    phil::Spin_Mutex mutex;
    std::vector< json > report_vector;

    void clear() {
        phil::Spin_Lock lock(mutex);
        report_vector.clear();
    }

    size_t report_vector_size() {
        phil::Spin_Lock lock(mutex);
        return report_vector.size();
    }

    void append(json & j) {
        phil::Spin_Lock lock(mutex);
        report_vector.push_back(j);
    }

    std::vector< json > & get_report_vector() {
        phil::Spin_Lock lock(mutex);
        return report_vector;
    }

    void reset() {
        phil::Spin_Lock lock(mutex);
        if (report_vector.size() > 0) {
            report_vector = std::vector< json >();
        }
    }

    void print() {
        phil::Spin_Lock lock(mutex);
        std::vector< json >::iterator itr;
        for (itr = report_vector.begin(); itr < report_vector.end(); ++itr) {
            //std::cout << *itr << std::endl;
            fputs(itr->dump().c_str(), Global::Reportfp);
            fputs("\n", Global::Reportfp);
        }
    }

};

class Report {

  public:

    void setup();
    void print();
    void clear();
    void append(json & j);

  protected:

    State< Report_State > report_state;

};


#endif // _PHIL_REPORT_H

//
//
// File: Vaccines.h
//

#ifndef _PHIL_VACCINES_H
#define _PHIL_VACCINES_H

class Vaccine;
class Vaccine_Dose;

class Vaccines {
    // Vaccines is a class used to describe a group of Vaccine Classes
  public:
    // Creation Operations
    Vaccines() { }
    void setup();

    Vaccine *get_vaccine(int i) const {
        return vaccines[i];
    }

    vector <int> which_vaccines_applicable(int age) const;
    int pick_from_applicable_vaccines(int age) const;
    int get_total_vaccines_avail_today() const;


    //utility Functions
    void print() const;
    void print_current_stocks() const;
    void update(int day);
    void reset();
    int get_num_vaccines() {return vaccines.size();}
  private:
    vector < Vaccine* > vaccines;
};

#endif

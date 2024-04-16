// Add the Arduino library
#include "Arduino.h"
#inlcude <string>

// PID libraries 
#include <PID_v1.h>

// Fuzzy libraries
#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyIO.h>
#include <FuzzyInput.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>

// Check if the header is called repeated
#ifndef controllers_h
#define controllers_h

/*
  starts the code 
  add information and license
*/
class PID_control
{
  public:

    // public variables
    const float &desired_temp;
    const float &sensed_temp;
    const int &output_signal_;

    PID_control(float& output, float& setPoint, float& input);
    ~PID_control();

    void set_controller();
    int actuator_signal();

    // get functions
    float get_kp();
    float get_ki();
    float get_kd();

  private:

    void reset_gains(float kp, float ki, float kd);
    void reset_direction(string& direction);

    const string direction;
    float kp_gain_;
    float ki_gain_;
    float kd_gain_;
};

class Fuzzy_control
{
  public:
    
    Fuzzy_control();
    ~Fuzzy_control();
};

#endif
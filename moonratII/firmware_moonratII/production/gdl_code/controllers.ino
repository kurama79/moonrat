#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyIO.h>
#include <FuzzyInput.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>

// Instantiating a Fuzzy object
Fuzzy *fuzzy = new Fuzzy();
long randNumber;
float input;
const int sensorPin = A1;
int ledPin = 9;
float temperaturaActual;

void setup()
{
  // Set the Serial output
  Serial.begin(9600);
  // Set a random seed
  randomSeed(analogRead(0));

  // Instantiating a FuzzyInput object
  FuzzyInput *temp = new FuzzyInput(1);
  // Instantiating a FuzzySet object
  FuzzySet *cold = new FuzzySet(10, 10, 30, 35);
  // Including the FuzzySet into FuzzyInput
  temp->addFuzzySet(cold);
  // Instantiating a FuzzySet object
  FuzzySet *target = new FuzzySet(33, 35, 35, 38);
  // Including the FuzzySet into FuzzyInput
  temp->addFuzzySet(target);
  // Instantiating a FuzzySet object
  FuzzySet *hot = new FuzzySet(35, 37, 60, 60);
  // Including the FuzzySet into FuzzyInput
  temp->addFuzzySet(hot);
  // Including the FuzzyInput into Fuzzy
  fuzzy->addFuzzyInput(temp);

  // Instantiating a FuzzyOutput objects
  FuzzyOutput *pwm = new FuzzyOutput(1);
  // Instantiating a FuzzySet object
  FuzzySet *fast = new FuzzySet(80, 150, 255, 255);
  // Including the FuzzySet into FuzzyOutput
  pwm->addFuzzySet(fast);
  // Instantiating a FuzzySet object
  FuzzySet *average = new FuzzySet(10, 30, 30, 1500);
  // Including the FuzzySet into FuzzyOutput
  pwm->addFuzzySet(average);
  // Instantiating a FuzzySet object
  FuzzySet *slow = new FuzzySet(0, 0, 0, 20);
  // Including the FuzzySet into FuzzyOutput
  pwm->addFuzzySet(slow);
  // Including the FuzzyOutput into Fuzzy
  fuzzy->addFuzzyOutput(pwm);

  // Building FuzzyRule "IF temp = cold THEN pwm = fast"
  // Instantiating a FuzzyRuleAntecedent objects
  FuzzyRuleAntecedent *iftempcold = new FuzzyRuleAntecedent();
  // Creating a FuzzyRuleAntecedent with just a single FuzzySet
  iftempcold->joinSingle(cold);
  // Instantiating a FuzzyRuleConsequent objects
  FuzzyRuleConsequent *thenpwmfast = new FuzzyRuleConsequent();
  // Including a FuzzySet to this FuzzyRuleConsequent
  thenpwmfast->addOutput(fast);
  // Instantiating a FuzzyRule objects
  FuzzyRule *fuzzyRule01 = new FuzzyRule(1, iftempcold, thenpwmfast);
  // Including the FuzzyRule into Fuzzy
  fuzzy->addFuzzyRule(fuzzyRule01);

  // Building FuzzyRule "IF temp = target THEN pwm = average"
  // Instantiating a FuzzyRuleAntecedent objects
  FuzzyRuleAntecedent *iftemptarget = new FuzzyRuleAntecedent();
  // Creating a FuzzyRuleAntecedent with just a single FuzzySet
  iftemptarget->joinSingle(target);
  // Instantiating a FuzzyRuleConsequent objects
  FuzzyRuleConsequent *thenpwmAverage = new FuzzyRuleConsequent();
  // Including a FuzzySet to this FuzzyRuleConsequent
  thenpwmAverage->addOutput(average);
  // Instantiating a FuzzyRule objects
  FuzzyRule *fuzzyRule02 = new FuzzyRule(2, iftemptarget, thenpwmAverage);
  // Including the FuzzyRule into Fuzzy
  fuzzy->addFuzzyRule(fuzzyRule02);

  // Building FuzzyRule "IF temp = hot THEN pwm = high"
  // Instantiating a FuzzyRuleAntecedent objects
  FuzzyRuleAntecedent *iftemphot = new FuzzyRuleAntecedent();
  // Creating a FuzzyRuleAntecedent with just a single FuzzySet
  iftemphot->joinSingle(hot);
  // Instantiating a FuzzyRuleConsequent objects
  FuzzyRuleConsequent *thenpwmslow = new FuzzyRuleConsequent();
  // Including a FuzzySet to this FuzzyRuleConsequent
  thenpwmslow->addOutput(slow);
  // Instantiating a FuzzyRule objects
  FuzzyRule *fuzzyRule03 = new FuzzyRule(3, iftemphot, thenpwmslow);
  // Including the FuzzyRule into Fuzzy
  fuzzy->addFuzzyRule(fuzzyRule03);
}

void loop()
{
  // Getting a random value
  // print a random number from 10.00 to 60.00
  randNumber = random(1000, 6000);
  input = (float)randNumber / 100.00;
  //int input = random(10.0, 60.0);
  // Printing something
   int Volt = analogRead(sensorPin);
  temperaturaActual = Volt * 250.0 / 1023.0;
  Serial.println("\n\n\nEntrance: ");
  Serial.print("\t\t\ttemp: ");
  Serial.println(temperaturaActual);
  // Set the random value as an input
  fuzzy->setInput(1, temperaturaActual);
  // Running the Fuzzification
  fuzzy->fuzzify();
  // Running the Defuzzification
  float output = fuzzy->defuzzify(1);
  // Printing something
  Serial.println("Result: ");
  Serial.print("\t\t\tpwm: ");
  Serial.println(round(output));
  analogWrite(ledPin,output);
  // wait 2 seconds
  delay(2000);
}
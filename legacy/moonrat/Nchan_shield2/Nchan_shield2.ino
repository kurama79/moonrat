//  Moonrat Control Code
//  Copyright (C) 2021 Robert L. Read and Halimat Farayola
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Note: When used with a 12V power supply, this code currently "overshoots"
// the target temperature due to the time it take for heat to flow from the
// heating element into the chamber and the sensor.
// Although we could solve this problem in a number of ways, the most versatiCompule
// would appear to be to use a PID controller where (without the integrative component.)
// This allows the rate of heating to be a negative feedback.

// TODO: This problem has two control problems that are quite distinct, and should
// be treated so to avoid overshoot. The first is a "warm up" phase which should
// be done at full-blast until we get within a few degrees of the target, then
// at some proportional power to prevent overshoot. The second phase
// should use the PID controller to try to hold the temperature as close
// to the target as possible.

#define NCHAN_SHIELD 1

#define LOG_VERBOSE 5
#define LOG_DEBUG 4
#define LOG_WARNING 3
#define LOG_MAJOR 2
#define LOG_ERROR 1
#define LOG_PANIC 0
#define LOG_LEVEL 5

// These are Menu items
#define TEMPERATURE_M 0
#define GRAPH_1_M 1
#define SET_TEMP_M 2
#define STOP_M 3

#include <PID_v1.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_BusIO_Register.h>
#include <stdio.h>
#include <math.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <splash.h>
// EEPROM for arc32 - Version: Latest
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
// This is a great library, but it does not work with this hardware
// #include <DailyStruggleButton.h>



// For our temperature sensor

// This hardware uses a "onewire" digital temperature sensor
#ifdef NCHAN_SHIELD

// The transitor drains through
// The display at the "bottom" of the board. Count the header pins
// starting from the bottom:
// H0: Transistor PIN closest to Bottom (GND)
// H1: Unused
// H2: 12V
// H3: A0
// H4: +5V
// H5: GND

#define INPUT_PIN A0
#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(INPUT_PIN);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
#endif

//display variables
#define WIDTH 128
#define HEIGHT 64
#define SPLIT 16  // location of the graphing area (vertially, down from the top)
#define LEFT_MARGIN 9
#define LINE_HEIGHT 9
#define OLED_RESET 4
Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, OLED_RESET);

//pin variables
#define HEAT_PIN 9  // This needs to be a PWM pin!
#define BTN_SELECT 5
#define BTN_UP 6
#define BTN_DOWN 7

#define BZR_PIN 9

//temperature variables
#define TEMP_PIN A0  //This is the Arduino Pin that will read the sensor output
int sensorInput;     //The variable we will use to store the sensor input

#define MAX_TEMPERATURE_C 42.0

// #define USE_DEBUGGING
// #define USE_DEBUGGING 1
// #define USE_LOW_TEMP 1
#ifdef USE_LOW_TEMP
float targetTemperatureC = 30.0;  // Celcius
#else
float targetTemperatureC = 35.0;  // Celcius
#endif

// Variables to represent the state of the machine
// BASIC_STATE (this is different from the GUI State)
#define PREPARING 0
#define INCUBATING 1
#define FINISHED 2
int basic_state = INCUBATING;

// When Incubating, when are in two modes: warm-up phase or balance phase.
// The warm-up phase occurs when you we are first turned on or badly too cool.
// the balance phase is when we want slow stable mainteance of the target temperature.
// I am aribritraily defining warm-up phase as 4C below target phase.
#define WARM_UP_TEMP_DIFF_C 4.0
int warm_up_phase = true;

// This is a change to use PWM on our transistor, and further more
// to use a PID controller to set this parameter.
//Define Variables we'll be connecting to
// #define USE_PID_AND_PWM 1
double Setpoint = 0.0,
       Input = 0.0,
       Output = 0.0;

// Think we can create a different set of factors for the startup period.
// Basically we will have zero integration, a high Kd, and a high Kp.
double SLOW_DOWN_FACTOR = .05;

// Once we get within 4 degrees of the target temp, we will switch from the
// Startup parameters to the normal parameters.
// These are specifically stable for 12V power supply near the temperature range.
//Specify the links and initial tuninng parameters

double SKp = 10.0 / SLOW_DOWN_FACTOR, SKi = 0.0 / SLOW_DOWN_FACTOR, SKd = 1.0 / SLOW_DOWN_FACTOR;

// NOTE: These prameters seem to work pretty well once the stable temperature is
// reached, but overshoot badly if you start, so we really need a two phase solution!!
// double Kp = 7.0 / SLOW_DOWN_FACTOR, Ki = 3.0 / SLOW_DOWN_FACTOR, Kd = 2.0 / SLOW_DOWN_FACTOR;
double Kp = 7.0 / SLOW_DOWN_FACTOR, Ki = 3.0 / SLOW_DOWN_FACTOR, Kd = 2.0 / SLOW_DOWN_FACTOR;



PID* myPID = 0;

// This is the "Control Variable". It's range is 0-255, and it
// is the a PWM value for our heating circuit.
double target_duty_cycle = 0.0;

int rawTemp = 0;
float temperatureC;  //The variable we will use to store temperature in degrees.
bool heating = false;

// This is switched ONLY when turning heater on or off.
bool currently_heating = false;
uint32_t time_incubation_started_ms = 0;
uint32_t time_heater_turned_on_ms = 0;
uint32_t time_spent_heating_ms = 0;
uint32_t time_spent_incubating_ms = 0;
uint32_t time_of_last_entry = 0;


// This is ten minutes, this should give us more
// than 48 hours. We record data in the eprom at this rate
// once the begin is done.
unsigned long BASE_DATA_RECORD_PERIOD = 600000;

// For DEBUGGING, we may use a faster rate, of every 10 seconds
#ifdef USE_DEBUGGING
unsigned long DATA_RECORD_PERIOD = (BASE_DATA_RECORD_PERIOD / 60);
#else
unsigned long DATA_RECORD_PERIOD = BASE_DATA_RECORD_PERIOD;
#endif


//time variables
#define FIVE_MINUTES 300000

#define FORTYEIGHT_HOURS 172860000  //forty eight hours
#define TICK_LENGTH 125
//graph variables
int graphTimeLength = 24;  //2 hours long bexause plotting every 5 mins
/*graphTimeLength2=511;*/

int center = SPLIT + ((64 - SPLIT) / 2);  // (of ex axis?)
int graphwidth = 128 - LEFT_MARGIN;
int xaxisleft = LEFT_MARGIN * 2;
int xaxismid = LEFT_MARGIN + (graphwidth / 2) - 4;
int xaxisright = 128 - 24;



//menu variables
int menuSelection = 0;
bool inMainMenu = true;
bool up = false;
bool down = false;
bool sel = false;

// for debouncing
bool up_pressed = false;
bool dn_pressed = false;
bool sel_pressed = false;

const int BAUD_RATE = 19200;

//eeprom variables
// NOTE: I treat the EEPROM as 16-bit words.
#define TARGET_TEMP_ADDRESS 511
// #define TARGET_TEMP_ADDRESS 211
// Because we keep the "INDEX" at location, we chave to be careful
// about our accounting and our meaning.
// TODO: This is not implementing a full ring buffer!
// INDEX is the number of samples we have at any point in time.
// Our functions to read and write must remove the 0-indexed element
// in which a sample is not stored.
#define INDEX_ADDRESS 0  //location of index tracker in EEPROM
#define MAX_SAMPLES 509  //maximum number of samples that we can store in EEPROM

bool incubating = true;

int b = 1;  //initialize serial print index

void enter_warm_up_phase() {
  // These are the "balance" points
  //  myPID = new PID(&Input, &Output, &Setpoint, SKp, SKi, SKd, DIRECT);
  //  myPID = new PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
  myPID->SetTunings(SKp, SKi, SKd);
  //  myPID->SetOutputLimits(0, 255);
  //  myPID->SetMode(AUTOMATIC);
  warm_up_phase = true;
  Serial.println(F("Entered Warmup Phase!"));
}
void leave_warm_up_phase() {
  // These are the "balance" points
  //  myPID = new PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
  myPID->SetTunings(Kp, Ki, Kd);
  //  myPID->SetOutputLimits(0, 255);
  //  myPID->SetMode(AUTOMATIC);
  warm_up_phase = false;
  Serial.println(F("Left Warmup Phase!"));
}

//gets the contents of the EEPROM at each indec
float readIndex(int index) {
  if (index > MAX_SAMPLES) {
    return -1;
  }
  uint16_t bitData = rom_read16((index + 1) * 2);
  return sixteenToFloat(bitData);
}

// DISPLAY FUNCTIONS --------------------------------------------------
//dispays the current temperature i.e. when menu option 'temparature' is selected
void showCurStatus(float temp, float duty_factor) {
  display.clearDisplay();  //removes current plots
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(F("T (C):"));
  display.println(temp);
  display.print(F("F (%):"));
  display.println(duty_factor * 100);
  display.display();
}

void showSetTempMenu(float target) {
  display.clearDisplay();  //removes current plots
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("Up (U) to increase,"));
  display.println(F("Dn (D) to decrease,"));
  display.print(F("T (C):"));
  display.println(target);
  display.display();
}

// here we dump to the serial port for debugging and checking
void dumpData() {
  Serial.println(F("DATA DUMP:"));
  uint16_t num_samples = rom_read16(INDEX_ADDRESS);
  Serial.print(F("# samples: "));
  Serial.println(num_samples);
  if (num_samples > MAX_SAMPLES) num_samples = MAX_SAMPLES;
  for (int i = 0; i < num_samples; i++) {
    float currenttemp = readIndex(i);
    Serial.print(i);
    Serial.print(F(" "));
    Serial.println(currenttemp);
  }
  Serial.println(F("Done with output!"));
}

/*
   Shows a graph of the temperature data over a period of time
*/

void showGraph(int eeindex) {

  //  int graphMaxTemp = targetTemperatureC + 2; //initializing max temp
  //  int graphMinTemp = targetTemperatureC - 2; //initializing min temp
  //  int graphMaxTemp1 = targetTemperatureC + 20; //initializing max temp
  //  int graphMinTemp1 = targetTemperatureC - 20; //initializing min temp
  //  int maxtarget = targetTemperatureC + 2; //max range for target temp
  //  int mintarget = targetTemperatureC - 2; //min range for target temp
  //TODO find out have to shift x axis values
  //Graph

  int num_samples_drawn = min(graphwidth, eeindex);
  // I guess these are supposed to be in hours...
  float timeleft = 0;                                                                 //(xaxisright-xaxisleft)*FIVE_MINUTES;
  float timemid = (((float)(num_samples_drawn) / 2) * DATA_RECORD_PERIOD) / 3600000;  //(xaxisright-xaxismid)*FIVE_MINUTES;
  float timeright = ((float)(num_samples_drawn)*DATA_RECORD_PERIOD) / 3600000;        //xaxisright*FIVE_MINUTES;

  int diff1;
  int diff2;

  // index begins at the end minus graph width
  int startIndex = (eeindex > graphwidth) ? eeindex - graphwidth : 0;
  //plots 119 points fron the EEPROM Contents at a period of 5 minutes
  // This logic should render the last 119 samples, I suppose
  float maxTempToGraph = -1.0;
  float minTempToGraph = 5000.0;
  for (int i = startIndex; i < eeindex; i++) {
    float currentTemp = readIndex(i);
    minTempToGraph = min(minTempToGraph, currentTemp);
    maxTempToGraph = max(maxTempToGraph, currentTemp);
  }


  int graphMaxTemp = ceil(maxTempToGraph);
  int graphMinTemp = floor(minTempToGraph);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(F("# sam:"));
  display.println(eeindex);

  display.setTextSize(1);
  display.setCursor(1, SPLIT);
  display.println(graphMaxTemp);
  display.setCursor(1, 56);
  display.println(graphMinTemp);
  display.setCursor(LEFT_MARGIN * 2, 56);
  display.println(timeleft);
  display.setCursor(xaxismid, 56);
  display.println(timemid);
  display.setCursor(xaxisright, 56);
  display.println(timeright);

  // Pixels per degree
  float scale = (64.0 - SPLIT) / (float)(graphMaxTemp - graphMinTemp);  //creates scale y axis pixels

  float meanTemp = (((float)graphMaxTemp + (float)graphMinTemp) / 2.0);
  float middle = meanTemp - (float)graphMinTemp;
  int j = 0;
  for (int i = startIndex; i < eeindex; i++) {
    float currentTemp = readIndex(i);
    float ypos = (currentTemp - graphMinTemp);

    int x = LEFT_MARGIN + j;
    int y = (64 - (scale * ypos));
    int midy = (64 - (scale * middle));
    display.drawPixel(x, y, SSD1306_WHITE);  //plots temparature
    display.drawPixel(x, midy, SSD1306_WHITE);
    j++;
  }

  display.display();
}

/* Displays the menu.

   Menu Options:
   - Show Temperature : switches to temperature display mode
   - Show Graph : switches to graph dispay mode
   - Start/Stop : starts or stops data collection
   - Set Target : sets the target incubation temperature
   - Export : send temperature data through serial port to a pc
   - Reset : resets temperature data
*/
void showMenu() {
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  //print title
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print(F("Menu"));
  display.setTextSize(1);
  //displays time since incubation started
  display.setCursor(56, 7);

  // It's really useful to see the current temp at all times...
  double curTemp = read_temp();
  display.print(curTemp);
  display.print(" ");
  char currentTime[80];
  // String currentTime =
  getTimeString(currentTime);
  // chop off the seconds...
  currentTime[5] = '\0';
  display.println(currentTime);
  //menu option for current temperature
  display.setTextSize(1);
  display.setCursor(LEFT_MARGIN, SPLIT);
  //menu option for current temperature
  display.println(F("Temperature"));
  display.setCursor(LEFT_MARGIN, SPLIT + LINE_HEIGHT);
  //menu option for detailed graph
  display.println(F("Graph 1"));
  display.setCursor(LEFT_MARGIN, SPLIT + 2 * LINE_HEIGHT);

  //menu option for graph over 48 hours
  //  display.println(F("Graph 2"));
  //  display.setCursor(LEFT_MARGIN, SPLIT + 3 * LINE_HEIGHT);
  //menu option for entering the target temparature. Entered by clicking the up/down buttons
  display.println(F("Set Temperature"));
  display.setCursor(LEFT_MARGIN, SPLIT + 3 * LINE_HEIGHT);
  if (incubating) {
    display.println(F("Stop"));
  } else {
    display.println(F("Start"));
  }

  //print cursor
  display.setCursor(0, SPLIT + menuSelection * LINE_HEIGHT);
  display.fillCircle(3, SPLIT + menuSelection * LINE_HEIGHT + 3, 3, SSD1306_WHITE);

  display.display();
}

// UTILITY FUNCTIONS --------------------------------------------------
void incubationON() {
  if (incubating) return;
  time_incubation_started_ms = millis();
}
void incubationOFF() {
  if (!incubating) return;
  uint32_t time_now = millis();
  time_spent_incubating_ms = time_now - time_incubation_started_ms;
}
uint32_t time_incubating() {
  return (incubating) ? millis() - time_incubation_started_ms : time_spent_incubating_ms;
}

uint32_t time_of_last_PWM;
int current_PWM;
double weighted_voltage_ms = 0.0;


float duty_factor() {
#ifdef USE_PID_AND_PWM
  return (double)(((millis() - time_of_last_PWM) * current_PWM / 255.0) + weighted_voltage_ms) / (double)time_incubating();
#else
  return (float)time_heating() / (float)time_incubating();
#endif
}

uint32_t time_heating() {
  return (currently_heating) ? time_spent_heating_ms + (millis() - time_heater_turned_on_ms) : time_spent_heating_ms;
}

//turn the heating pad on
void heatOFF() {
  if (currently_heating) {
    //  digitalWrite(HEAT_PIN, HIGH);
#ifdef NCHAN_SHIELD
    analogWrite(HEAT_PIN, 0);
    // digitalWrite(HEAT_PIN, LOW);
#else
    Serial.println(F("not implemented heatsOFF"));
#endif
    if (LOG_LEVEL >= LOG_DEBUG) {
      Serial.println(F("HEAT OFF!"));
    }
    currently_heating = false;
    uint32_t time_now = millis();
    time_spent_heating_ms += time_now - time_heater_turned_on_ms;
  }
}
void heatON() {
  if (!currently_heating) {
    //  digitalWrite(HEAT_PIN, LOW);
#ifdef NCHAN_SHIELD
    analogWrite(HEAT_PIN, 255);
    Serial.println(F("setting HEAT_PIN 255"));
    //  digitalWrite(HEAT_PIN, HIGH);
#else
    Serial.println(F("not implemented heatsOFF"));
#endif
    if (LOG_LEVEL >= LOG_DEBUG) {
      Serial.println(F("HEAT ON!"));
    }
    time_heater_turned_on_ms = millis();
    currently_heating = true;
  }
}
// TODO: I would like to have a total measure of Joules here...
// that requires us to keep track of the last time we changed
// the pwn, track the pwm, and assume linearity.

void setHeatPWM(double intended_df) {
  if (intended_df > 255.0) {
    Serial.println(F("excessive duty_factor: "));
    Serial.println(intended_df);
    intended_df = 255.0;
  }
  if (intended_df < 0.0) {
    Serial.println(F("negative duty_factor: "));
    Serial.println(intended_df);
    intended_df = 0.0;
  }
  uint32_t tm = millis();
  // _voltage_ms += ((double) current_PWM / 255.0)* (tm - time_of_last_PWM);
  const int pwm = (int)intended_df;
  analogWrite(HEAT_PIN, pwm);

  time_of_last_PWM = tm;
  current_PWM = pwm;
  if (LOG_LEVEL >= LOG_MAJOR) {
    Serial.print(F("Set Heat PWM :"));
    Serial.println(pwm);
    Serial.print(F("Weighted Duty Factor :"));
    Serial.println(duty_factor());
  }
}

void getTimeString(char* buff) {
  uint32_t mils = time_incubating();
  uint8_t hours = mils / 3600000;
  mils = mils % 3600000;
  uint8_t mins = mils / 60000;
  mils = mils % 60000;
  uint8_t secs = mils / 1000;
  mils = mils % 1000;

  sprintf(buff, "%02d:%02d:%02d", hours, mins, secs);
}

//creates a whole number
uint16_t floatToSixteen(float flt) {
  uint16_t out = round(flt * 100);
  return out;
}
//creates a decimal
float sixteenToFloat(uint16_t sixteen) {
  float flt = sixteen / 100.0;
  return flt;
}

// EEPROM FUNCTIONS --------------------------------------------------

/*
   Writes 16 bits into EEPROM using big-endian respresentation. Will be called to enter temp at a specific address every 5 mibutes
*/
void rom_write16(uint16_t address, uint16_t data) {
  EEPROM.update(address, (data & 0xFF00) >> 8);
  EEPROM.update(address + 1, data & 0x00FF);
}

/*
   Reads 16 bits from EEPROM using big-endian respresentation. Will be called in readindex to get temp at certain index in order to plot graph
*/
uint16_t rom_read16(uint16_t address) {
  uint16_t data = EEPROM.read(address) << 8;
  data += EEPROM.read(address + 1);
  return data;
}
/*
   Marks the EEPROM as empty by clearing the first address
   Note: Data is not actually cleared from other addresses
*/
void rom_reset() {
  rom_write16(INDEX_ADDRESS * 2, 0);
}

/*
   Writes a new entry into the next spot in EEPROM
   returns false if the list is full
*/
bool writeNewEntry(float data) {
  uint16_t bitData = floatToSixteen(data);
  uint16_t index = rom_read16(INDEX_ADDRESS * 2);
  // TODO: this needs to be turned into a ring buffer
  if (index >= MAX_SAMPLES) {
    return false;
  }
  rom_write16(2 * (index + 1), bitData);
  index = index + 1;
  rom_write16(INDEX_ADDRESS * 2, index);
  return true;
}

// Note: Since we desire to allow the target temperature to be half values,
// yet are using an integer, we will double the value on the way in and
// halve it on the way out.
float getTargetTemp() {
  uint16_t targetTempC = rom_read16(TARGET_TEMP_ADDRESS * 2);
  return targetTempC / 2.0;
}
void setTargetTemp(float temp) {
  uint16_t temp_i = (uint16_t)temp * 2;
  rom_write16(TARGET_TEMP_ADDRESS * 2, temp_i);
}

// SETUP FUNCTIONS --------------------------------------------------

/*
   Starts interrupts
   Timer1 runs at 8 Hz
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);  //Start the Serial Port at 9600 baud (default)
  delay(500);

  //local_ptr_to_serial = &Serial;

  //  while (! Serial); // Wait untilSerial is ready
  Serial.println(F("Enter u, d, s"));

  //createarray();

  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_SELECT, INPUT);

  pinMode(HEAT_PIN, OUTPUT);
  // heatsON();

  // rom_reset();

  //  startInterrupts();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed -- This is an internal error."));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

#ifdef NCHAN_SHIELD
  pinMode(INPUT_PIN, INPUT);
  pinMode(A0, INPUT_PULLUP);
#endif
  myPID = new PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
  myPID->SetOutputLimits(0, 255);
  myPID->SetMode(AUTOMATIC);
  enter_warm_up_phase();

  incubationON();

  float storedTargetTemp = getTargetTemp();
  if (storedTargetTemp > MAX_TEMPERATURE_C) {
    setTargetTemp(MAX_TEMPERATURE_C);
    storedTargetTemp = MAX_TEMPERATURE_C;
  }
  if (storedTargetTemp < 25.0) {
    setTargetTemp(35.0);
    Serial.println(F("target temp to low, setting to 35!"));
    storedTargetTemp = 35.0;
  }
  targetTemperatureC = storedTargetTemp;
}
// Loop

/*
   Switch test program
*/
#ifndef NCHAN_SHIELD
double read_temp() {
  double sensorInput = analogRead(A0);                 //read the analog sensor and store it
  double temp = (double)sensorInput / (double)1024.0;  //find percentage of input reading
  double voltage = temp * 5.0;                         //multiply by 5V to get voltage
  Serial.print(F("voltage: "));
  Serial.println(voltage);
  double offsetVoltage = voltage - 0.5;     //Subtract the offset
  double degreesC = offsetVoltage * 100.0;  //Convert to degrees C
  return degreesC;
}
#endif

#ifdef NCHAN_SHIELD
double read_temp() {
  sensors.requestTemperatures();  // Send the command to get temperatures
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if (tempC != DEVICE_DISCONNECTED_C) {
    //    Serial.print(F("Temperature for the device 1 (index 0) is: "));
    //   Serial.println(tempC);
  } else {
    Serial.println(F("Error: Could not read temperature data"));
  }
  return tempC;
}
#endif


//main
#define PERIOD_TO_CHECK_TEMP_MS 5000
uint32_t last_temp_check_ms = 0;
bool returnToMain = false;
#define LOOP_PERIOD 250
#define BUTTON_POLL_PERIOD 0
uint32_t last_loop_run = 0;
void loop() {
  delay(BUTTON_POLL_PERIOD);

  bool sel_button = digitalRead(BTN_SELECT);
  bool up_button = digitalRead(BTN_UP);
  bool dn_button = digitalRead(BTN_DOWN);
  sel_pressed |= sel_button;
  up_pressed |= up_button;
  dn_pressed |= dn_button;
  // This is meant to catch the release of the button!
  sel = sel_pressed && !sel_button;
  down = dn_pressed && !dn_button;
  up = up_pressed && !up_button;
  if (sel) sel_pressed = false;
  if (down) dn_pressed = false;
  if (up) up_pressed = false;

  // Serial.println("buttons:");
  //  Serial.print(dn_button);
  // Serial.print(F(" "));
  // Serial.print(up_button);
  // Serial.print(F(" "));
  // Serial.println(sel_button);

  //   Serial.println("presses:");
  //  Serial.print(dn_pressed);
  // Serial.print(F(" "));
  // Serial.print(up_pressed);
  // Serial.print(F(" "));
  // Serial.println(sel_pressed);

  // Serial.print(down);
  // Serial.print(F(" "));
  // Serial.print(up);
  // Serial.print(F(" "));
  // Serial.println(sel);

  uint32_t loop_start = millis();
  if (!((sel || up || down) || (loop_start - last_loop_run > LOOP_PERIOD))) {
    return;
  }


  // Test reading the buttons...
  //  if (LOG_LEVEL >= LOG_VERBOSE) {
  //    Serial.print(F("Incubating: "));
  //    Serial.println(incubating);
  //    Serial.print(F("Currently Heating: "));
  //    Serial.println(currently_heating);
  //  }

  boolean dumpdata = false;
  //read keyboard entries from the serial monitor
  char T;
  if (Serial.available()) {
    T = Serial.read();  //getting string input in varaible "T"

    Serial.print(F("T ="));
    Serial.println(T);
    up = (T == 'u');
    down = (T == 'd');
    sel = (T == 's');
    dumpdata = (T == 'x');
  } else {
    //   Serial.println(F("loop AAA"));
    // sel = digitalRead(BTN_SELECT);
    // up = digitalRead(BTN_UP);
    // down = digitalRead(BTN_DOWN);
  }

  if (dumpdata) {
    dumpData();
  }

  //  if (LOG_LEVEL >= LOG_DEBUG) {
  //    Serial.print(inMainMenu);
  //    Serial.print(menuSelection);
  //    Serial.print(F("-"));
  //    Serial.print(sel);
  //    Serial.print(up);
  //    Serial.print(down);
  //    Serial.println();
  //  }
  // I don't know what this is supposed to do...
  int multiple = 0;
  if (!inMainMenu && sel) {
    returnToMain = true;
    sel = false;
  }
  // Serial.println(F("loop ZZZ"));
  //controls menu selection
  if (inMainMenu) {
    //read buttons and menu
    if (up && menuSelection > 0) {
      menuSelection--;
      up = false;
    } else if (down && menuSelection < 3) {
      menuSelection++;
      down = false;
    } else if (sel) {
      inMainMenu = false;
      sel = false;
    }
    //   Serial.println(F("loop PPP 1"));
    showMenu();
    //   Serial.println(F("loop PPP 2"));
  } else {
    //    Serial.println(F("loop RRR"));
    switch (menuSelection) {
      case TEMPERATURE_M:
        showCurStatus(temperatureC, duty_factor());
        break;
      case GRAPH_1_M:
        {
          uint16_t index = rom_read16(INDEX_ADDRESS);
          showGraph(index);
        }
        break;
      case SET_TEMP_M:
        {
          // This is wrong; we whould be showing instructions
          showSetTempMenu(targetTemperatureC);
          if (up) {
            if (targetTemperatureC > MAX_TEMPERATURE_C) {
              targetTemperatureC = MAX_TEMPERATURE_C;
            }
            targetTemperatureC += 0.5;
            setTargetTemp(targetTemperatureC);
          }
          if (down) {
            targetTemperatureC -= 0.5;
            setTargetTemp(targetTemperatureC);
          }
        }
        break;
      case STOP_M:
        {
          Serial.println(F("Toggling Incubation!"));
          if (!incubating) {
            // In this case, we want to restart the EEPROM...
            rom_reset();
            Serial.println(F("EEPROM RESET!"));
          }
          incubating = !incubating;
          //          milliTime = 0;
          inMainMenu = true;
        }
        break;
    }
    //    Serial.println(F("loop MMM"));
    if (returnToMain) {
      returnToMain = false;
      inMainMenu = true;
      sel = false;
    }
    //    Serial.println(F("loop NNN"));
  }

  sel = false;
  up = false;
  down = false;
  //Serial.println(F("loop BBB"));
  // We have to process menu changes without delay to have
  // a good user experience; but reading the temperature can
  // be delayed.
  if (loop_start < (last_temp_check_ms + PERIOD_TO_CHECK_TEMP_MS))
    return;

  // otherwise we need to processe temperature stuff
  last_temp_check_ms = loop_start;

  temperatureC = read_temp();
  if (LOG_LEVEL >= LOG_DEBUG) {
    Serial.print(F("Temp (C): "));
    Serial.println(temperatureC);
    if (LOG_LEVEL >= LOG_VERBOSE) {
      Serial.print(F("Target Temp (C): "));
      Serial.println(targetTemperatureC);
      Serial.print(F("Time spent incubating (s):"));
      Serial.println(((float)time_incubating()) / 1000.0);
      Serial.print(F("Time spent heating (s):"));
      Serial.println(((float)time_heating()) / 1000.0);
    }
  }
  // Test warm_up_phase or not...
  if ((temperatureC < (targetTemperatureC - WARM_UP_TEMP_DIFF_C))
      && !warm_up_phase) {
    enter_warm_up_phase();
  }
  if ((temperatureC >= (targetTemperatureC - WARM_UP_TEMP_DIFF_C))
      && warm_up_phase) {
    leave_warm_up_phase();
  }

  int numEEPROM = 0;

  uint32_t time_now = millis();
  uint32_t time_since_last_entry = time_now - time_of_last_entry;

  // Here we write into the EEPROM for Data logging
#ifdef USE_DEBUGGING
  Serial.println(F("DEBUGGING MODE!"));
#endif

  if (time_since_last_entry > DATA_RECORD_PERIOD) {
    //  entryFlag = false;
    Serial.println(F("Writing New Entry"));
    writeNewEntry(temperatureC);
    time_of_last_entry = time_now;
  }

  // Here is the actual heat algorithm...
  int USE_PID;
#ifdef USE_PID_AND_PWM
  USE_PID = 1;
#else
  USE_PID = 0;
#endif

  if (USE_PID) {
    Setpoint = (double)targetTemperatureC;
    Input = temperatureC;
    if (LOG_LEVEL >= LOG_VERBOSE) {
      Serial.print(F("SetPoint"));
      Serial.println(Setpoint);
      Serial.print(F("Input:"));
      Serial.println(Input);
    }
    myPID->Compute();
    if (LOG_LEVEL >= LOG_VERBOSE) {
      Serial.print(F("Output:"));
      Serial.println(Output);
      Serial.println(target_duty_cycle);
    }
    target_duty_cycle = Output;
    setHeatPWM(target_duty_cycle);
  } else {

    //if temp below target turn heat on
    //if temp above target + gap turn heat off
    //if the incubating has started then start heating if the temperature is too low.
    // Note: This should probably protected as a "strategy".
    if (incubating) {
      if (temperatureC > (targetTemperatureC + 0.25)) {
        heatOFF();
      } else {
        if (temperatureC < (targetTemperatureC - 0.25)) {
          heatON();
        } else {  // no change
        }
      }
    } else {
      heatOFF();
    }
  }

  if (LOG_LEVEL >= LOG_DEBUG) {
    if (time_incubating() > 0) {
      float duty_f = ((float)time_heating()) / ((float)time_incubating());
      //      Serial.print(F("Duty Factor: "));
      //      Serial.println(duty_f);
      //      Serial.println(duty_factor());
      if (LOG_LEVEL >= LOG_VERBOSE) {
        Serial.println(F("Warm Up: "));
        Serial.println(warm_up_phase);
        Serial.println(time_heating());
        Serial.println(time_incubating());
      }
    }
  }

  /* // This code is ineffective right now; it does nothing! */
  /* //countery=countery+1; */
  /* //timerpulse=timerpulse+digitalRead(HEAT_PIN); */
  /* //start buzzing after 48 hours until the user stops incubating */
  /* int timern = ceil(milliTime / 1000); */
  /* // I think this buzzes every minute...but the logic of this is wrong, */
  /* // and I don't have a buzzer installed! */
  /* // I think we need a new state to represent incubation ended.. */
  /* if (incubating && (timern % 60 == 0)) { */
  /*   //Serial.println(float(timerpulse/countery)); */
  /* } */
}

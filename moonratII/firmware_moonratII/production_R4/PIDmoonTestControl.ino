
// LIBRARIES
#include <Wire.h>
// Display Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Control Libraries (PID)
#include <PID_v1.h>

//OLED VARIABLES
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// PID variables
float Kp = 5.5;
float Ki = 3.7;
float Kd = 0.0;
double setPoint; // Desired reference for the controller
double contolInput; // Sensor's information in voltage
double controlOutput; // Control's output signal

// I/0 VARIABLES
const int sensorPin = A0;
int HEATER_PWM = 10;
int buzzerPin = 9;
#define BUTTON_SELECT 5
#define BUTTON_UP 7
#define BUTTON_DOWN 6
float CurrentTemp;

// GRAPH VARIABLES
const int numPoints = 60;
float TempHistory[numPoints] = {0};
const int barWidth = SCREEN_WIDTH / numPoints;

// MENU VARIABLES
int selectedOption = 0;
int tempMaxOptions[] = {29, 35, 37, 41.5};
int tempMin = 0;
int tempMax = 0;
int totalOptions = 4; // Change this to the total number of options in the menu

// KALMAN FILTER VARIABLES
float Q = 1E-9;       // Process variance
float R = 1.12E-5;     // Reading variance
float Pc = 0.0;
float G = 0.0;
float P = 1.0;
float Xp = 0.0;
float Zp = 0.0;
float FilteredTemp = 0.0;

// TIMER VARIABLES
unsigned long timeNow = 0;
unsigned long timeLast = 0;
int startingHour = 0;
int seconds = 0;
int secondOld = 0;
int minutes = 0;
int hours = startingHour;

static const unsigned char PROGMEM image_data_Saraarray[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xe0, 0x03, 0xe0, 0x1e, 0x01, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x60, 0x1e, 0x01, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xfc, 0x38, 0xff, 0xc7, 0xc3, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0x18, 0xff, 0xcf, 0xe7, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0x88, 0xff, 0xcf, 0xe7, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0x88, 0xff, 0xcf, 0xe7, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xc8, 0xff, 0xcf, 0xe7, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xc8, 0xff, 0xcf, 0xe3, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0x88, 0xff, 0xcf, 0xe0, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0x88, 0xff, 0xcf, 0xe0, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0x18, 0xff, 0xc7, 0xe7, 0xfc, 0x3f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0x78, 0x38, 0xff, 0xc7, 0xe7, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x78, 0xff, 0xcf, 0xe7, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0x07, 0xf8, 0xff, 0xcf, 0xe7, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xfc, 0xff, 0x8f, 0xe7, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xfc, 0x7f, 0x8f, 0xe7, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xfe, 0x3e, 0x1f, 0xe7, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf0, 0x3f, 0xff, 0x00, 0x3f, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf0, 0x3f, 0xff, 0xc0, 0xff, 0x00, 0x03, 0x3f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xfe, 0x7f, 0xdf, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xfe, 0x7f, 0xcf, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xe0, 0x3f, 0x81, 0xfc, 0x07, 0x81, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xe0, 0x1f, 0x81, 0xfc, 0x07, 0x81, 0xf8, 0x0f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xe0, 0xff, 0x1f, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf0, 0x7f, 0x1f, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf0, 0x7f, 0x1f, 0xe3, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf0, 0x3f, 0x1f, 0xf1, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0x1f, 0x1f, 0xf1, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0x1f, 0x1f, 0xf9, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0x8f, 0x1f, 0xf8, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0xc7, 0x1f, 0xfc, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0xe3, 0x1f, 0xfc, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0xe3, 0x1f, 0xfc, 0x73, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0xf1, 0x1f, 0xfe, 0x23, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0xf8, 0x1f, 0xfe, 0x23, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0xfc, 0x1f, 0xff, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xf3, 0xfc, 0x1f, 0xff, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf8, 0x3f, 0xe3, 0xfe, 0x1f, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf0, 0x1f, 0x80, 0x7f, 0x1f, 0xff, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xf0, 0x3f, 0x80, 0xff, 0x9f, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Generate the controller object
PID moonPID(&contolInput, &controlOutput, &setPoint, Kp, Ki, Kd, DIRECT);

void setup() {
  pinMode(buzzerPin, OUTPUT);
  analogWrite(buzzerPin, 100);
  delay(500);
  analogWrite(buzzerPin,0);
  delay(100);
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  {
    Serial.println(F("Error while initializing OLED"));
    for (;;)
      ;
  }
  delay(100);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Display static text
  display.drawBitmap(0, 0, image_data_Saraarray, 128, 64, 1);
  display.display();
  delay(1500);
  display.clearDisplay();
  display.setCursor(10, 10);
  display.println(F("Version/Wokwi: V12"));
  display.display();
  delay(2500);
  display.clearDisplay();
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);

  // Control Initialization
  moonPID.SetMode(AUTOMATIC);
}

void loop() 
{
  displayMenu();
  checkButtons();
  setMaxTemp();
  timeLast = millis()/1000;
  while (true)
  {
    timeNow = millis()/1000; // the number of milliseconds that have passed since boot
    seconds = timeNow - timeLast;

    if(seconds - secondOld != 0)
    {
      int Volt = analogRead(sensorPin);
      CurrentTemp = ((Volt * (4500/ 1024.0)) - 500) / 10; //T6G Sensor
      // Kalman Filter
      Pc = P + Q;
      G = Pc/(Pc + R);
      P = (1-G) * Pc;
      Xp = FilteredTemp;
      Zp = Xp;
      FilteredTemp = G*(CurrentTemp-Zp)+Xp;
      

      for (int i = 0; i < numPoints - 1; i++)
      {
        TempHistory[i] = TempHistory[i + 1];
      }

      TempHistory[numPoints - 1] = FilteredTemp;

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 2);
      display.print("Temp:");
      display.print(FilteredTemp);
      display.println("C ");
      
      // Display temperature graph on the rest of the screen
      for (int i = 0; i < numPoints; i++) 
      {
        int x = i * barWidth;
        int barHeight = map(TempHistory[i], 0, tempMax, 0, 44);
        //display.fillRect(x, SCREEN_HEIGHT - barHeight, barWidth, barHeight-14, SSD1306_WHITE);
        display.fillRect(x, 54 - barHeight, barWidth, barHeight, SSD1306_WHITE);
        //display.fillRect(x, 32 , 1, 6, SSD1306_WHITE);
      }

      // Show the horizontal lines for maximum and minimum temperatures.
      int yMax = map(tempMax, 0, 45, 0, 14);
      int yMin = map(tempMin, 0, 45, 50, 0);
      //display.drawFastHLine(0, yMax, SCREEN_WIDTH, SSD1306_WHITE);
      display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
      //display.drawFastHLine(0, yMin, SCREEN_WIDTH, SSD1306_WHITE);
      display.drawLine(0, 54, SCREEN_WIDTH, 54, SSD1306_WHITE);

      // Show the maximum and minimum temperature label
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(84, 2); // Position adjustment for Max legend
      display.print("Max:");
      display.print(tempMax);
      display.print("C");
      display.setCursor(2, SCREEN_HEIGHT-8);
      display.print("Timer:");
      display.print(hours);
      display.print(":");
      display.print(minutes);
      display.print(":");
      display.print(seconds);
      display.setCursor(84, SCREEN_HEIGHT-8); // Position adjustment for Min legend
      display.print("Min:");
      display.print(tempMin);
      display.print("C");
      display.display();

      // Control set
      setPoint = tempMax*0.01;
      contolInput = FilteredTemp*0.01;
      moonPID.Compute();
      analogWrite(HEATER_PWM, map(controlOutput, 0, 5, 0, 255));

      // Serial to see information
      Serial.print("\nTemperatura actual: ");
      Serial.print(FilteredTemp);
      //Serial.print("Control Input: ");
      //Serial.print(controlInput);
      Serial.print("\nSalida de control: ");
      Serial.print(controlOutput);

      /*if (FilteredTemp > tempMax) {
      // digitalWrite(PWM_OUT, LOW);
      analogWrite(HEATER_PWM, 0);
      } 
      else {
      analogWrite(HEATER_PWM, 255);
      }*/
    }

    if (seconds == 60)
    {
      timeLast = timeNow;
      minutes = minutes + 1; 
    }

    if (minutes == 60)
    {
      minutes = 0;
      hours = hours + 1; 
    }
    
    secondOld = seconds;
  }

  displayMenu();
}

void displayMenu() 
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("SELECT SAMPLE"));
  display.setCursor(10, 12);
  display.println(F("Yeast and mold"));
  display.setCursor(10, 24);
  display.println(F("Coliform"));
  display.setCursor(10, 36);
  display.println(F("Env. listeria"));
  display.setCursor(10, 48);
  display.println(F("Salmonella Express"));
  

  int circleY = 15 + selectedOption * 12; // Position of filled circle
  display.fillCircle(5, circleY, 3, SSD1306_WHITE);
    display.display();
}

void checkButtons() 
{
  while (tempMax == 0){
  if (digitalRead(BUTTON_SELECT) == HIGH) {
    Serial.println(F("BUTTON SELECT PUSHED!"));
    tempMax = tempMaxOptions[selectedOption];
    delay(200); // Debouncing
  }

  if (digitalRead(BUTTON_UP) == HIGH) {
    Serial.println(F("BUTTON UP PUSHED!"));
    selectedOption = (selectedOption - 1 + totalOptions) % totalOptions;
    delay(200); // Debouncing
    displayMenu();
  }

  if (digitalRead(BUTTON_DOWN) == HIGH) {
    Serial.println(F("BUTTON DOWN PUSHED!"));
    selectedOption = (selectedOption + 1) % totalOptions;
    delay(200); // Debouncing
    displayMenu();
  }
  }
}

void setMaxTemp() 
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Target Temp.");
  display.setCursor(0, 16);
  display.print(tempMax);
  display.println("C");
  display.display();
  delay(1000);
}

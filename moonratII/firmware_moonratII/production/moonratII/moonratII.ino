//Wokwi version: V10 (https://wokwi.com/projects/390933163302682625)

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fuzzy.h>
#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int SENSOR_PIN = A0;
int HEATER_PWM = 10;
int BUZZER_PIN = 9;
float temperaturaActual;


const int numPoints = 60;
float temperaturaHistorial[numPoints] = {0};
const int barWidth = SCREEN_WIDTH / numPoints;

#define BUTTON_SELECT 5
#define BUTTON_UP 6
#define BUTTON_DOWN 7

int selectedOption = 0;
int tempMaxOptions[] = {23, 35, 37, 41.5};
int tempMin = 0;
int tempMax = 0;
int totalOptions = 4; // Cambia esto al número total de opciones en tu menú


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

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  analogWrite(BUZZER_PIN, 50);
  delay(500);
  analogWrite(BUZZER_PIN,0);
  delay(100);
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Error al iniciar el OLED"));
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
  display.println(F("Version/Wokwi: V10"));
  display.display();
  delay(2500);
  display.clearDisplay();
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
}

void loop() {
  displayMenu();
  checkButtons();
  configurarTemperaturaMaxima();
  
  while (true)
  {
    int Volt = analogRead(SENSOR_PIN);
    temperaturaActual = Volt * 250.0 / 1023.0;
    

    for (int i = 0; i < numPoints - 1; i++)
    {
      temperaturaHistorial[i] = temperaturaHistorial[i + 1];
    }

    temperaturaHistorial[numPoints - 1] = temperaturaActual;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(temperaturaActual);
    display.println(" C");
    
// Mostrar la gráfica de temperatura en el resto de la pantalla
  for (int i = 0; i < numPoints; i++) {
    int x = i * barWidth;
    int barHeight = map(temperaturaHistorial[i], 0, 45, 0, 50);
    display.fillRect(x, SCREEN_HEIGHT - barHeight, barWidth, barHeight-14, SSD1306_WHITE);
    //display.fillRect(x, 32 , 1, 6, SSD1306_WHITE);
  }

  // Mostrar las líneas horizontales para las temperaturas máxima y mínima
  int yMax = map(tempMax, 0, 45, 0, 14);
  int yMin = map(tempMin, 0, 45, 50, 0);
  display.drawFastHLine(0, yMax, SCREEN_WIDTH, SSD1306_WHITE);
  display.drawFastHLine(0, yMin, SCREEN_WIDTH, SSD1306_WHITE);

  // Mostrar la etiqueta de temperatura máxima y mínima
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(80, 2); // Ajuste de posición para la leyenda Max
  display.print("Max: ");
  display.print(tempMax);
  display.print("C");
  display.setCursor(80, SCREEN_HEIGHT-8); // Ajuste de posición para la leyenda Min
  display.print("Min: ");
  display.print(tempMin);
  display.print("C");
  display.display();
  if (temperaturaActual > tempMax) {
  // digitalWrite(PWM_OUT, LOW);
  analogWrite(HEATER_PWM, 0);
  } 
  else {
  analogWrite(HEATER_PWM, 255);
  }
  delay(1000);
}
  displayMenu();
}

void displayMenu() {
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
  

  int circleY = 15 + selectedOption * 12; // Posición del círculo relleno
  display.fillCircle(5, circleY, 3, SSD1306_WHITE);
    display.display();
}

void checkButtons() {
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

void configurarTemperaturaMaxima() {
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
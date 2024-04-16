/*
  Codigo de pruebas para prototipo
*/

// Librerias
#include <PID_v1.h>

// Generacion de variables
float valSensor; // Valor que se lee del sensor
int pinSensor = A1; // Pin del sensor
int pinActuador = 9; // Pin al actuador

// Variables del control
double setPoint; // Referencia del sistema
double input; // Lectura del sensor
double output; // Accion de control
float kp = 20.463; // Ganancia proporcional - para reduccion del error
float ki = 8.667; // Ganancia integral - para el seguimiento de referencia en estado estacionario
float kd = 0.0; // Ganancia derivativa - reduccion de armonicos (no es necesario para este sistema)
int pwm; // Señal de temperatura

// Generamos el objeto controlador
PID myPID(&input, &output, &setPoint, kp, ki, kd, DIRECT);
//PID myPID(&input, &output, &setPoint, kp, ki, kd, REVERSE);

void setup() {
  
  // Asignando los pines
  pinMode(pinSensor, INPUT);
  pinMode(pinActuador, OUTPUT);

  // Encendemos el PID
  myPID.SetMode(AUTOMATIC);

  // Iniciando el serial
  Serial.begin(9600);

}

void loop() {
 /* 
  // Definir el setPoint
  if (Serial.available() > 0)
  {
    setPoint = Serial.read();
    Serial.print("El SetPoint es: ");
    Serial.print(setPoint);
    Serial.print("\n");
  }
*/
  // Leer sensor
  valSensor = analogRead(pinSensor);



  // Ajustar los parametros para el control
  setPoint = 25;
  setPoint *= 0.02;
  Serial.print("Setpoint: ");
  Serial.print(setPoint);
  Serial.print('\n');

  input =  valSensor*5.0/1024.0;
  Serial.print("Input: ");
  Serial.print(input);
  Serial.print('\n');

  // Descomentar el que se necesite !!!
  valSensor = (valSensor * 250.0)/1023.0; // Para el sensor
  // valSensor = map(valSensor, 0, 1023, 0, 5); // Para el potenciometro

  Serial.print("Lectura del sensor: ");
  Serial.print(valSensor);
  Serial.print('\n');

  // Control
  myPID.Compute();
  pwm = map(output, 0, 5, 0, 255);

  // PWM - Depende del escalamiento del sensor
  // pwm = map(valSensor, 0, 5, 0, 255); // Para el caso del sensor
  // pwm = map(valSensor, 0, 50, 0, 255); // Con el sensor de temperatura

  Serial.print("Salida del PWM: ");
  Serial.print(pwm);
  Serial.print('\n');

  analogWrite(pinActuador, pwm);
  delay(2000);

}
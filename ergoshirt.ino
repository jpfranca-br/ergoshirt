// Blynk Device Info
#define BLYNK_TEMPLATE_ID "<FILL_IN>"
#define BLYNK_TEMPLATE_NAME "<FILL_IN>"
#define BLYNK_AUTH_TOKEN "<FILL_IN>"
#define BLYNK_PRINT Serial

// Debugging functionality
#define DEBUG // Comment to turn off debugging
#ifdef DEBUG
  #define PRINT(x) Serial.print(x)
#else
  #define PRINT(x)
#endif

// Include Libraries for WiFi and Blynk
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
// Set password to "" for open networks.
char ssid[] = "<FILL_IN>";
char pass[] = "<FILL_IN>";

// Timer settings
#define TIMER_PERIOD 500L // Period in milliseconds (500 ms)
#define SENDING_INTERVAL 4 // Number of periods before sending average readings to Blynk

// Number of sensors and actuators
#define NUMBER_OF_SENSORS 3

// Pin configuration for ESP32-C3 SuperMini
const int ANALOG_PIN[NUMBER_OF_SENSORS] = {0, 1, 2};
const int DIGITAL_PIN[NUMBER_OF_SENSORS] = {21, 20, 10};

// Blynk virtual pins
const int TERMINAL_PIN = V0;
const int SENDING_INTERVAL_PIN = V1;
const int TIMER_PERIOD_PIN = V2;
const int VIRTUAL_PIN_SENSOR[NUMBER_OF_SENSORS] = {V10, V20, V30};
const int VIRTUAL_PIN_ACTUATOR[NUMBER_OF_SENSORS] = {V11, V21, V31};
const int VIRTUAL_PIN_MAX[NUMBER_OF_SENSORS] = {V12, V22, V32};
const int VIRTUAL_PIN_MIN[NUMBER_OF_SENSORS] = {V13, V23, V33};

// Minimum gap between maximum and minimum values (hysteresis)
const int MIN_GAP = 100;

// Initialize variables
int sensor[NUMBER_OF_SENSORS];
int sensorAccumulator[NUMBER_OF_SENSORS];
int targetMax[NUMBER_OF_SENSORS] = {4095, 4095, 4095};
int targetMin[NUMBER_OF_SENSORS] = {2047, 2047, 2047};
int sendingInterval = SENDING_INTERVAL;
int timerPeriod = TIMER_PERIOD;
bool actuator[NUMBER_OF_SENSORS];
bool previousActuator;
int j = 0;
int timerId;

// Create Timer class
BlynkTimer timer;

// Blynk function to handle virtual pin writes
BLYNK_WRITE_DEFAULT() {
  int pinValue = param.asInt();
  int pinNumber = request.pin;
  
  // Update target max and min values from Blynk
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    if (VIRTUAL_PIN_MAX[i] == pinNumber) {
      targetMax[i] = pinValue;
      if (targetMax[i] - targetMin[i] < MIN_GAP) {
        targetMax[i] = min(targetMin[i] + MIN_GAP, 4095);
        Blynk.virtualWrite(VIRTUAL_PIN_MAX[i], targetMax[i]);
      }
      break;
    }
    if (VIRTUAL_PIN_MIN[i] == pinNumber) {
      targetMin[i] = pinValue;
      if (targetMax[i] - targetMin[i] < MIN_GAP) {
        targetMin[i] = max(targetMax[i] - MIN_GAP, 0);
        Blynk.virtualWrite(VIRTUAL_PIN_MIN[i], targetMin[i]);
      }
      break;
    }
  }
  
  // Update interval and period settings
  if (pinNumber == SENDING_INTERVAL_PIN) { 
    sendingInterval = pinValue; 
  } else if (pinNumber == TIMER_PERIOD_PIN) {
    timerPeriod = pinValue;
    timer.changeInterval(timerId, timerPeriod);
  }
  
  // Print received value to terminal
  terminal("Valor recebido no Pin " + String(pinNumber) + ": " + String(pinValue) + "\n");
}

// Function to print messages to Blynk terminal
void terminal(String message) {
  PRINT(message);
  Blynk.virtualWrite(TERMINAL_PIN, message);
}

// Function to read analog sensors
void readSensors() {
  PRINT(String(j * timerPeriod) + "ms - #" + String(j) + "/" + String(j) + " - ");
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    sensor[i] = analogRead(ANALOG_PIN[i]);
    sensorAccumulator[i] += sensor[i];
    PRINT(String(i) + ": " + String(sensor[i]) + ", ");
  }
  PRINT("\n");
}

// Function to check sensor values and update actuators
void checkInRange() {
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    previousActuator = actuator[i];
    actuator[i] = (sensor[i] > targetMax[i]) || (sensor[i] < targetMin[i]);
    if (previousActuator != actuator[i]) {
      Blynk.virtualWrite(VIRTUAL_PIN_ACTUATOR[i], actuator[i]);
      digitalWrite(DIGITAL_PIN[i], actuator[i]);
    }
  }
}

// Alternative function to check sensor values and update actuators
void checkInRange_alt() {
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    previousActuator = actuator[i];
    actuator[i] = (sensor[i] > targetMax[i]) || (sensor[i] < targetMin[i]);
    if (previousActuator != actuator[i]) {
      Blynk.virtualWrite(VIRTUAL_PIN_ACTUATOR[i], actuator[i]);
    }
  }
  
  bool compare = false;
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    compare = compare || actuator[i];
  }
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    digitalWrite(DIGITAL_PIN[i], compare);
  }
}

// Function to calculate average sensor readings and send to Blynk
void sendToBlynk() {
  if (j >= sendingInterval) {
    PRINT("\nEnviando para a Blynk após " + String(j) + " leituras de " + String(timerPeriod) + " ms\n");
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
      Blynk.virtualWrite(VIRTUAL_PIN_SENSOR[i], sensorAccumulator[i] / j);
      String message = String(sensorAccumulator[i] / j) + "(" + String(targetMin[i]) + "," + String(targetMax[i]) + ") | ";
      PRINT(message);
    }
    PRINT("\n\n");
    j = 0;
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) { sensorAccumulator[i] = 0; }
  }
}

// Function to check Blynk connection and reconnect if needed
void checkBlynk() {
  if (!Blynk.connected()) {
    PRINT("Blynk não conectado. Tentando reconectar...");
    Blynk.connect();
  }
}

// Main function to be called periodically by the timer
void mainFunction() {
  j++;
  readSensors();
  checkInRange_alt();
  checkBlynk();
  sendToBlynk();
}

// Setup function
void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);

  // Configure pins
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    pinMode(ANALOG_PIN[i], INPUT);
    pinMode(DIGITAL_PIN[i], OUTPUT);
  }

  // Connect to WiFi and Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Setup timer to call mainFunction periodically
  timerId = timer.setInterval(timerPeriod, mainFunction);

  // Sync initial values from Blynk and send default variables
  Blynk.run();
  Blynk.syncAll();
  terminal("Iniciando\n");
  Blynk.virtualWrite(V1, sendingInterval);
  Blynk.virtualWrite(V2, timerPeriod);
}

// Main loop
void loop() {
  Blynk.run();
  timer.run();  // Run BlynkTimer
}

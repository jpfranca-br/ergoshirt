// Blynk Device Info
#define BLYNK_TEMPLATE_ID "<FILL_IN>"
#define BLYNK_TEMPLATE_NAME "<FILL_IN>"
#define BLYNK_AUTH_TOKEN "<FILL_IN>"
#define BLYNK_PRINT Serial

// Include Libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
// Set password to "" for open networks.
char ssid[] = "<FILL_IN>";
char pass[] = "<FILL_IN>";

// Define analog input pins. Using ESP32 C3 SuperMini
// See pinout at https://www.sudo.is/docs/esphome/boards/esp32c3supermini/
#define TIMER_PERIOD 100L
#define SENDING_INTERVAL 20  // send to Blynk an average of the last X readings
#define NUMBER_OF_SENSORS 3

// Blynk Virtual Pin Definitions
#define TERMINAL_PIN V0
#define SENDING_INTERVAL_PIN V1
#define TIMER_PERIOD_PIN V2

#define VIRTUAL_PIN_SENSOR_0 V10
#define VIRTUAL_PIN_SENSOR_1 V20
#define VIRTUAL_PIN_SENSOR_2 V30

#define VIRTUAL_PIN_ACTUATOR_0 V11
#define VIRTUAL_PIN_ACTUATOR_1 V21
#define VIRTUAL_PIN_ACTUATOR_2 V31

#define VIRTUAL_PIN_MAX_0 V12
#define VIRTUAL_PIN_MAX_1 V22
#define VIRTUAL_PIN_MAX_2 V32

#define VIRTUAL_PIN_MIN_0 V13
#define VIRTUAL_PIN_MIN_1 V23
#define VIRTUAL_PIN_MIN_2 V33

#define MIN_GAP 100

int virtualPinMax[NUMBER_OF_SENSORS] = { VIRTUAL_PIN_MAX_0, VIRTUAL_PIN_MAX_1, VIRTUAL_PIN_MAX_2 };
int virtualPinMin[NUMBER_OF_SENSORS] = { VIRTUAL_PIN_MIN_0, VIRTUAL_PIN_MIN_1, VIRTUAL_PIN_MIN_2 };

// Analog Pin Definitions
#define ANALOG_PIN_0 0
#define ANALOG_PIN_1 1
#define ANALOG_PIN_2 2

// Create Constants
int analogPin[NUMBER_OF_SENSORS];
int virtualPinSensor[NUMBER_OF_SENSORS];
int virtualPinActuator[NUMBER_OF_SENSORS];

// Create Variables
int sensor[NUMBER_OF_SENSORS];
int sensorAccumulator[NUMBER_OF_SENSORS];
int targetMax[NUMBER_OF_SENSORS];
int targetMin[NUMBER_OF_SENSORS];
int sendingInterval = SENDING_INTERVAL;
int timerPeriod = TIMER_PERIOD;
bool actuator[NUMBER_OF_SENSORS];  // assuming one actuator per sensor
bool previousActuator;
int j;
int timerId;

// Create Timer
BlynkTimer timer;

// Read max and min values from Blynk
BLYNK_WRITE_DEFAULT() {
  int pinValue = param.asInt();
  int pinNumber = request.pin;
  // Find the index of the current pin in the virtualPinTarget array
  int pinIndex = -1;
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    if (virtualPinMax[i] == pinNumber) {
      targetMax[i] = pinValue;
      if (targetMax[i] - targetMin[i] < MIN_GAP) {
        targetMax[i] = min(targetMin[i] + MIN_GAP, 4095);
        Blynk.virtualWrite(virtualPinMax[i], targetMax[i]);
      }
      break;
    }
    if (virtualPinMin[i] == pinNumber) {
      targetMin[i] = pinValue;
      if (targetMax[i] - targetMin[i] < MIN_GAP) {
        targetMin[i] = max(targetMax[i] - MIN_GAP, 0);
        Blynk.virtualWrite(virtualPinMin[i], targetMin[i]);
      }
      break;
    }
  }
  if (pinNumber == SENDING_INTERVAL_PIN) { 
    sendingInterval = pinValue; 
  };
  if (pinNumber == TIMER_PERIOD_PIN) {
    timerPeriod = pinValue;
    timer.changeInterval(timerId, timerPeriod);
  };
  terminal("Valor recebido no Pin " + String(pinNumber) + ": " + String(pinValue) + "\n");
}

// Write to Blynk terminal
void terminal(String message) {
  Serial.print(message);
  Blynk.virtualWrite(TERMINAL_PIN, message);
}

// Read analog sensors
void readSensors() {
  // Read analog values from A0, A1, and A2
  // Serial.print("#" + String(j) + " - ");
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    sensor[i] = analogRead(analogPin[i]);
    sensorAccumulator[i] = sensor[i] + sensorAccumulator[i];
    // Serial.print(String(i) + ": " + String(sensor[i]) + ", ");
  }
  //Serial.println();
}

void checkInRange() {
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    previousActuator = actuator[i];
    actuator[i] = (sensor[i] > targetMax[i]) || (sensor[i] < targetMin[i]);
    if (previousActuator != actuator[i]) {
      Blynk.virtualWrite(virtualPinActuator[i], actuator[i]);
    }
  }
}

void sendToBlynk() {
  if (j >= sendingInterval) {
    //Serial.println();
    //Serial.println("Enviado para a Blynk após " + String(j) + " leituras de " + String(timerPeriod) + " ms");
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
      Blynk.virtualWrite(virtualPinSensor[i], sensorAccumulator[i] / j);
      String message = String(sensorAccumulator[i] / j) + "(" + String(targetMin[i]) + "," + String(targetMax[i]) + ") | " ;
      Serial.print(message);
    }
    Serial.print("\n");
    j = 0;
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) { sensorAccumulator[i] = 0; };
  }
}

//
void checkBlynk() {
  if (!Blynk.connected()) {
    Serial.println("Blynk is not connected. Attempting to reconnect...");
    Blynk.connect();
  }
}

//
void mainFunction() {
  j++;
  //Serial.print(String(j*timerPeriod)+"ms.");
  //Serial.print(".");
  readSensors();
  checkInRange();
  checkBlynk();
  sendToBlynk();
}

//
void setup() {
  // Initialize debug console and set baudrate
  Serial.begin(115200);

  // Initialize Analog Input pins
  analogPin[0] = ANALOG_PIN_0;
  analogPin[1] = ANALOG_PIN_1;
  analogPin[2] = ANALOG_PIN_2;

  // Configure Analog pins as input (optional as it's the default state)
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    pinMode(analogPin[i], INPUT);
  }

  // Initialize Blynk Virtual pins
  virtualPinSensor[0] = VIRTUAL_PIN_SENSOR_0;
  virtualPinSensor[1] = VIRTUAL_PIN_SENSOR_1;
  virtualPinSensor[2] = VIRTUAL_PIN_SENSOR_2;
  virtualPinActuator[0] = VIRTUAL_PIN_ACTUATOR_0;
  virtualPinActuator[1] = VIRTUAL_PIN_ACTUATOR_1;
  virtualPinActuator[2] = VIRTUAL_PIN_ACTUATOR_2;

  j = 0;

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Setup a non blocking timer function
  timerId = timer.setInterval(timerPeriod, mainFunction);

  Blynk.run();
  Blynk.syncAll();
  terminal("Iniciando\n");
  Blynk.virtualWrite(V1, sendingInterval);
  Blynk.virtualWrite(V2, timerPeriod);
}

//
void loop() {
  Blynk.run();
  timer.run();  // Initiates BlynkTimer
}

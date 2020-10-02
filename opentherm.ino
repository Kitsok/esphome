/*
  OpenTherm Communication Example Code
  By: Ihor Melnyk
  Date: January 19th, 2018

  Uses the OpenTherm library to get/set boiler status and water temperature
  Open serial monitor at 115200 baud to see output.

  Hardware Connections (OpenTherm Adapter (http://ihormelnyk.com/pages/OpenTherm) to Arduino/ESP8266):
  -OT1/OT2 = Boiler X1/X2
  -VCC = 5V or 3.3V
  -GND = GND
  -IN  = Arduino (3) / ESP8266 (5) Output Pin
  -OUT = Arduino (2) / ESP8266 (4) Input Pin

  Controller(Arduino/ESP8266) input pin should support interrupts.
  Arduino digital pins usable for interrupts: Uno, Nano, Mini: 2,3; Mega: 2, 3, 18, 19, 20, 21
  ESP8266: Interrupts may be attached to any GPIO pin except GPIO16,
  but since GPIO6-GPIO11 are typically used to interface with the flash memory ICs on most esp8266 modules, applying interrupts to these pins are likely to cause problems
*/


#include <Arduino.h>
#include <OpenTherm.h>
#include <ArduinoJson.h>
#include <Shell.h>


#define BOILER_POLL_TIME 900

float tout;
float tin;
float pressure;
float modulation;
unsigned char fcode;
bool heating_g;
bool heating_s;
bool dhw;
bool flame;
bool fault;

bool enableHotWater = true;
bool enableCooling = false;

int boilerTemp = 45;

unsigned long response;
bool rcvd_ok = true;

unsigned long boilerTS = 0;

const int inPin = 2; //4
const int outPin = 3; //5

OpenTherm ot(inPin, outPin);
StaticJsonDocument<200> doc;
JsonObject root = doc.to<JsonObject>();

void handleInterrupt() {
  ot.handleInterrupt();
}

void setup()
{
  Serial.begin(115200);

  ot.begin(handleInterrupt);
  delay(100);

  // Initialize shell
  shell_init(shell_reader, shell_writer, 0);

  // Add commands to the shell
  shell_register(command_test, PSTR("test"));

  // Get JSON of boiler
  shell_register(printBoilerData, PSTR("g"));

  // Set temperature
  shell_register(command_setTemp, PSTR("s"));

  // Ping
  shell_register(command_ping, PSTR("p"));
}

unsigned long time;

void loop()
{
  time = millis();

  // Poll CLI task
  shell_task();

  // Poll boiler
  if (time < boilerTS || (time - boilerTS) > BOILER_POLL_TIME ) {
    pollBoiler();
  }
}

// Ping command
int command_ping(int argc, char** argv)
{
  shell_printf("{\"status\": true, \"ts\": %u}", millis());
  return SHELL_RET_FAILURE;
}

// Set boiler desired temperature
int command_setTemp(int argc, char** argv)
{
  if (argc != 2) {
    shell_print("{\"status\": false}");
    return SHELL_RET_FAILURE;
  }

  int i = atoi(argv[1]);

  if (i < 1 || i > 85)
  {
    shell_print("{\"status\": false}");
    return SHELL_RET_FAILURE;
  }

  if (i < 41) {
    heating_s = false;
    boilerTemp = 39;
  }
  else
  {
    heating_s = true;
    boilerTemp = i;
  }

  shell_print("{\"status\": true}");
  return SHELL_RET_SUCCESS;
}

// Just a test command
int command_test(int argc, char** argv)
{
  int i;

  shell_println("-----------------------------------------------");
  shell_println("SHELL DEBUG / TEST UTILITY");
  shell_println("-----------------------------------------------");
  shell_printf("Time:%ul", time);
  shell_println("");
  shell_printf("Received %d arguments for test command\r\n", argc);

  // Print each argument with string lenghts
  for (i = 0; i < argc; i++)
  {
    // Print formatted text to terminal
    shell_printf("%d - \"%s\" [len:%d]\r\n", i, argv[i], strlen(argv[i]) );
  }

  return SHELL_RET_SUCCESS;
}


/**
   Function to read data from serial port
   Functions to read from physical media should use this prototype:
   int my_reader_function(char * data)
*/
int shell_reader(char * data)
{
  // Wrapper for Serial.read() method
  if (Serial.available()) {
    *data = Serial.read();
    return 1;
  }
  return 0;
}

/**
   Function to write data to serial port
   Functions to write to physical media should use this prototype:
   void my_writer_function(char data)
*/
void shell_writer(char data)
{
  // Wrapper for Serial.write() method
  Serial.write(data);
}

// Poll boiler - get all data and set temperature
void pollBoiler() {

  // Set/Get Boiler Status
  response = ot.setBoilerStatus(heating_s, enableHotWater, enableCooling);
  OpenThermResponseStatus responseStatus = ot.getLastResponseStatus();

  rcvd_ok = false;
  if (responseStatus == OpenThermResponseStatus::SUCCESS) {
    rcvd_ok = true;
    heating_g = ot.isCentralHeatingEnabled(response);
    dhw = ot.isHotWaterEnabled(response);
    flame = ot.isFlameOn(response);
    fault = ot.isFault(response);
  }

  // Get Boiler data
  if (rcvd_ok) {
    tout = ot.getBoilerTemperature();
    tin = ot.getRetTemperature();
    pressure = ot.getPressure();
    modulation = ot.getModulation();
    fcode = ot.getFault();

    // Set boier temperature
    ot.setBoilerTemperature(boilerTemp);
  }

  // Update TS
  boilerTS = millis();

}

/* Print JSON data of the boiler */
void printBoilerData() {
  root["status"] = true;
  root["Connected"] = rcvd_ok;
  root["TS"] = boilerTS;
  root["Tout"] = tout;
  root["Tin"] = tin;
  root["Modulation"] = modulation;
  root["Fault"] = fault;
  root["Setpoint"] = boilerTemp;
  root["Heating"] = heating_g;
  root["DHW"] = dhw;
  root["Flame"] = flame;

  String str;
  serializeJson(root, Serial);
}

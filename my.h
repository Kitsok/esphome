#include "esphome.h"
#include "OpenTherm.h"


#define BOILER_POLLING 900 // 900ms
#define POST_INTERVAL 20 // 9 seconds

float tout;
float mod;
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

const int inPin = 22; //4
const int outPin = 23; //5

OpenTherm ot(inPin, outPin);
  void handleInterrupt() {
     ot.handleInterrupt();
  }

class MyCustomSensor : public PollingComponent {
 public:
  Sensor *heatant_temp = new Sensor();
  Sensor *burner_modulation = new Sensor();

  // constructor
  // Set polling interval
  MyCustomSensor() : PollingComponent(BOILER_POLLING) { }

  void setup() override {
    // This will be called by App.setup()
    ot.begin(handleInterrupt);
    ESP_LOGD("custom", "Started OpenTherm");
  }
  void update() override {
     // This will be called every "update_interval" milliseconds.
    ticks ++;
    // Poll boiler each interval
    rcvd_ok = false;
    response = ot.setBoilerStatus(heating_s, enableHotWater, enableCooling);
    OpenThermResponseStatus responseStatus = ot.getLastResponseStatus();

    if (responseStatus == OpenThermResponseStatus::SUCCESS) {
      rcvd_ok = true;
      heating_g = ot.isCentralHeatingActive(response);
      dhw = ot.isHotWaterActive(response);
      flame = ot.isFlameOn(response);
      fault = ot.isFault(response);
      tout = ot.getBoilerTemperature();
      mod = ot.getModulation();

      ot.setBoilerTemperature(boilerTemp);
    }

    if (ticks == POST_INTERVAL) {
       heatant_temp->publish_state(tout);
       burner_modulation->publish_state(mod);
       ESP_LOGD("custom", "Tout:%f, Modulation:%f, rcvd_ok:%d", tout, mod, rcvd_ok);
       ticks = 0;
    }
  }

 private:
    int ticks = 0;
};

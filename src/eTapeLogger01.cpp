/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "d:/Luca_wd/ParticleWorkbench/eTapeLogger01/src/eTapeLogger01.ino"
#include "Particle.h"

void setup();
void loop();
#line 3 "d:/Luca_wd/ParticleWorkbench/eTapeLogger01/src/eTapeLogger01.ino"
#ifndef SYSTEM_VERSION_v620
SYSTEM_THREAD(ENABLED); // System thread defaults to on in 6.2.0 and later and this line is not required
#endif

SerialLogHandler logHandler;

// Sleep time between cycles (measurement period)
const std::chrono::minutes publishPeriod = 1min;

// Time to turn sleep OFF off before taking a measurement
const std::chrono::seconds sensorWarmup = 10s;

// Time until sleep is turned ON after taking a measurement
const std::chrono::seconds postDelay = 10s;

// The event name to publish with
const char *eventName = "eTape Log";
//const char *eventName2 = "Total Log";

const pin_t ETAPE_PIN = A0;     // your eTape on A0
const float VREF      = 3.3f;   // Boron ADC full-scale
const int   NUMSAMPLES = 20;   // Amount of samples taken for smoothing

//declaration of variables
FuelGauge fuel;

int   v = 0;                
float depth    = 0.0f;
float volts    = 0.0f;
float depthin  = 0.0f;
float batterySoc = 0.0f;
float batteryVolts;

//declaration of functions
void takeMeasurement();         // Take a reading
void publishMeasurement();      // Publish reading to SHEETS
void goToSleep();               // Go into sleep mode
void batt_settings();           // Configure PMIC

void setup() {
    
//begin serial connection
    Serial.begin(9600);
    waitFor(Serial.isConnected, 3000);
    Log.info("Starting eTape logger");
    batt_settings();
    
}

void loop() {
    
    // 1) Warmup period
    delay((uint32_t)(sensorWarmup / 1ms));              //Turn sleep OFF

    // 2) Take a measurement
    takeMeasurement();                              

    // 3) Publish the measurement
    publishMeasurement();

    // 4) Post-measurement delay
    delay((uint32_t)(postDelay / 1ms));

    // 5) Go to sleep until next cycle
    goToSleep();
}

void takeMeasurement() {
    
    long sum = 0;

    for (int i = 0; i < NUMSAMPLES; i++) {
        v = analogRead(ETAPE_PIN);
        sum += v;
        delay(50);
    }

    float smoothed = (float)sum / (float)NUMSAMPLES;

    volts = (smoothed / 4095.0f) * VREF;

    if (volts < 0.05f) {
        depth = 0.0f;
        depthin = 0.0f;
    }
    else if (volts >= 0.05f && volts < 0.1f) {
        depth = 0.762f;   // small depth placeholder
        depthin = 0.3f;
    }
    else {
        depth = (volts * 19.8f - 0.2271f) - 2.54f;
        depthin = depth / 2.54f;
    }

    // Update battery State of Charge(SoC)
    batterySoc = System.batteryCharge();
    batteryVolts = fuel.getVCell();
}

// prepares a JSON-ish array and publishes it
void publishMeasurement() {
   
    if (!Particle.connected()) {
        Particle.connect();
        // Wait up to 3 minutes for connection
        waitFor(Particle.connected, 3 * 60 * 1000);
    }

    if (Particle.connected()) {
        char sheetBuf[128];
        //char totalBuf[128];
        
        //print to SHEETS
        snprintf(sheetBuf, sizeof(sheetBuf), "[%.3f,%.3f,%.1f,%.2f]", volts, depth, batterySoc, batteryVolts);

        Particle.publish(eventName, sheetBuf, PRIVATE);
        
        //snprintf(totalBuf, sizeof(totalBuf), "[%.3f,%.3f,%.3f,%.1f,%.2f]", volts, depth, depthin, batterySoc, batteryVolts);

        //Particle.publish(eventName2, totalBuf, PRIVATE);
    }
}

void goToSleep() {

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
      .duration(publishPeriod)
      .network(NETWORK_INTERFACE_CELLULAR, SystemSleepNetworkFlag::INACTIVE_STANDBY);

    System.sleep(config);
    // On wake, we drop back into loop() and do the cycle again
}

void batt_settings() {
  
  SystemPowerConfiguration conf; 
  conf.powerSourceMaxCurrent(900)    // 5W / 5V = 1000mA. 900mA is the closest PMIC register setting.
      .powerSourceMinVoltage(3880)  
      .batteryChargeCurrent(500)
      .batteryChargeVoltage(4110);  
      
  System.setPowerConfiguration(conf);
}


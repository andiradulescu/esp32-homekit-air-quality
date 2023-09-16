#include <HomeSpan.h>
#include <SoftwareSerial.h>
#include "SerialCom.hpp"
#include "Types.hpp"
#include <Smoothed.h>

#define INTERVAL			 10	  // in seconds

bool				  airQualityAct = false;
particleSensorState_t state;
Smoothed<float>		  mySensor_air;

struct DEV_AirQualitySensor : Service::AirQualitySensor { // A standalone Air Quality sensor

	// An Air Quality Sensor is similar to a Temperature Sensor except that it supports a wide variety of measurements.
	// We will use three of them.  The first is required, the second two are optional.

	SpanCharacteristic *airQuality; // reference to the Air Quality Characteristic, which is an integer from 0 to 5
	SpanCharacteristic *pm25;
	SpanCharacteristic *airQualityActive;

	DEV_AirQualitySensor() : Service::AirQualitySensor() { // constructor() method

		airQuality		 = new Characteristic::AirQuality(1); // instantiate the Air Quality Characteristic and set initial value to 1
		pm25			 = new Characteristic::PM25Density(0);
		airQualityActive = new Characteristic::StatusActive(false);

		Serial.print("Configuring Air Quality Sensor"); // initialization message
		Serial.print("\n");

		SerialCom::setup();

		mySensor_air.begin(SMOOTHED_AVERAGE, 4); // SMOOTHED_AVERAGE, SMOOTHED_EXPONENTIAL options

	} // end constructor

	void loop() {

		if (pm25->timeVal() > INTERVAL * 1000) { // modify the Air Quality Characteristic every 5 seconds

			SerialCom::handleUart(state);

			if (state.valid) {

				if (!airQualityAct) {
					airQualityActive->setVal(true);
					airQualityAct = true;
				}

				mySensor_air.add(state.avgPM25);

				pm25->setVal(mySensor_air.get());

				int airQualityVal = 0;

				// Set Air Quality level based on PM2.5 value
				if (state.avgPM25 >= 150) {
					airQualityVal = 5;
				} else if (state.avgPM25 >= 55) {
					airQualityVal = 4;
				} else if (state.avgPM25 >= 35) {
					airQualityVal = 3;
				} else if (state.avgPM25 >= 12) {
					airQualityVal = 2;
				} else if (state.avgPM25 >= 0) {
					airQualityVal = 1;
				}
				airQuality->setVal(airQualityVal);
			}
		}

	} // loop
};

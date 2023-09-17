#include <HomeSpan.h>
#include <SoftwareSerial.h>
#include "SerialCom.hpp"
#include "Types.hpp"

Vindriktning vindriktning;

struct DEV_AirQualitySensor : Service::AirQualitySensor { // A standalone Air Quality sensor

	// An Air Quality Sensor is similar to a Temperature Sensor except that it supports a wide variety of measurements.
	// We will use three of them.  The first is required, the second two are optional.

	SpanCharacteristic *airQuality; // reference to the Air Quality Characteristic, which is an integer from 0 to 5
	SpanCharacteristic *pm25, *pm10;
	SpanCharacteristic *airQualityActive;

	DEV_AirQualitySensor() : Service::AirQualitySensor() { // constructor() method
		airQuality = new Characteristic::AirQuality(1); // instantiate the Air Quality Characteristic and set initial value to 1
		pm25 = new Characteristic::PM25Density(0);
		pm10 = new Characteristic::PM10Density(0);
		airQualityActive = new Characteristic::StatusActive(false);

		Serial.print("Configuring Air Quality Sensor"); // initialization message
		Serial.print("\n");

		SerialCom::setup();
	}

	void loop() {

		if (pm25->timeVal() > 1000) { // modify the Air Quality Characteristic every 1 second

			bool valid = SerialCom::readData(vindriktning);

			if (valid) {

				if (!airQualityActive->getVal()) {
					airQualityActive->setVal(true);
				}

				pm25->setVal(vindriktning.pm2_5);
				pm10->setVal(vindriktning.pm10);

				int airQualityVal = 0;

				// Set Air Quality level based on PM2.5 value
				if (vindriktning.pm2_5 >= 150) {
					airQualityVal = 5;
				} else if (vindriktning.pm2_5 >= 55) {
					airQualityVal = 4;
				} else if (vindriktning.pm2_5 >= 35) {
					airQualityVal = 3;
				} else if (vindriktning.pm2_5 >= 12) {
					airQualityVal = 2;
				} else if (vindriktning.pm2_5 >= 0) {
					airQualityVal = 1;
				}
				airQuality->setVal(airQualityVal);
			}
		}

	} // loop
};

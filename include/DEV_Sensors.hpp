#include <HomeSpan.h>
#include <SoftwareSerial.h>
#include "SerialCom.hpp"
#include "Types.hpp"

#ifdef CO2_SENSOR_ENABLED
#include <SensirionI2cScd4x.h>
#include <Wire.h>
#endif

Vindriktning vindriktning;

#ifdef CO2_SENSOR_ENABLED

static constexpr int16_t SCD4X_NO_ERROR = 0;

SensirionI2cScd4x sensor;
bool scd4x_initialized = false;
static char errorMessage[64];
static int16_t error;

void PrintUint64(uint64_t& value) {
    Serial.print("0x");
    Serial.print((uint32_t)(value >> 32), HEX);
    Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX);
}

struct DEV_CO2Sensor : Service::CarbonDioxideSensor { // A standalone CO2 sensor

	SpanCharacteristic *co2Detected;
	SpanCharacteristic *co2Level;
	SpanCharacteristic *co2PeakLevel;
	SpanCharacteristic *co2StatusActive;

	DEV_CO2Sensor() : Service::CarbonDioxideSensor() { // constructor() method

		co2Detected		= new Characteristic::CarbonDioxideDetected(false);
		co2Level		= new Characteristic::CarbonDioxideLevel(400);
		co2PeakLevel	= new Characteristic::CarbonDioxidePeakLevel(400);
		co2StatusActive = new Characteristic::StatusActive(false);

		co2StatusActive->setVal(false); // Set to false initially until sensor is initialized

    Wire.begin(PIN_SCD4X_SDA, PIN_SCD4X_SCL);
    sensor.begin(Wire, SCD40_I2C_ADDR_62);

    uint64_t serialNumber = 0;
    delay(30);
    // Ensure sensor is in clean state
    error = sensor.wakeUp();
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute wakeUp(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }
    error = sensor.stopPeriodicMeasurement();
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }
    error = sensor.reinit();
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute reinit(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }
    // Read out information about the sensor
    error = sensor.getSerialNumber(serialNumber);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    Serial.print("Serial number: ");
    PrintUint64(serialNumber);
    Serial.println();

    // Get sensor variant
    SCD4xSensorVariant sensorVariant;
    error = sensor.getSensorVariant(sensorVariant);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getSensorVariant(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Sensor variant: ");
        switch (sensorVariant) {
            case SCD4X_SENSOR_VARIANT_SCD40:
                Serial.println("SCD40");
                break;
            case SCD4X_SENSOR_VARIANT_SCD41:
                Serial.println("SCD41");
                break;
            default:
                Serial.println("Unknown");
                break;
        }
    }

    // Check if automatic self-calibration is enabled
    uint16_t ascEnabled = 0;
    error = sensor.getAutomaticSelfCalibrationEnabled(ascEnabled);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getAutomaticSelfCalibrationEnabled(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Automatic self-calibration enabled: ");
        Serial.println(ascEnabled ? "true" : "false");
    }

    // Check temperature offset
    float temperatureOffset = 0.0;
    error = sensor.getTemperatureOffset(temperatureOffset);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getTemperatureOffset(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Temperature offset: ");
        Serial.print(temperatureOffset);
        Serial.println(" °C");
    }

    // Check sensor altitude
    uint16_t sensorAltitude = 0;
    error = sensor.getSensorAltitude(sensorAltitude);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getSensorAltitude(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Sensor altitude: ");
        Serial.print(sensorAltitude);
        Serial.println("m");
    }

    // Check automatic self-calibration target
    uint16_t ascTarget = 0;
    error = sensor.getAutomaticSelfCalibrationTarget(ascTarget);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getAutomaticSelfCalibrationTarget(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Automatic self-calibration target: ");
        Serial.print(ascTarget);
        Serial.println(" ppm");
    }

    // Check automatic self-calibration initial period
    uint16_t ascInitialPeriod = 0;
    error = sensor.getAutomaticSelfCalibrationInitialPeriod(ascInitialPeriod);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getAutomaticSelfCalibrationInitialPeriod(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Automatic self-calibration initial period: ");
        Serial.print(ascInitialPeriod);
        Serial.println(" hours");
    }

    // Check automatic self-calibration standard period
    uint16_t ascStandardPeriod = 0;
    error = sensor.getAutomaticSelfCalibrationStandardPeriod(ascStandardPeriod);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute getAutomaticSelfCalibrationStandardPeriod(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Automatic self-calibration standard period: ");
        Serial.print(ascStandardPeriod);
        Serial.println(" hours");
    }

    // Perform self test
    uint16_t sensorStatus = 0;
    error = sensor.performSelfTest(sensorStatus);
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute performSelfTest(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    } else {
        Serial.print("Self test result: ");
        if (sensorStatus == 0) {
            Serial.println("OK - no malfunction detected");
        } else {
            Serial.print("ERROR - malfunction detected (status: ");
            Serial.print(sensorStatus);
            Serial.println(")");
        }
    }

    //
    // If temperature offset and/or sensor altitude compensation
    // is required, you should call the respective functions here.
    // Check out the header file for the function definitions.
    // Start periodic measurements (5sec interval)
    error = sensor.startPeriodicMeasurement();
    if (error != SCD4X_NO_ERROR) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }

		scd4x_initialized = true;
		co2StatusActive->setVal(true);
	}

	void loop() {
		if (!scd4x_initialized) {
			return;
		}

		if (co2Level->timeVal() > 5000) { // modify the CO2 Characteristic every 5 seconds
			bool dataReady = false;
			uint16_t co2Concentration = 0;
			float temperature = 0.0;
			float relativeHumidity = 0.0;

			error = sensor.getDataReadyStatus(dataReady);
			if (error != SCD4X_NO_ERROR) {
					Serial.print("Error trying to execute getDataReadyStatus(): ");
					errorToString(error, errorMessage, sizeof errorMessage);
					Serial.println(errorMessage);
					return;
			}
			if (!dataReady) {
				return;
			}

			//
			// If ambient pressure compenstation during measurement
			// is required, you should call the respective functions here.
			// Check out the header file for the function definition.
			error = sensor.readMeasurement(co2Concentration, temperature, relativeHumidity);
			if (error != SCD4X_NO_ERROR) {
					Serial.print("Error trying to execute readMeasurement(): ");
					errorToString(error, errorMessage, sizeof errorMessage);
					Serial.println(errorMessage);
					return;
			}

			//
			// Print results in physical units.
			//
			Serial.print("CO2 concentration [ppm]: ");
			Serial.print(co2Concentration);
			Serial.println();
			Serial.print("Temperature [°C]: ");
			Serial.print(temperature);
			Serial.println();
			Serial.print("Relative Humidity [RH]: ");
			Serial.print(relativeHumidity);
			Serial.println();

			if (co2Concentration >= 400) { // Valid CO2 reading
				// Update CO2 level
				co2Level->setVal(co2Concentration);

				// Update peak value if needed
				if (co2Concentration > co2PeakLevel->getVal()) {
					co2PeakLevel->setVal(co2Concentration);
				}

				// Trigger HomeKit sensor when concentration reaches threshold
				if (co2Concentration > 1000) {
					co2Detected->setVal(true);
				} else {
					co2Detected->setVal(false);
				}
			}

			// Reset peak level every 12 hours
			if (co2Level->timeVal() > 12 * 60 * 60 * 1000) {
				co2PeakLevel->setVal(400);
			}
		}
	}
};
#endif

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

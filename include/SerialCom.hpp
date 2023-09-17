/*********************************************************************************************\
 * IKEA VINDRIKTNING PM2.5 particle concentration sensor
 *
 * This sensor uses a subset of the PM1006K LED particle sensor
 * readData code was taken from xsns_91_vindriktning.ino -> bool VindriktningReadData(void)
 * IKEA vindriktning particle concentration sensor support for Tasmota
 * https://github.com/arendst/Tasmota/blob/master/tasmota/tasmota_xsns_sensor/xsns_91_vindriktning.ino
 *
 * Copyright (C) 2021  Marcel Ritter and Theo Arends
\*********************************************************************************************/

#pragma once

#include <SoftwareSerial.h>

#include "Types.hpp"

namespace SerialCom {
	SoftwareSerial vindriktningSerial(PIN_UART_RX, PIN_UART_TX);

	void setup() {
		vindriktningSerial.begin(9600);
	}

	bool readData(Vindriktning &vindriktning) {
		if (!vindriktningSerial.available()) {
			return false;
		}
		while ((vindriktningSerial.peek() != 0x16) && vindriktningSerial.available()) {
			vindriktningSerial.read();
		}
		if (vindriktningSerial.available() < VINDRIKTNING_DATASET_SIZE) {
			return false;
		}

		uint8_t buffer[VINDRIKTNING_DATASET_SIZE];
		vindriktningSerial.readBytes(buffer, VINDRIKTNING_DATASET_SIZE);
		vindriktningSerial.flush();  // Make room for another burst

		uint8_t crc = 0;
		for (uint32_t i = 0; i < VINDRIKTNING_DATASET_SIZE; i++) {
			crc += buffer[i];
		}
		if (crc != 0) {
			Serial.printf("Received message with invalid checksum. Expected: 0. Actual: %d\n", crc);
			return false;
		}

		// sample data:
		//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
		// 16 11 0b 00 00 00 0c 00 00 03 cb 00 00 00 0c 01 00 00 00 e7
		//               |pm2_5|     |pm1_0|     |pm10 |        | CRC |
		vindriktning.pm2_5 = (buffer[5] << 8) | buffer[6];
		vindriktning.pm1_0 = (buffer[9] << 8) | buffer[10];
		vindriktning.pm10 = (buffer[13] << 8) | buffer[14];

		Serial.printf("Received PM 2.5 reading: %d\n", vindriktning.pm2_5);
		Serial.printf("Received PM 1.0 reading: %d\n", vindriktning.pm1_0);
		Serial.printf("Received PM 10 reading: %d\n", vindriktning.pm10);

		return true;
	}
} // namespace SerialCom

/*********************************************************************************
 *  MIT License
 *
 *  Copyright (c) 2020 Gregg E. Berman
 *
 *  https://github.com/HomeSpan/HomeSpan
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 ********************************************************************************/

/*
 *                  ESP-WROOM-32 Utilized pins
 *                ╔═════════════════════════════╗
 *                ║┌─┬─┐  ┌──┐  ┌─┐             ║
 *                ║│ | └──┘  └──┘ |             ║
 *                ║│ |            |             ║
 *                ╠═════════════════════════════╣
 *            +++ ║GND                       GND║ +++
 *            +++ ║3.3V                     IO23║ USED_FOR_NOTHING
 *                ║                         IO22║ SCL
 *                ║IO36                      IO1║ TX
 *                ║IO39                      IO3║ RX
 *                ║IO34                     IO21║ SDA
 *   LIGHT_SENSOR ║IO35                         ║ NC
 *        RED_LED ║IO32                     IO19║ MHZ TX
 *                ║IO33                     IO18║ MHZ RX
 *                ║IO25                      IO5║
 *     LED_YELLOW ║IO26                     IO17║
 *                ║IO27                     IO16║ NEOPIXEL
 *   VINDRIKTNING ║IO14                      IO4║
 *                ║IO12                      IO0║ +++, BUTTONS
 *                ╚═════════════════════════════╝
 */

#define REQUIRED   VERSION(1, 6, 0)
#define FW_VERSION "1.3"

String FirmwareVer = {
	FW_VERSION};

#define URL_fw_Version "https://raw.githubusercontent.com/oleksiikutuzov/esp32-homekit-air-quality/V4.0_test_updater/bin_version.txt"
#define URL_fw_Bin	   "https://raw.githubusercontent.com/oleksiikutuzov/esp32-homekit-air-quality/V4.0_test_updater/esp32_air_quality.bin"

#include "DEV_Sensors.hpp"
#include "SerialCom.hpp"
#include "Types.hpp"
#include <Adafruit_NeoPixel.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <ErriezMHZ19B.h>
#include <HomeSpan.h>
#include <SoftwareSerial.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"

#define BUTTON_PIN	   0
#define LED_STATUS_PIN 26

WebServer server(80);

DEV_CO2Sensor		 *CO2; // GLOBAL POINTER TO STORE SERVICE
DEV_AirQualitySensor *AQI; // GLOBAL POINTER TO STORE SERVICE

extern "C++" bool needToWarmUp;

#if HARDWARE_VER == 4
DEV_TemperatureSensor *TEMP;
DEV_HumiditySensor	  *HUM;
#endif

void firmwareUpdate();
int	 FirmwareVersionCheck();

unsigned long previousMillis = 0; // will store last time LED was updated
const long	  interval		 = 60000;

void repeatedCall() {
	static int	  num			= 0;
	unsigned long currentMillis = millis();
	if ((currentMillis - previousMillis) >= interval) {
		// save the last time you blinked the LED
		previousMillis = currentMillis;
		if (FirmwareVersionCheck()) {
			firmwareUpdate();
		}
	}
}

void setup() {

	Serial.begin(115200);

	Serial.print("Active firmware version: ");
	Serial.println(FirmwareVer);

	String	   temp			  = FW_VERSION;
	const char compile_date[] = __DATE__ " " __TIME__;
	char	  *fw_ver		  = new char[temp.length() + 30];
	strcpy(fw_ver, temp.c_str());
	strcat(fw_ver, " (");
	strcat(fw_ver, compile_date);
	strcat(fw_ver, ")");

	homeSpan.setControlPin(BUTTON_PIN);						   // Set button pin
	homeSpan.setStatusPin(LED_STATUS_PIN);					   // Set status led pin
	homeSpan.setLogLevel(1);								   // set log level
	homeSpan.setPortNum(88);								   // change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setStatusAutoOff(10);							   // turn off status led after 10 seconds of inactivity
	homeSpan.setWifiCallback(setupWeb);						   // need to start Web Server after WiFi is established
	homeSpan.reserveSocketConnections(3);					   // reserve 3 socket connections for Web Server
	homeSpan.enableWebLog(10, "pool.ntp.org", "UTC", "myLog"); // enable Web Log
	homeSpan.enableAutoStartAP();							   // enable auto start AP
	homeSpan.setSketchVersion(fw_ver);

	homeSpan.begin(Category::Bridges, "HomeSpan Air Sensor Bridge");

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::FirmwareRevision(temp.c_str());

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Carbon Dioxide Sensor");
	CO2 = new DEV_CO2Sensor(); // Create a CO2 Sensor (see DEV_Sensors.h for definition)

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Air Quality Sensor");
	AQI = new DEV_AirQualitySensor(); // Create an Air Quality Sensor (see DEV_Sensors.h for definition)

#if HARDWARE_VER == 4
	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Temperature Sensor");
	TEMP = new DEV_TemperatureSensor(); // Create a Temperature Sensor (see DEV_Sensors.h for definition)

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Humidity Sensor");
	HUM = new DEV_HumiditySensor(); // Create a Temperature Sensor (see DEV_Sensors.h for definition)
#endif
}

void loop() {
	homeSpan.poll();
	server.handleClient();
	repeatedCall();
}

void setupWeb() {
	LOG0("Starting Air Quality Sensor Server Hub...\n\n");

	server.on("/metrics", HTTP_GET, []() {
		float airQuality = AQI->pm25->getVal();
		float co2		 = CO2->co2Level->getVal();
#if HARDWARE_VER == 4
		float temp = TEMP->temp->getVal<float>();
		float hum  = HUM->hum->getVal<float>();
#endif
		int	   lightness		= neopixelAutoBrightness();
		float  uptime			= esp_timer_get_time() / (6 * 10e6);
		float  heap				= esp_get_free_heap_size();
		String airQualityMetric = "# HELP air_quality PM2.5 Density\nhomekit_air_quality{device=\"air_sensor\",location=\"home\"} " + String(airQuality);
		String CO2Metric		= "# HELP co2 Carbon Dioxide\nhomekit_carbon_dioxide{device=\"air_sensor\",location=\"home\"} " + String(co2);
		String uptimeMetric		= "# HELP uptime Sensor uptime\nhomekit_uptime{device=\"air_sensor\",location=\"home\"} " + String(int(uptime));
		String heapMetric		= "# HELP heap Available heap memory\nhomekit_heap{device=\"air_sensor\",location=\"home\"} " + String(int(heap));
		String lightnessMetric	= "# HELP lightness Lightness\nhomekit_lightness{device=\"air_sensor\",location=\"home\"} " + String(lightness);
#if HARDWARE_VER == 4
		String tempMetric = "# HELP temp Temperature\nhomekit_temperature{device=\"air_sensor\",location=\"home\"} " + String(temp);
		String humMetric  = "# HELP hum Relative Humidity\nhomekit_humidity{device=\"air_sensor\",location=\"home\"} " + String(hum);
#endif
		LOG1(airQualityMetric);
		LOG1(CO2Metric);
		LOG1(uptimeMetric);
		LOG1(heapMetric);
		LOG1(lightnessMetric);
#if HARDWARE_VER == 4
		LOG1(tempMetric);
		LOG1(humMetric);
#endif
		if (needToWarmUp == 0) { // exclude co2 and air quality if it is still warming up
#if HARDWARE_VER == 4
			server.send(200, "text/plain", airQualityMetric + "\n" + CO2Metric + "\n" + uptimeMetric + "\n" + heapMetric + "\n" + lightnessMetric + "\n" + tempMetric + "\n" + humMetric);
#else
			server.send(200, "text/plain", airQualityMetric + "\n" + CO2Metric + "\n" + uptimeMetric + "\n" + heapMetric + "\n" + lightnessMetric);
#endif
		} else {
#if HARDWARE_VER == 4
			server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + lightnessMetric + "\n" + tempMetric + "\n" + humMetric);
#else
			server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + lightnessMetric);

#endif
		}
	});

	server.on("/reboot", HTTP_GET, []() {
		String content = "<html><body>Rebooting!  Will return to configuration page in 10 seconds.<br><br>";
		content += "<meta http-equiv = \"refresh\" content = \"10; url = /\" />";
		server.send(200, "text/html", content);

		ESP.restart();
	});

	ElegantOTA.begin(&server); // Start ElegantOTA
	server.begin();
	Serial.println("HTTP server started");
} // setupWeb

void firmwareUpdate(void) {
	WiFiClientSecure client;
	client.setCACert(rootCACertificate);
	t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

	switch (ret) {
	case HTTP_UPDATE_FAILED:
		Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
		break;

	case HTTP_UPDATE_NO_UPDATES:
		Serial.println("HTTP_UPDATE_NO_UPDATES");
		break;

	case HTTP_UPDATE_OK:
		Serial.println("HTTP_UPDATE_OK");
		break;
	}
}

int FirmwareVersionCheck(void) {
	String payload;
	int	   httpCode;
	String fwurl = "";
	fwurl += URL_fw_Version;
	fwurl += "?";
	fwurl += String(rand());
	Serial.println(fwurl);
	WiFiClientSecure *client = new WiFiClientSecure;

	if (client) {
		client->setCACert(rootCACertificate);

		// Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
		HTTPClient https;

		if (https.begin(*client, fwurl)) { // HTTPS
			Serial.print("[HTTPS] GET...\n");
			// start connection and send HTTP header
			delay(100);
			httpCode = https.GET();
			delay(100);
			if (httpCode == HTTP_CODE_OK) // if version received
			{
				payload = https.getString(); // save received version
			} else {
				Serial.print("error in downloading version file:");
				Serial.println(httpCode);
			}
			https.end();
		}
		delete client;
	}

	if (httpCode == HTTP_CODE_OK) // if version received
	{
		payload.trim();
		if (payload.equals(FirmwareVer)) {
			Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
			return 0;
		} else {
			Serial.println(payload);
			Serial.println("New firmware detected");
			return 1;
		}
	}
	return 0;
}
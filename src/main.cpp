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

// HomeSpan minimum required version
#define REQUIRED VERSION(1, 6, 0)

#include "DEV_Sensors.hpp"
#include "SerialCom.hpp"
#include "Types.hpp"
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <HomeSpan.h>
#include <SoftwareSerial.h>
#include "OTA.hpp"

WebServer server(80);

DEV_AirQualitySensor *AQI; // GLOBAL POINTER TO STORE SERVICE
#ifdef CO2_SENSOR_ENABLED
DEV_CO2Sensor *CO2Sensor; // GLOBAL POINTER TO STORE CO2 SENSOR SERVICE
#endif

void setupWeb();

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
	homeSpan.enableWebLog(10, "pool.ntp.org", "UTC", "myLog"); // enable Web Log
	homeSpan.enableAutoStartAP();							   // enable auto start AP
	homeSpan.setSketchVersion(fw_ver);

	homeSpan.begin(Category::Sensors, "HomeSpan Air Sensor");

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::FirmwareRevision(temp.c_str());

	AQI = new DEV_AirQualitySensor(); // Create an Air Quality Sensor (see DEV_Sensors.h for definition)
#ifdef CO2_SENSOR_ENABLED
	CO2Sensor = new DEV_CO2Sensor(); // Create a CO2 Sensor (see DEV_Sensors.h for definition)
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
#ifdef CO2_SENSOR_ENABLED
		float co2Level = CO2Sensor->co2Level->getVal();
		bool co2Detected = CO2Sensor->co2Detected->getVal();
		float co2PeakLevel = CO2Sensor->co2PeakLevel->getVal();
#endif
		float uptime = esp_timer_get_time() / (6 * 10e6);
		float heap = esp_get_free_heap_size();

		String airQualityMetric = "# HELP air_quality PM2.5 Density\nhomekit_air_quality{device=\"air_sensor\",location=\"home\"} " + String(airQuality);
#ifdef CO2_SENSOR_ENABLED
		String co2Metric = "# HELP co2_level CO2 Level in ppm\nhomekit_co2_level{device=\"air_sensor\",location=\"home\"} " + String(co2Level);
		String co2PeakMetric = "# HELP co2_peak_level CO2 Peak Level in ppm\nhomekit_co2_peak_level{device=\"air_sensor\",location=\"home\"} " + String(co2PeakLevel);
		String co2DetectedMetric = "# HELP co2_detected CO2 Detected (1=true, 0=false)\nhomekit_co2_detected{device=\"air_sensor\",location=\"home\"} " + String(co2Detected ? 1 : 0);
#endif
		String uptimeMetric = "# HELP uptime Sensor uptime\nhomekit_uptime{device=\"air_sensor\",location=\"home\"} " + String(int(uptime));
		String heapMetric = "# HELP heap Available heap memory\nhomekit_heap{device=\"air_sensor\",location=\"home\"} " + String(int(heap));

		LOG1(airQualityMetric);
#ifdef CO2_SENSOR_ENABLED
		LOG1(co2Metric);
		LOG1(co2PeakMetric);
		LOG1(co2DetectedMetric);
#endif
		LOG1(uptimeMetric);
		LOG1(heapMetric);

#ifdef CO2_SENSOR_ENABLED
		server.send(200, "text/plain", airQualityMetric + "\n" + co2Metric + "\n" + co2PeakMetric + "\n" + co2DetectedMetric + "\n" + uptimeMetric + "\n" + heapMetric);
#else
		server.send(200, "text/plain", airQualityMetric + "\n" + uptimeMetric + "\n" + heapMetric);
#endif
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

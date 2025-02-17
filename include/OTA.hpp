#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.hpp"
#include <HomeSpan.h>

String FirmwareVer = {
	FW_VERSION
};

void firmwareUpdate();
int	 FirmwareVersionCheck();

unsigned long previousMillis = 0;			   // will store last time LED was updated
const long	  interval		 = 60 * 60 * 1000; // 1 hour in ms

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

void firmwareUpdate(void) {
	WiFiClientSecure client;
	client.setCACert(rootCACertificate);
	t_httpUpdate_return ret = httpUpdate.update(client, OTA_BIN_URL);

	switch (ret) {
	case HTTP_UPDATE_FAILED:
		LOG1("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
		break;

	case HTTP_UPDATE_NO_UPDATES:
		LOG1("HTTP_UPDATE_NO_UPDATES");
		break;

	case HTTP_UPDATE_OK:
		LOG1("HTTP_UPDATE_OK");
		break;
	}
}

int FirmwareVersionCheck(void) {
	String payload;
	int	   httpCode;
	String fwurl = "";
	fwurl += OTA_VERSION_URL;
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

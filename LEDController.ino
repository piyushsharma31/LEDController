#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266httpUpdate.h>
#include <EepromUtil.h>
#include <ESPConfig.h>
#include <ESP8266Controller.h>
#include "LEDController.h"

WiFiUDP wifiUdp;

// maximum bytes for LEDController = 3 LED x 201 bytes = 603 bytes. UDP_TX_PACKET_MAX_SIZE max size of UDP packet
const int SIZE_OF_PACKET_BUFFER = 255 * 3;
byte packetBuffer[SIZE_OF_PACKET_BUFFER];
byte replyBuffer[SIZE_OF_PACKET_BUFFER];
short replyBufferSize = 0;

ESPConfig configuration(/*controller name*/	"RGB LED", /*location*/ "Unknown", /*firmware version*/ "rgbc.201110.bin", /*router SSID*/ "onion", /*router SSID key*/ "242374666");

// initialized LED colors at start
LEDController redLED(/*controller name*/ "RED", /*red pin*/ 0, /*No. of capabilities*/ 6, configuration.sizeOfEEPROM());
LEDController blueLED(/*controller name*/ "BLUE", /*blue pin*/ 2, /*No. of capabilities*/ 6, configuration.sizeOfEEPROM()+redLED.sizeOfEEPROM());
LEDController greenLED(/*controller name*/ "GREEN", /*green pin*/ 3, /*No. of capabilities*/ 6, configuration.sizeOfEEPROM()+redLED.sizeOfEEPROM()+blueLED.sizeOfEEPROM());

void setup() {

	//delay(1000);
	DEBUG_PRINTLN("ESPController::setup");
	Serial.begin(115200);//,SERIAL_8N1,SERIAL_TX_ONLY);

	configuration.init(redLED.pin);

	// load RGB values from EEPROM
	redLED.loadCapabilities();
	greenLED.loadCapabilities();
	blueLED.loadCapabilities();

	wifiUdp.begin(port);
	DEBUG_PRINTLN("ESPController::setup end");
}

unsigned long last = 0;
int packetSize = 0;
_udp_packet udpPacket;
int heap_print_interval = 10000;
byte _pin = 10;
int count = 0;

void loop() {

	/*if (millis() - last > heap_print_interval) {
		DEBUG_PRINTLN();

		last = millis();
		DEBUG_PRINT("[MAIN] Free heap: ");
		Serial.println(ESP.getFreeHeap(), DEC);

	}*/

	packetSize = wifiUdp.parsePacket();

	if (packetSize) {

		// read the packet into packetBuffer
		int packetLength = wifiUdp.read(packetBuffer, packetSize);
		//wifiUdp.read(packetBuffer, packetSize);
		//if (packetLength > 0) {
		packetBuffer[packetSize] = 0;
		//}

    //Serial.println(++count);
		DEBUG_PRINTLN();
		DEBUG_PRINT("Received packet of packetSize ");
		DEBUG_PRINT(packetSize);
		DEBUG_PRINT(" packetLength ");
		DEBUG_PRINT(packetLength);
		DEBUG_PRINT(" from ");
		DEBUG_PRINT(wifiUdp.remoteIP());
		DEBUG_PRINT(", port ");
		DEBUG_PRINTLN(wifiUdp.remotePort());

		//printArray(packetBuffer, packetSize, false);

		// initialize the replyBuffer and replyBufferSize
		memset(replyBuffer, 0, SIZE_OF_PACKET_BUFFER);
		memcpy(replyBuffer, packetBuffer, 3);
		replyBufferSize = 3;

		// prepare the UDP header from buffer
		udpPacket._size = packetBuffer[1] << 8 | packetBuffer[0];
		udpPacket._command = packetBuffer[2];
		udpPacket._payload = (char*)packetBuffer + 3;

		_pin = udpPacket._payload[0];

		if (udpPacket._command == DEVICE_COMMAND_DISCOVER) {
			DEBUG_PRINTLN("Command: DEVICE_COMMAND_DISCOVER");

			//replyBufferSize += configuration.discover(replyBuffer+3);
			replyBufferSize += configuration.toByteArray(replyBuffer+replyBufferSize);

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONFIGURATION) {
			DEBUG_PRINTLN("Command: DEVICE_COMMAND_SET_CONFIGURATION");

			replyBufferSize += configuration.set(replyBuffer+3, (byte*)udpPacket._payload);

		} else if (udpPacket._command == DEVICE_COMMAND_GET_CONTROLLER) {
			DEBUG_PRINTLN("Command: DEVICE_COMMAND_GET_CONTROLLER");

			if (_pin == redLED.pin) {

				//memcpy(replyBuffer + 3, redLED.toByteArray(), redLED.sizeOfUDPPayload());
				replyBufferSize += redLED.toByteArray(replyBuffer + replyBufferSize);
				//replyBufferSize += redLED.sizeOfUDPPayload();

			} else if (_pin == greenLED.pin) {

				//memcpy(replyBuffer + 3, greenLED.toByteArray(), greenLED.sizeOfUDPPayload());
				replyBufferSize += greenLED.toByteArray(replyBuffer + replyBufferSize);
				//replyBufferSize += greenLED.sizeOfUDPPayload();

			} else if (_pin == blueLED.pin) {

				//memcpy(replyBuffer + 3, blueLED.toByteArray(), blueLED.sizeOfUDPPayload());
				replyBufferSize += blueLED.toByteArray(replyBuffer + replyBufferSize);
				//replyBufferSize += blueLED.sizeOfUDPPayload();
			}

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONTROLLER) {
			DEBUG_PRINTLN("Command: DEVICE_COMMAND_SET_CONTROLLER");

			if (_pin == redLED.pin) {

				redLED.fromByteArray((byte*)udpPacket._payload);

			} else if (_pin == greenLED.pin) {

				greenLED.fromByteArray((byte*)udpPacket._payload);

			} else if (_pin == blueLED.pin) {

				blueLED.fromByteArray((byte*)udpPacket._payload);

			}

		} else if (udpPacket._command == DEVICE_COMMAND_GETALL_CONTROLLER) {
			DEBUG_PRINTLN("Command: DEVICE_COMMAND_GETALL_CONTROLLER");

			//memcpy(replyBuffer + 3, redLED.toByteArray(), redLED.sizeOfUDPPayload());
			//memcpy(replyBuffer + 3 + redLED.sizeOfUDPPayload(), greenLED.toByteArray(), greenLED.sizeOfUDPPayload());
			//memcpy(replyBuffer + 3 + redLED.sizeOfUDPPayload() + greenLED.sizeOfUDPPayload(), blueLED.toByteArray(), blueLED.sizeOfUDPPayload());

			//replyBufferSize = 3 + redLED.sizeOfUDPPayload() + greenLED.sizeOfUDPPayload() + blueLED.sizeOfUDPPayload();
			//replyBufferSize += redLED.sizeOfUDPPayload() + greenLED.sizeOfUDPPayload() + blueLED.sizeOfUDPPayload();
			replyBufferSize += redLED.toByteArray(replyBuffer + replyBufferSize);
			replyBufferSize += greenLED.toByteArray(replyBuffer + replyBufferSize);
			replyBufferSize += blueLED.toByteArray(replyBuffer + replyBufferSize);

		} else if (udpPacket._command == DEVICE_COMMAND_SETALL_CONTROLLER) {
			DEBUG_PRINTLN("Command: DEVICE_COMMAND_SETALL_CONTROLLER");

			int i = 0;

			// update the LED variables with new values
			for (int count = 0; count < 3; count++) {

				if (udpPacket._payload[i] == redLED.pin) {

					redLED.fromByteArray((byte*)udpPacket._payload + i);

					i = i + redLED.sizeOfEEPROM();

				} else if (udpPacket._payload[i] == greenLED.pin) {

					greenLED.fromByteArray((byte*)udpPacket._payload + i);

					i = i + greenLED.sizeOfEEPROM();

				} else if (udpPacket._payload[i] == blueLED.pin) {

					blueLED.fromByteArray((byte*)udpPacket._payload + i);

					i = i + blueLED.sizeOfEEPROM();

				}
			}

			// (OVERRIDE) send 3 bytes (size, command) as reply to client
			//replyBufferSize = 3;

		}

		// update the size of replyBuffer in packet bytes
		replyBuffer[0] = lowByte(replyBufferSize);
		replyBuffer[1] = highByte(replyBufferSize);

		// send a reply, to the IP address and port that sent us the packet we received
		wifiUdp.beginPacket(wifiUdp.remoteIP(), wifiUdp.remotePort());
		wifiUdp.write(replyBuffer, replyBufferSize);
		wifiUdp.endPacket();

	}

	redLED.loop();
	greenLED.loop();
	blueLED.loop();

	yield();

}

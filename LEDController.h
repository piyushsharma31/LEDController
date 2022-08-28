#ifndef LEDController_h
#define LEDController_h

// store last time EEPROM was updated with LED values
//unsigned long lastMillisUpdateEEPROM = 0;

// lock to hold for delayTime until next LED get on
static int lockedByPIN = -1;

class LEDController : public ESP8266Controller {

public:

	// store LED PIN fade state; can be different from pinState when blink & fade both are ON
	short fadeState = LOW;
	// store LED PIN fade rate (negative or positive value of actual fade rate)
	short fadeRate = LOW;
	// times LED has blinked so far continuously
	short blinkedTimes = 0;
	// if LED must blink
	boolean mustBlink = false;

	// store current time
	unsigned long currentMillis = millis();
	// store last time LED was updated
	unsigned long previousMillis = 0;

	// time when first entered loop() function for this object
	unsigned long startTime = 0;

public:
	LEDController(char* nam, uint8_t _pin, uint8_t capCount, int start_address):ESP8266Controller(nam, _pin, capCount, start_address)	{
		DEBUG_PRINTLN("LEDController::ESP8266Controller");

		pinMode(pin, OUTPUT);
		digitalWrite(pin, LOW);

		strcpy(capabilities[0]._name, "ON TIME");
		capabilities[0]._value_min = 0;
		capabilities[0]._value_max = 1000;
		capabilities[0]._value  = 0;

		strcpy(capabilities[1]._name, "OFF TIME");
		capabilities[1]._value_min = 0;
		capabilities[1]._value_max = 1000;
		capabilities[1]._value  = 0;

		strcpy(capabilities[2]._name, "BLINK TIMES");
		capabilities[2]._value_min = 0;
		capabilities[2]._value_max = 10;
		capabilities[2]._value  = 0;

		strcpy(capabilities[3]._name, "INTENSITY");
		capabilities[3]._value_min = 0;
		capabilities[3]._value_max = 1024;
		capabilities[3]._value  = 1024;

		strcpy(capabilities[4]._name, "FADE RATE");
		capabilities[4]._value_min = 0;
		capabilities[4]._value_max = 100;
		capabilities[4]._value  = 0;

		strcpy(capabilities[5]._name, "START DELAY");
		capabilities[5]._value_min = 0;
		capabilities[5]._value_max = 1000;
		capabilities[5]._value  = 0;

	}

public:
	virtual void loop();
};


void LEDController::loop() {

	// check to see if it's time to change the state of the LED
	currentMillis = millis();

	// LED should be OFF in these conditions
	if (capabilities[3]/*intensity*/._value  == 0) {

		// if LED is ON, put it OFF
		if (pinState > 0) {
			pinState = 0;

			analogWrite(pin, pinState);
			DEBUG_PRINT("LED OFF:");toString();
		}

		// if LED is blinking, reset blinkedTimes
		if(mustBlink) {

			mustBlink = false;

			if(blinkedTimes != capabilities[2]/*blinkTimes*/._value) {
				//reset blinkedTimes
				blinkedTimes = capabilities[2]/*blinkTimes*/._value;
			}
		}

	} else {

		if (lockedByPIN != -1 && lockedByPIN != pin) {
			// skip if controller is locked by other pin

			return;
		}

		// if LED must be blinking & blinkTimes==0, then set blinkTimes=1 if not already
		if (capabilities[1]/*offTime*/._value  > 0 && capabilities[0]/*onTime*/._value  > 0) {

			if(!mustBlink) {
				mustBlink = true;
			}

			// if LED must be blinking (off, on time > 0), and blinkTimes==0, then set it to 1
			if(capabilities[2]/*blinkTimes*/._value  == 0) {
				capabilities[2]/*blinkTimes*/._value  = 1;
			}
		} else if((capabilities[1]/*offTime*/._value == 0 || capabilities[0]/*onTime*/._value == 0) && mustBlink) {

			mustBlink = false;
		}

		// lock controller if pin not locked & startDelay>0 OR mustBlink=true & blinkedTimes < blinkTimes
		if (lockedByPIN != pin && (capabilities[5]/*startdelay*/._value  > 0 || (capabilities[2]/*blinkTimes*/._value > blinkedTimes && mustBlink))) {

			lockedByPIN = pin;
			startTime = currentMillis;

			DEBUG_PRINT("*locked by* ");DEBUG_PRINT(currentMillis);DEBUG_PRINTLN(pin);
		}

		if (mustBlink && blinkedTimes < capabilities[2]/*blinkTimes*/._value) {
			// Blink LED @blinkTimes

			if (pinState == LOW && currentMillis - previousMillis >= capabilities[1]/*offTime*/._value) {
				// Turn ON LED

				// change the brightness for next time through the loop:
				if (capabilities[4]/*faderate*/._value > 0) {

					if(fadeRate != capabilities[4]/*faderate*/._value && fadeRate != -capabilities[4]/*faderate*/._value) {
						if(fadeRate <0) {
							fadeRate = -capabilities[4]/*faderate*/._value;
						} else {
							fadeRate = capabilities[4]/*faderate*/._value;
						}
					}

					pinState = fadeState + fadeRate;

					// reverse the direction of the fading at the ends of the fade:

					if (pinState <= 0) {
						pinState = 0;
						fadeRate = capabilities[4]/*faderate*/._value;
					} else if (pinState >= capabilities[3]/*intensity*/._value ) {
						pinState = capabilities[3]/*intensity*/._value ;
						fadeRate = -capabilities[4]/*faderate*/._value ;
					}

					fadeState = pinState;
					DEBUG_PRINT("FADECHK1:");
				} else {
					// Turn ON; initialized the fadeState with current intensity
					pinState = fadeState = capabilities[3]/*intensity*/._value ;
					DEBUG_PRINT("BLINK ON times ");DEBUG_PRINT(blinkedTimes);DEBUG_PRINT(":");
				}

				// Remember the time
				previousMillis = currentMillis;

				analogWrite(pin, pinState);

				toString();

			} else if (pinState != LOW && currentMillis - previousMillis >= capabilities[0]/*onTime*/._value) {
				// Turn OFF LED

				// Remember the time
				previousMillis = currentMillis;

				pinState = LOW;
				analogWrite(pin, pinState);

				blinkedTimes++;
				DEBUG_PRINT("BLINK OFF times ");DEBUG_PRINT(blinkedTimes);DEBUG_PRINT(":");toString();

			}
		} else if (capabilities[4]/*faderate*/._value > 0 && (currentMillis - previousMillis >= capabilities[0]/*onTime*/._value )) {
			// Fade LED @fadeRate

			analogWrite(pin, pinState);

			if(fadeRate != capabilities[4]/*faderate*/._value && fadeRate != -capabilities[4]/*faderate*/._value) {
				if(fadeRate <0) {
					fadeRate = -capabilities[4]/*faderate*/._value;
				} else {
					fadeRate = capabilities[4]/*faderate*/._value;
				}
			}

			// change the brightness for next time through the loop:
			pinState = pinState + fadeRate;

			// Remember the time
			previousMillis = currentMillis;

			// reverse the direction of the fading at the ends of the fade:
			if (pinState <= 0) {
				pinState = 0;
				fadeRate = capabilities[4]/*faderate*/._value;
			} else if (pinState >= capabilities[3]/*intensity*/._value) {
				pinState = capabilities[3]/*intensity*/._value ;
				fadeRate = -capabilities[4]/*faderate*/._value ;
			}

			DEBUG_PRINT("FADECHK2:");toString();
		} else if (!mustBlink && capabilities[4]/*faderate*/._value  == 0 && pinState != capabilities[3]/*intensity*/._value ) {
			// LED should be put on once

			pinState = capabilities[3]/*intensity*/._value ;
			fadeState = capabilities[3]/*intensity*/._value ;
			// Remember the time
			previousMillis = currentMillis;

			analogWrite(pin, pinState);
			DEBUG_PRINT("LED ON:");toString();
		}
	}

	if (currentMillis - startTime >= capabilities[5]/*startdelay*/._value && (!mustBlink || blinkedTimes >= capabilities[2]/*blinkTimes*/._value) && lockedByPIN == pin) {
		// reset startTime and unlock
		startTime = 0;
		blinkedTimes = 0;
		lockedByPIN = -1;
		DEBUG_PRINT("*unlocked by* ");DEBUG_PRINT(currentMillis);DEBUG_PRINTLN(pin);
	}

	// update EEPROM if capabilities changed
	if (currentMillis - lastEepromUpdate > eeprom_update_interval) {
		//DEBUG_PRINTLN();

		lastEepromUpdate = millis();
		//DEBUG_PRINT("[MAIN] Free heap: ");
		//Serial.println(ESP.getFreeHeap(), DEC);

		// save RGB LED status every one minute if required
		if(eepromUpdatePending == true) {

			DEBUG_PRINT("saveCapabilities pin ");DEBUG_PRINTLN(pin);
			saveCapabilities();
		}
	}
}

#endif

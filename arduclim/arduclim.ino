/*
   Arduclim mk II         written by navid
*/

//libraries to be added
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <MemoryFree.h>					//only include when debugging sram


//configurations
int compressor_interval;          //how often can the compressor can be switched in ms


//Pin setup
//analogue
const byte photoresistor_pin = 1;
const byte temp_in_pin = 3;
const byte button_pin = 2;
//digital
const byte temp_fan_pin = 3;
const byte compressor_pin = 8;
const byte fanspeed_pin = 9;

//EEPROM addresses
byte ee_ttemp = 0;
byte ee_setfanspeed = 1;
byte ee_lighttrig = 2;
byte ee_compressor_interval = 3;

//OLED configuration
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

//variables related to buttons
short button;
byte buttpress;

//variables related to photoresistor
byte photoresistor;
byte lighttrig;

//variables related to GUI
byte s;
byte s3 = 1;
byte fandisp;          //fan speed converted from either chartspeed or setfanspeed depending on fanstate ranging from 0 to 100

//variables related to fan controller
boolean fanstate;
byte setfanspeed;          //speed selected by the user ranging from 0 to 100
byte fanspeed;         //actual fan speed ranging from 0 to 255
byte chartspeed;

//variables related to compressor controller
boolean ic;
unsigned long compressor_timer = 0;
unsigned long currentMillis_compressor;

//variables related to fan probe
float ttemp;
float tidelta;
int Vo;
float tempfua = 0;
float tempfcollector = 0;
byte itf;
float R1 = 10000;
float logR2, R2, T, tempf, Tf;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

//variables related to recirculation
boolean resirc;

//misc. variables


void setup() {
  //loads all the saved values from the EEPROM
  EEPROM.get(ee_ttemp, ttemp);
  setfanspeed = EEPROM.read(ee_setfanspeed);
  lighttrig = EEPROM.read(ee_lighttrig);
  EEPROM.get(ee_compressor_interval, compressor_interval);

  //OLED setup
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();

  //pin mode setup
  pinMode(compressor_pin, OUTPUT);
  pinMode(fanspeed_pin, OUTPUT);

  //Misc.
  s = 0;
  ic = 1;
  chartspeed = 120;
  Serial.begin(9600);         //comment to disable when serial is disabled
}

void loop() {
  tidelta = tempf - ttemp;

  //debug_ValueSerial();          //uncomment to enable printing values to serial
  value_refresh();         //refreshes all values
  temp_fan();
  button_value();        //button reader
  screen();
  speedchart();
  controller();
}

float temp_fan() {         //retrieves the temperature from the probe in the fan box
  Vo = analogRead(temp_fan_pin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  tempfua = T - 273.15;
  tempfcollector = tempfcollector + tempfua;
  itf++;
  if (itf >= 50) {
    tempf = tempfcollector / itf;
    tempfcollector = 0;
    itf = 0;
    return;
  }
}
void value_refresh() {
  button = analogRead(button_pin);
  photoresistor = analogRead(photoresistor_pin) / 4.02;
}

void controller() {
  // compressor controller
  if ( ic == 1) {
    if (tidelta > 0) {
      digitalWrite(compressor_pin, 0);
      ic = 0;
    }
    if (tidelta <= 0) {
      digitalWrite(compressor_pin, 1);
      ic = 0;
    }
  }
  if ( ic == 0) {
    currentMillis_compressor = millis();
    if (currentMillis_compressor - compressor_timer >= compressor_interval) {
      compressor_timer = currentMillis_compressor;
      ic = 1;
    }
  }

  // fan speed controller
  if ( ic == 1) {
    if (tidelta > 0) {
      digitalWrite(compressor_pin, 0);
      ic = 0;
    }
    if (tidelta <= 0) {
      digitalWrite(compressor_pin, 1);
      ic = 0;
    }
  }
  if ( ic == 0) {
    currentMillis_compressor = millis();
    if (currentMillis_compressor - compressor_timer >= compressor_interval) {
      compressor_timer = currentMillis_compressor;
      ic = 1;
    }
  }



  if (setfanspeed >= 0 && setfanspeed <= 100) {
    fanspeed = setfanspeed * 2.55;
    fandisp = setfanspeed;
    fanstate = 0;
  } if (setfanspeed == 105) {
    fanspeed = chartspeed;
    fandisp = chartspeed / 2.55;
    fanstate = 1;
  }
  analogWrite(fanspeed_pin, fanspeed);
}

void speedchart() {
  if (tidelta < -1) {
    if ( chartspeed > 0 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 0 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= -1 && tidelta < 0) {
    if ( chartspeed > 30 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 30 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= 0 && tidelta < 1) {
    if ( chartspeed > 40 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 40 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= 1 && tidelta < 2) {
    if ( chartspeed > 60 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 60 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= 2 && tidelta < 3) {
    if ( chartspeed > 120 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 120 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= 3 && tidelta < 4) {
    if ( chartspeed > 160 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 160 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= 4 && tidelta < 5) {
    if ( tidelta >= 5 && tidelta < 6) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 190 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= 5 && tidelta < 6) {
    if ( chartspeed > 220 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 220 ) {
      chartspeed = chartspeed + 1;
    }
  }

  if (tidelta >= 6) {
    if ( chartspeed > 255 ) {
      chartspeed = chartspeed - 1;
    } if ( chartspeed < 255 ) {
      chartspeed = chartspeed + 1;
    }
  }
}

/*void debug_ValueSerial() {
  Serial.print(F("Button value: "));
  Serial.print(button);
  Serial.print(F(" Button pressed: "));
  Serial.print(buttpress);
  Serial.print(F(" temperature: "));
  Serial.print(tempf, 1);
  Serial.print(F(" light level: "));
  Serial.print(photoresistor);
  Serial.print(F(" setfanspeed: "));
  Serial.print(setfanspeed);
  Serial.print(F(" fanspeed: "));
  Serial.print(fanspeed);
  Serial.print(F(" compressor_timer: "));
  Serial.print(compressor_timer);
  Serial.print(F(" ic: "));
  Serial.println(freeMemory());

}*/

void screen() {
  if (s == 0) {         //boot menu
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(F("Arduclim"));
    display.println(F("v0.6"));
    display.display();
    delay(1000);
    display.clearDisplay();
    s = 1;
  }
  if (s == 1) {
    display.setTextColor(WHITE, BLACK);
    display.setCursor(0, 12);
    display.setTextSize(1);
    display.println(F("room"));
    display.print(F("temp:"));
    display.setTextSize(4);
    display.setCursor(30, 0);
    display.print(tempf, 1);
    display.setCursor(0, 32);
    display.setTextSize(1);
    display.print(F("Target:"));
    display.print(ttemp, 1);
    display.println(F("C"));
    display.print(F("fan speed: "));
    display.print(fandisp);
    display.print(F("% "));
    if (fanstate == 1) {
      display.setTextColor(WHITE, BLACK);
      display.println(F("Auto "));
    } else {
      display.setTextColor(WHITE, BLACK);
      display.println(F("Man  "));
    }
    display.print(F("recirculation: "));
    if (resirc == 1) {
      display.setTextColor(WHITE, BLACK);
      display.print(F("On "));
    }
    else {
      display.setTextColor(WHITE, BLACK);
      display.print(F("Off"));
    }
    display.display();
    if (buttpress == 1) {
      ttemp = ttemp + 0.5;
      if (ttemp > 33) {
        ttemp = 33;
      }
      EEPROM.put(ee_ttemp, ttemp);
	  delay(50);
    }
    if (buttpress == 2) {
      ttemp = ttemp - 0.5;
      if (ttemp < 16) {
        ttemp = 16;
      }
	  EEPROM.put(ee_ttemp, ttemp);
	  delay(50);
    }
    if (buttpress == 3) {
      setfanspeed = setfanspeed + 5;
      if (setfanspeed > 105) {
        setfanspeed = 105;
      }
      EEPROM.update(ee_setfanspeed, setfanspeed);
	  delay(50);
    }
    if (buttpress == 4) {
      setfanspeed = setfanspeed - 5;
      if (setfanspeed < 0) {
        setfanspeed = 0;
      }
      EEPROM.update(ee_setfanspeed, setfanspeed);
	  delay(50);
    }
    if (buttpress == 6) {
      display.clearDisplay();
      s = 2;
    }
  }
  if (s == 2) {					//Home Page
    display.setTextColor(WHITE, BLACK);
    display.setCursor(0,  0);
    display.setTextSize(1);
    display.println(F("1: Default Screen"));
    display.println(F("2: Debug Monitor"));
    display.println(F("3: Configuration"));
    display.display();
    if (buttpress == 1) {
      display.clearDisplay();
      s = 1;
    }
	if (buttpress == 2) {
		display.clearDisplay();
		s = 3;
	}
	if (buttpress == 3) {
		display.clearDisplay();
		s = 4;
	}
  }
  if (s == 3) {					//Debug Monitor
    if (s3 == 1) {
      display.setTextColor(WHITE, BLACK);
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.print(F("Button value: "));
      display.println(button);
      display.print(F("Button pressed: "));
      display.println(buttpress);
      display.print(F("temperature: "));
      display.println(tempf, 1);
      display.print(F("light level: "));
      display.println(photoresistor);
      display.print(F("setfanspeed: "));
      display.println(setfanspeed);
      display.print(F("fanspeed: "));
      display.println(fanspeed);
      display.print(F("ct: "));
      display.println(compressor_timer);
      display.print(F("free memory: "));
      display.println(freeMemory());
	  display.display();
	  if (buttpress == 5) {
		  display.clearDisplay();
		  s3 = 2;
	  }
    }
	if (s3 == 2) {
		display.setTextColor(WHITE, BLACK);
		display.setCursor(0, 0);
		display.setTextSize(1);
		display.print(F("ic: "));
		display.println(ic);
		display.print(F("ti delta: "));
		display.println(tidelta);
		display.display();
		if (buttpress == 5) {
			display.clearDisplay();
			s3 = 1;
		}

	}
	if (buttpress == 6) {
		display.clearDisplay();
		s = 2;
	}
  }
  if (s == 4) {
	  display.setTextColor(WHITE, BLACK);
	  display.setCursor(0, 0);
	  display.setTextSize(1);
	  display.print(F("Compressor Interval: "));
	  display.print(compressor_interval/1000);
	  display.print(F("S"));
	  display.display();
	  if (buttpress == 1) {
		  compressor_interval = compressor_interval + 1000;
		  EEPROM.put(ee_compressor_interval, compressor_interval);
		  delay(50);
	  }
	  if (buttpress == 2) {
		  compressor_interval = compressor_interval - 1000;
		  EEPROM.put(ee_compressor_interval, compressor_interval);
		  delay(50);
	  }
	  if (buttpress == 6) {
		  display.clearDisplay();
		  s = 2;
	  }
  }
}

void button_value() {

  if (button > 300  && button < 400) {

    buttpress = 6;

  } else if (button > 500 && button < 600) {

    buttpress = 5;

  } else if (button > 200 && button < 299) {

    buttpress = 4;

  } else if (button > 650 && button < 750) {

    buttpress = 3;

  } else if (button < 50) {

    buttpress = 2;

  } else if (button > 800 && button < 900) {

    buttpress = 1;

  } else {

    buttpress = 0;

  }
}

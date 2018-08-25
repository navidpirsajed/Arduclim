/*
   Arduclim mk II         written by navid 
*/

//libraries to be added
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Arduino.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);


//configurations
const int compressor_interval = 20000;          //how often can the compressor can be switched in ms

//Pin setup
const int photoresistor_pin = A0;
const int temp_in_pin = A1;
const int button_pin = A2;
const int temp_fan_pin = A3;
const int compressor_pin = 12;
const int fanspeed_pin = 9;

//EEPROM addresses
int ee_ttemp = 0;
int ee_setfanspeed = 1;
int ee_lighttrig = 2;

//OLED configuration


//variables related to buttons
int button;
int buttpress;

//variables related to photoresistor
int photoresistor;
int lighttrig;

//variables related to GUI
int s;
int fandisp;          //fan speed converted from either chartspeed or setfanspeed depending on fanstate ranging from 0 to 100

//variables related to fan controller
int fanstate;
int setfanspeed;          //speed selected by the user ranging from 0 to 100
int fanspeed;         //actual fan speed ranging from 0 to 255
int chartspeed;

//variables related to compressor controller
int ic;
int compressor;
int compressor_previous;
unsigned long compressor_timer = 0;
unsigned long currentMillis_compressor;

//variables related to fan probe
int tempi;
int ttemp;
int tidelta;
int Vo;
float tempfua = 0;
float tempfcollector = 0;
int itf;
float R1 = 10000;
float logR2, R2, T, tempf, Tf;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;



void setup() {
  //loads all the saved values from the EEPROM
  ttemp = EEPROM.read(ee_ttemp);
  setfanspeed = EEPROM.read(ee_setfanspeed);
  lighttrig = EEPROM.read(ee_lighttrig);

  //OLED setup
  u8x8.begin();
  u8x8.setPowerSave(0);
  
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

  debug_ValueSerial();          //uncomment to enable printing values to serial
  value_refresh();         //refreshes all values
  temp_fan();
  button_value();        //button reader
  //screen();
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
  if (itf >= 100) {
    itf = itf + 1;
    tempf = tempfcollector / itf;
    tempfcollector = tempf;
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
  if (compressor - compressor_previous != 0) {
    ic = 0;
  }

  // fan speed controller
  compressor_previous = compressor;
  if ( ic == 1) {
    if (tidelta > 0) {
      digitalWrite(compressor_pin, 0);
      compressor = 1;
    }
    if (tidelta <= 0) {
      digitalWrite(compressor_pin, 1);
      compressor = 0;
    }
  }
  if ( ic == 0) {
    currentMillis_compressor = millis();
    if (currentMillis_compressor - compressor_timer >= compressor_interval) {
      compressor_timer = currentMillis_compressor;
      ic = 1;
    }
  }
  if (compressor - compressor_previous != 0) {
    ic = 0;
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

void debug_ValueSerial() {
  Serial.print("Button value: ");
  Serial.print(button);
  Serial.print(" Button pressed: ");
  Serial.print(buttpress);
  Serial.print(" temperature: ");
  Serial.print(tempf);
  Serial.print(" light level: ");
  Serial.print(photoresistor);
  Serial.print(" setfanspeed: ");
  Serial.print(setfanspeed);
  Serial.print(" fanspeed: ");
  Serial.print(fanspeed);
  Serial.print(" compressor_timer: ");
  Serial.print(compressor_timer);
  Serial.print(" ic: ");
  Serial.println(ic);

}

void screen() {
  if (s == 0) {         //boot menu
    s = 1;
  }
  if (s == 1) {
    
    if (buttpress == 1) {
      ttemp = ttemp + 1;
      if (ttemp > 33) {
        ttemp = 33;
      }
      EEPROM.write(ee_ttemp, ttemp);
      delay(75);
      return;
    }
    if (buttpress == 2) {
      ttemp = ttemp - 1;
      if (ttemp < 16) {
        ttemp = 16;
      }
      EEPROM.write(ee_ttemp, ttemp);
      delay(75);
      return;
    }
    if (buttpress == 3) {
      setfanspeed = setfanspeed + 5;
      if (setfanspeed > 105) {
        setfanspeed = 105;
      }
      EEPROM.write(ee_setfanspeed, setfanspeed);
      delay(50);
      return;
    }
    if (buttpress == 4) {
      setfanspeed = setfanspeed - 5;
      if (setfanspeed < 0) {
        setfanspeed = 0;
      }
      EEPROM.write(ee_setfanspeed, setfanspeed);
      delay(75);
      return;
    }
    if (buttpress == 5) {
      s = s + 1;
      delay(75);
      return;
    }
  }
  if (s == 2) {
    if (buttpress == 1) {
      lighttrig = lighttrig + 1;
      if (lighttrig > 255) {
        lighttrig = 255;
      }
      EEPROM.write(ee_lighttrig, lighttrig);
      delay(75);
      return;
    }
    if (buttpress == 2) {
      lighttrig = lighttrig - 1;
      if (lighttrig < 0) {
        lighttrig = 0;
      }
      EEPROM.write(ee_lighttrig, lighttrig);
      delay(75);
      return;
    }
    if (buttpress == 6) {
      s = 1;
      delay(75);
      return;
    }
  }
}

void button_value() {

  if (button > 1004  && button < 1010) {

    buttpress = 6;

  }else if (button > 850 && button < 960) {

    buttpress = 5;

  }else if (button > 400 && button < 600) {

    buttpress = 4;

  }else if (button > 294 && button < 354) {

    buttpress = 3;

  }else if (button > 675 && button < 730) {

    buttpress = 2;
    
  }else if (button > 60 && button < 150) {

    buttpress = 1;

  } else {

    buttpress = 0;

  }
}

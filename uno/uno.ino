/**
* ARDUINO UNO
*/
 
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <TimedAction.h>

#include "DFRobot_PH.h"
#include "OneWire.h"
#include "DallasTemperature.h"

// Const
#define PH_PIN A0                    // PH analog pin
#define ORP_PIN A1                   // ORP analog pin
#define WT_PIN A2                    // Water sensor analog pin
#define WT_BUZZER 6                  // Buzzer digital pin
#define ONE_WIRE_BUS 4               // Temperature digital pin
#define ARRAY_LENGTH_ORP  40         // Times of collection for ORP
#define OFFSET_ORP 0                 // Offset for ORP
#define VOLTAGE_ORP 5.00             // Voltage for ORP

// Data transmission
SoftwareSerial nodemcu(2,3);

// Temperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// PH
DFRobot_PH ph;

// ORP
int orpArray[ARRAY_LENGTH_ORP];
int orpArrayIndex=0;

// Misc
float voltage, phValue, wtValue, orpValue, temperature = 25;

/**
* get the average of a given array
*/
double averageArray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount = 0;
  if(0 > number) {
    Serial.println("Error number for the array to averaging!");
    return 0;
  }
  if(number<5) {   //less than 5, calculated directly statistics
    for(i = 0; i < number; ++i) {
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  } else {
    if(arr[0] < arr[1]) {
      min = arr[0];
      max = arr[1];
    }
    else {
      min = arr[1];
      max = arr[0];
    }
    for(i = 2; i < number; ++i) {
      if(arr[i] < min) {
        amount += min;        //arr<min
        min = arr[i]; 
      } else {
        if(arr[i] > max) {
          amount += max;    //arr>max
          max = arr[i];
        } else {
          amount += arr[i]; //min<=arr<=max
        }
      } //if
    } //for
    avg = (double) amount / (number-2);
  } //if
  return avg;
}

/**
* Reads the celsius temperature of the temperature sensor.
*/
float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

/**
* Reads the water level and send it to the ESP8266, to the serial port (debug) and updates the buzzer status.
*/
void handleWT() {
  wtValue = analogRead(WT_PIN);
  Serial.print("wt:");
  Serial.println(wtValue);
  nodemcu.print("wt:");
  nodemcu.println(wtValue);
  if (200 < wtValue) {
    digitalWrite(WT_BUZZER, HIGH); // BEEP BEEP I'M A SHEEP
  } else {
    digitalWrite(WT_BUZZER, LOW); // NO MORE BEEP
  }
}

/**
* Reads the temperature (celsius) and send it to the ESP8266 and to the serial port, with 2 decimals.
*/
void handleTemp() {
  temperature = readTemperature();
  Serial.print("temp:");
  nodemcu.print("temp:");
  Serial.println(temperature, 2);
  nodemcu.println(temperature, 2);
}

/**
* Reads the ORP value as milivolts and sent it to the ESP8266 and to the serial port, as an int.
* ORP values are a bit volatile, so we do an average on 40 values.
*/
void handleORP() {
  static unsigned long orpTimer=millis();
  static unsigned long printTime=millis();
  if(millis() >= orpTimer) {
    orpTimer=millis()+20;
    orpArray[orpArrayIndex++]=analogRead(ORP_PIN);    //read an analog value every 20ms
    if (orpArrayIndex == ARRAY_LENGTH_ORP) {
      orpArrayIndex=0;
    }
    // really complex math here, converting the raw input to milivolts, depends on the analog read rate and the voltage
    orpValue=((30*(double)VOLTAGE_ORP*1000)-(75*averageArray(orpArray, ARRAY_LENGTH_ORP)*VOLTAGE_ORP*1000/1024))/75-OFFSET_ORP;

  }  
  if(millis() >= printTime) {
    //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
    printTime=millis()+800;
    Serial.print("orp:");
    nodemcu.print("orp:");
    Serial.println((int)orpValue);
    nodemcu.println((int)orpValue); 
  }
}

/**
* Reads the pH value and convert it to the pH scale, then send it to the ESP8266 and the serial port.
* PH needs temperature to be more accurate.
*/
void handlePH() {
  temperature = readTemperature();         // read your temperature sensor to execute temperature compensation
  voltage = analogRead(PH_PIN)/1024.0*5000;  // read the voltage
  phValue = ph.readPH(voltage,temperature);  // convert voltage to pH with temperature compensation
  Serial.print("ph:");
  nodemcu.print("ph:");
  Serial.println(phValue,2);
  nodemcu.println(phValue,2);
  ph.calibration(voltage,temperature);           // calibration process by Serail CMD
}

// Protothreading
TimedAction tempThread = TimedAction(5000, handleTemp);
TimedAction phThread = TimedAction(10000, handlePH);
TimedAction orpThread = TimedAction(10000, handleORP);
TimedAction wtThread = TimedAction(1000, handleWT);

/**
* Setup
*/
void setup() {
    Serial.begin(115200);  
    nodemcu.begin(115200);
    pinMode(WT_LED, OUTPUT);
    ph.begin();
    sensors.begin();
}

/**
* Loop
*/
void loop() {
  // Run protothreading
  tempThread.check();
  phThread.check();
  orpThread.check();
  wtThread.check();
  // Sleep a little
  delay(1000);
}

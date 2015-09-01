/* 8.12.2015 Kyle Bowerman
* sparkcore temprature sensors to UBidots
* from: https://particle.hackster.io/AgustinP/logging-temperature-data-using-the-spark-core
* from: spark-temp
* from OLDETEST
  deviceOne: 2856B13A06000082
*/
#include "SparkFun_Micro_OLED_Particle_Library/firmware/SparkFunMicroOLED.h"
#include "math.h"
#include "HttpClient/HttpClient.h"
#include "OneWire/OneWire.h"
#include "spark-dallas-temperature/spark-dallas-temperature.h"

/*
Micro-OLED-Shield-Example.ino
SparkFun Micro OLED Library Hello World Example
Jim Lindblom @ SparkFun Electronics
Original Creation Date: June 22, 2015

This sketch prints a friendly, recognizable logo on the OLED Shield, then
  goes on to demo the Micro OLED library's functionality drawing pixels,
  lines, shapes, and text.

  Hardware Connections:
  This sketch was written specifically for the Photon Micro OLED Shield, which does all the wiring for you. If you have a Micro OLED breakout, use the following hardware setup:

    MicroOLED ------------- Photon
      GND ------------------- GND
      VDD ------------------- 3.3V (VCC)
    D1/MOSI ----------------- A5 (don't change)
    D0/SCK ------------------ A3 (don't change)
      D2 -------------------- oneWire bus
      D/C ------------------- D6 (can be any digital pin)
      RST ------------------- D7 (can be any digital pin)
      CS  ------------------- A2 (can be any digital pin)

  Development environment specifics:
    IDE: Particle Build
    Hardware Platform: Particle Photon
                       SparkFun Photon Micro OLED Shield

  This code is beerware; if you see me (or any other SparkFun
  employee) at the local, and you've found our code helpful,
  please buy us a round!

  Distributed as-is; no warranty is given.
*/

//////////////////////////////////
// MicroOLED Object Declaration //
//////////////////////////////////
// Declare a MicroOLED object. If no parameters are supplied, default pins are
// used, which will work for the Photon Micro OLED Shield (RST=D7, DC=D6, CS=A2)

MicroOLED oled;

//////////////////////////////////
// other Declaration //
//////////////////////////////////

#define ONE_WIRE_BUS D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

#define VARIABLE_ID "55cbc6f2762542069b2798a9"  // temp 1
#define TEMP2_VARIABLE_ID "55cbd3277625421c8977ecea"
#define TOKEN "hsFRLnMcucOZlfLsQbH9BRbJpveccOc37ksq7eLOtjztxoEpZDA1D2wnWiuP"
#define UBIVARSIZE 24
#define PUSHFREQ 100
#define FILENAME "2sparktemp"
#define MYVERSION "0.2"


//Globals
char* ubivar[]={"55cbc6f2762542069b2798a9","55cbd3277625421c8977ecea","55cd69c37625426d64a926d3"};
float temperature = 0.0;
char resultstr[64];
HttpClient http;
int deviceCount, lastDeviceCount, mycounter = 0;
DeviceAddress tempDeviceAddress, deviceOne, deviceTwo, deviceThree, deviceFour; // We'll use this variable to store a found device address
http_header_t headers[] = {
      { "Content-Type", "application/json" },
      { "X-Auth-Token" , TOKEN },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};
http_request_t request;
http_response_t response;
int displayMode = 1;
bool debug = false;

//Prototypes
void printAddress(DeviceAddress deviceAddress);
int queryDevices(String command);
int setmode(String command);
int printEEProm(String command);
int regDevice(String command);




void setup()
{
  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  oled.display();  // Display what's in the buffer (splashscreen)
  delay(1000);     // Delay 1000 ms
  oled.clear(PAGE); // Clear the buffer.
  request.port = 80;
  request.hostname = "things.ubidots.com";
  Serial.begin(9600);
  sensor.begin();

  Spark.variable("count_devices", &deviceCount, INT);
  Spark.function("q", queryDevices);
  Spark.function("setmode", setModeFunc);
  Spark.function("printEEProm", printEEPROMFunc);
  Spark.function("regDevice", regDeviceFunc);





}

void loop()
{
  mycounter++;
  if(lastDeviceCount != deviceCount ) {  //device count changes   this never works
    oled.clear(ALL); // Clear the display's internal memory
    oled.display();  // Display what's in the buffer (splashscreen)
    //delay(1000);     // Delay 1000 ms
    oled.clear(PAGE); // Clear the buffer.
    Serial.println("\n\n The device Count Changed ");
  }
  deviceCount = getDeviceCount();
  temperatureJob();  // do the main temprature job
  lastDeviceCount = getDeviceCount();  // used to detect


}

void temperatureJob() {
      for (int i =0; i < deviceCount; i++ ) {
     request.body = getTemp(i);
      if (mycounter % PUSHFREQ == 0  ) {
            String mypath = String("/api/v1.6/variables/");
            mypath.concat(ubivar[i]);
            mypath.concat("/values");
            request.path = mypath;
            http.post(request, response, headers);
      }
      debugSerial(i);
      delay(200);
  }
}

void debugSerial(int i ) {
    if( debug  ) {
      Serial.print("\nmycounter ");
      Serial.print(mycounter);
      Serial.print(" index ");
      Serial.print(i);
      Serial.print(" ");
      Serial.print(request.body);
      Serial.print(" dcount ");
      Serial.print(deviceCount);
      Serial.print(" lcount ");
      Serial.println(lastDeviceCount);
    }

}

char *getTemp(int tempIndex) {
    static char retbuf[64];
    sensor.requestTemperatures();
    sensor.getAddress(tempDeviceAddress, tempIndex);
    temperature=sensor.getTempCByIndex( tempIndex );
    sprintf(retbuf, "{\"value\":%.4f}",temperature*1.8 + 32);
   // oPrintTemp(tempIndex, temperature);
   oDispatch(tempIndex, temperature);
    return retbuf;

}

void oDispatch(int tempIndex, float mytemp) {
    if (displayMode == 0 ) {
        // turn off display
        oled.clear(PAGE);  // this really clears the screen but need the oled.display()
        oled.display();
        }
    if (displayMode == 1 ) {
        oPrintTemp(tempIndex, temperature);
    }
    if (displayMode == 2 ) {
        oPrintTemp2(tempIndex, temperature);
    }
    if (displayMode == 3 ) {
    oPrintInfo();
    }

}

void oPrintTemp(int index, float mytemp){
    oled.setFontType(0);
    oled.setCursor(0,0);
    oled.print("devices ");
    oled.print(sensor.getDeviceCount());
    oled.setCursor(0,index*12+12);
    oled.print("T");
    oled.print(index);
    oled.print(" ");
    oled.print(mytemp*1.8 + 32);
    oled.display();

}

void oPrintTemp2(int index, float mytemp){
    oled.setFontType(0);
    oled.setCursor(0,0);
    oled.setCursor(0,index*12);
    oled.print("T");
    oled.print(index);
    oled.print(" ");
    oled.print(mytemp*1.8 + 32);
    oled.display();

}

void oPrintInfo() {
    oled.clear(PAGE);
    oled.setCursor(0,0);
    oled.print(FILENAME);
    oled.setCursor(0,10);
    oled.print("v");
    oled.print(MYVERSION);
    oled.setCursor(0,20);
    oled.print("pfreq ");
    oled.print(PUSHFREQ);
     oled.setCursor(0,30);
     oled.print("devices ");
     oled.print(deviceCount);
    oled.setCursor(0,40);
    oled.print(convertMillisToHuman(millis()));
    oled.display();
}

String convertMillisToHuman(int ms) {

    int seconds, minutes, hours, days = 0;

    int x = ms / 1000;
     seconds = x % 60;
    x /= 60;
     minutes = x % 60;
    x /= 60;
     hours = x % 24;
    x /= 24;
     days = x;

    String t = String(days);
    t.concat(":");
    t.concat(String(hours));
    t.concat(":");
    t.concat(String(minutes));
    t.concat(":");
    t.concat(String(seconds));

    return String(t);
}

int getDeviceCount() {
     sensor.begin();
    deviceCount = sensor.getDeviceCount();
    return deviceCount;
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++)
  {
          Serial.print(" ");
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

int printEEPROMFunc(String command){
   if(command == "all") {
       for(int i = 0; i < 100; i++) {
            if(EEPROM.read(i) != 0xFF ) {   //only print if it is used
                Serial.print(i);
                Serial.print(" EEPROM ");
                Serial.print(EEPROM.read(i),HEX);
                Serial.println();
            }
       }
       return 1;
   }
   if(command != "all" ){
       Serial.print(command);
       Serial.print(" EEPROM ");
       Serial.print(EEPROM.read(command.toInt()));
       //Now in hex
       Serial.print(" EEPROM 0x ");
       Serial.print(EEPROM.read(command.toInt()),HEX);
       Serial.println();
       return EEPROM.read(command.toInt());
   }

}

int queryDevices(String command) {
 if(command == "0" ) {
  if (!sensor.getAddress(deviceOne, 0)) Serial.println("\nUnable to find address for Device 0");
         Serial.print("\ndeviceOne: ");
         printAddress(deviceOne);
         Serial.print(" ");
         return 1;
  }
 if(command == "1") {
  if (!sensor.getAddress(deviceTwo, 1)) Serial.println("\nUnable to find address for Device 1");
        Serial.print("\ndeviceTwo: ");
        printAddress(deviceTwo);
        Serial.print(" ");
        return 1;
 }
 if(command == "2") {
  if (!sensor.getAddress(deviceThree, 2)) Serial.println("\nUnable to find address for Device 2");
        Serial.print("\ndeviceThree: ");
        printAddress(deviceThree);
        Serial.print(" ");
        return 1;
 }
 if(command == "3") {
  if (!sensor.getAddress(deviceFour, 3)) Serial.println("\nUnable to find address for Device 3");
         Serial.print("\ndeviceFour: ");
         printAddress(deviceFour);
         Serial.print(" ");
         return 1;
 }
 else return -1;
}


int setModeFunc(String command){ // now used for display mode and to toggle debug
    if(command != "debug") {
    oled.clear(PAGE);
    oled.display();
    displayMode = command.toInt();
    return displayMode;
    }
    if(command == "debug"){
        debug = !debug;
        Serial.print("\n\n debug mode: ");
        Serial.println(debug);
        return debug;
    }
}

int regDeviceFunc(String command){
    if(command == "searchReg") {
        int regfound = 0;
        for(int i = 0; i < 100; i++) {

           if(EEPROM.read(i) == 0xFF ) {
                regfound++;
                Serial.print(i);
                Serial.print(" EEPROM ");
                Serial.println();
           } else {
               Serial.print(i);
               Serial.println(" EEPROM not matching");
           }
       }
       return regfound;
    }
    if(command != "searchReg" ) {
        int mindex = command.toInt();
        int setAddr = mindex * 8;
        Serial.print("the address for device ");
        Serial.print(mindex);
        sensor.getAddress(tempDeviceAddress, mindex);
        // Serial.print(" the size of the tempAddress is ");
       // Serial.print(arraySize(tempDeviceAddress));

        /* got address in array
        1.  run through eeprom and search for device
        2.  if no device is found find the available slot, return the location in eeprom
        3.  loop through the tempaddress and write it to the location.
        */
        //Serial.println(tempDeviceAddress[0], HEX); // Works prints 28

        //write the address at location 0 - 7
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t writeLocation = i  + setAddr;
            EEPROM.write(writeLocation, tempDeviceAddress[i]);


        }






        return 2;

    }
}

/* 8.12.2015 Kyle Bowerman
* sparkcore temprature sensors to UBidots
* from: https://particle.hackster.io/AgustinP/logging-temperature-data-using-the-spark-core
* from: spark-temp
* from OLDETEST
  deviceOne: 2856B13A06000082
*/
#include "application.h"
#include "lib/streaming/firmware/spark-streaming.h"
#include "lib/SparkFun_Micro_OLED_Particle_Library/firmware/SparkFunMicroOLED.h"
#include "math.h"
#include "lib/HttpClient/firmware/HttpClient.h"
#include "lib/OneWire/OneWire.h"
//#include "spark-dallas-temperature/spark-dallas-temperature.h"
#include "lib/SparkDallas/spark-dallas-temperature.h"



/*
*
*
*    Hardware Connections:
*    This sketch was written specifically for the Photon Micro OLED Shield, which does all the wiring for you. If you have a Micro OLED breakout, use the following hardware setup:
*
*      MicroOLED ------------- Photon
*        GND ------------------- GND
*        VDD ------------------- 3.3V (VCC)
*      D1/MOSI ----------------- A5 (don't change)
*      D0/SCK ------------------ A3 (don't change)
*        D2 -------------------- D2 oneWire bus
*        D/C ------------------- D6 (can be any digital pin)
*        RST ------------------- D7 (can be any digital pin)
*        CS  ------------------- A2 (can be any digital pin)
*                                A0 encoder PinA
*                                A1 encoder PinB
*
*      A0   ENCODER PinA
*      A1   ENCODER PinB
*      A2
*      A3   OLED SCK
*      A4
*      A5   OLED MOSI
*      A6   -
*      A7
*      D0
*      D1
*      D2   ONEWIRE bus
*      D3
*      D4
*      D5
*      D6   OLED D/C
*      D7   OLED reset
*
*
*
*
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

/*
    //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
    Device Index 0 28 56 B1 3A 06 00 00 82
    Device Index 1 28 0E 52 58 06 00 00 0E
    Device Index 2 28 31 26 59 06 00 00 3A
    Device Index 3 28 49 2E E3 02 00 00 29

*/

#define ONE_WIRE_BUS D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

#define VARIABLE_ID "55cbc6f2762542069b2798a9"  // temp 1
#define TEMP2_VARIABLE_ID "55cbd3277625421c8977ecea"
#define TOKEN "hsFRLnMcucOZlfLsQbH9BRbJpveccOc37ksq7eLOtjztxoEpZDA1D2wnWiuP"
#define UBIVARSIZE 24
#define PUSHFREQ 500
#define FILENAME "5sparktemp"
#define MYVERSION "0.4.1"
#define GETTEMPFEQ 50


//Globals
char* ubivar[]={"55cbc6f2762542069b2798a9","55cbd3277625421c8977ecea","55cd69c37625426d64a926d3"};
float temperature = 0.0;
char resultstr[64];
HttpClient http;
int deviceCount, lastDeviceCount, lastime, mycounter = 0;
DeviceAddress tempDeviceAddress, deviceOne, deviceTwo, deviceThree, deviceFour; // We'll use this variable to store a found device address
DeviceAddress deviceIndexArray[5];  //dynamic Array


DeviceAddress outsideAddress = { 0x28, 0xe, 0x52, 0x58, 0x6, 0x0, 0x0, 0xe };
DeviceAddress floorAddress = { 0x28, 0x56, 0xB1, 0x3A, 0x06, 0x00, 0x00, 0x82 };
DeviceAddress pitAddress = { 0x28, 0x31, 0x26, 0x59, 0x06, 0x00, 0x00, 0x3A };
DeviceAddress boardAddress = { 0x28, 0x49, 0x2E, 0xE3, 0x02, 0x00, 0x00, 0x29 };

DeviceAddress*  deviceAddressArray[4] =  { &outsideAddress, &floorAddress, &pitAddress, &boardAddress } ;
String deviceNames[4]= { "outside", "floor", "pit", "board" };


http_header_t headers[] = {
      { "Content-Type", "application/json" },
      { "X-Auth-Token" , TOKEN },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};
http_request_t request;
http_response_t response;
int displayMode = 1;
bool debug = false;
bool gettempflag, pushtoubiflag = true;  // this doesnt seem to work

//Prototypes
void printAddress(DeviceAddress deviceAddress);
int queryDevices(String command);
int setmode(String command);
int printEEProm(String command);
int regDevice(String command);
void oDispatch(int tempIndex, float mytemp, DeviceAddress passedAddress);
void oPrintTemp3(int index, float mytemp,  DeviceAddress passedAddress);
void doEncoderA();
void doEncoderB();
int setModeFunc(String command);
int printEEPROMFunc(String command);
int getDeviceCount();
void temperatureJob();
double freqChecker();
String convertMillisToHuman(int ms);
void dispatchEncoder();
void dispatchEncoder();
void doEncoderA();
void doEncoderA();
int printEEPROMFunc(String command);
int setModeFunc(String command);
int regDeviceFunc(String command);
char *getTemp(int tempIndex);
void debugSerial(int i );
void oPrintTemp(int index, float mytemp);
void oPrintTemp2(int index, float mytemp);
void oPrintInfo();



// encoder
int encoderA = A0;
int encoderB = A1;
volatile bool A_set = false;
volatile bool B_set = false;
volatile int encoderPos = 0;
int prevPos = 0;
int value = 0;
int thistime, lasttime = 0 ;


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

  Particle.variable("count_devices", &deviceCount, INT);
  Particle.function("q", queryDevices);
  Particle.function("setmode", setModeFunc);
  Particle.function("printEEProm", printEEPROMFunc);
  Particle.function("regDevice", regDeviceFunc);

  //Need to set the device Index Array at startup
  deviceCount = getDeviceCount();
  queryDevices("auto");

   Particle.publish("reboot",Time.timeStr() );

     //encoder
  pinMode(encoderA, INPUT_PULLUP);
  pinMode(encoderB, INPUT_PULLUP);
  attachInterrupt(encoderA, doEncoderA, CHANGE);
  attachInterrupt(encoderB, doEncoderB, CHANGE);





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
  // only do these things every GETTEMPFEQ loops
  if (mycounter % GETTEMPFEQ == 0 ) {
    deviceCount = getDeviceCount();
    temperatureJob();  // do the main temprature job
    lastDeviceCount = getDeviceCount();  // used to detect
  }
  if( debug ) Serial << "the freq is: " << freqChecker() << "Hz" << endl;

  //encoder
  if (prevPos != encoderPos) {
        prevPos = encoderPos;
        Serial << "encoder position: " << encoderPos << endl;
        dispatchEncoder();
  }

    //*************** testing by hardcoded address Array ******


/*
   for (int i = 0; i < 4; i++ ){
     Serial <<  "the array hardcode version of: " << deviceNames[i] << " " <<  sensor.getTempF(*deviceAddressArray[i]) << endl;
    }*/


    //****************************************************/

  lastime = thistime;
  //thistime = millis();
  //delay(500);
  thistime = millis();

}

void dispatchEncoder(){
    if (encoderPos > 4 ) encoderPos = 4;
    setModeFunc(String(encoderPos));
}

void doEncoderA(){
  if( digitalRead(encoderA) != A_set ) {  // debounce once more
    A_set = !A_set;
    // adjust counter + if A leads B
    if ( A_set && !B_set )
      encoderPos += 1;
  }
}

void doEncoderB(){
    // Interrupt on B changing state, same as A above
   if( digitalRead(encoderB) != B_set ) {
    B_set = !B_set;
    //  adjust counter - 1 if B leads A
    if( B_set && !A_set )
      encoderPos -= 1;
  }
}

void temperatureJob() {
    sensor.requestTemperatures();  // get all the tempratures first to speed up, moved up from getTemp()
    for (int i =0; i < deviceCount; i++ ) {
     request.body = getTemp(i);
      if (mycounter % PUSHFREQ == 0  && pushtoubiflag == true ) {
            String mypath = String("/api/v1.6/variables/");
            mypath.concat(ubivar[i]);
            mypath.concat("/values");
            request.path = mypath;
            http.post(request, response, headers);
            if( debug ) Serial << "http response: " << request.body << endl;
      }
      if( debug) debugSerial(i);
      //delay(200);  //seems like there is a natrual delay of about 150 ms
    }
}

void debugSerial(int i ) {
   // if( debug  ) {
      Serial << "count: " << mycounter << " index " << i << " " << request.body << " dcount "<<  deviceCount << " lcount " << lastDeviceCount << endl;
    // }

}

char *getTemp(int tempIndex) {
    static char retbuf[64];
    //sensor.requestTemperatures();  // I am getting the tempratures each time in the loop, try moving this up to the fuction prior to the loop

    // Maybe this is bad,  to do it this way,  I am resetting the index each time.
    //sensor.getAddress(deviceIndexArray[tempIndex], tempIndex);   //Why do I need to get the address here?

    //temperature=sensor.getTempC(deviceIndexArray[tempIndex]);

    temperature=sensor.getTempF(deviceIndexArray[tempIndex]);



    //temperature=sensor.getTempCByIndex( tempIndex );// moved off of get address by index to get it by address
    //sprintf(retbuf, "{\"value\":%.4f}", temperature*1.8 + 32);
    String s = "{\"value\": ";
    s.concat(String(temperature));
    s.concat("}");
    s.toCharArray(retbuf, 64);
    oDispatch(tempIndex, temperature, deviceIndexArray[tempIndex]);
    return retbuf;

}

void oDispatch(int tempIndex, float mytemp, DeviceAddress passedAddress) {
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
        oPrintTemp3(tempIndex, temperature, passedAddress);
    }
    if (displayMode == 4 ) {
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
    oled.print(mytemp);
    oled.display();

}

void oPrintTemp2(int index, float mytemp){
    oled.setFontType(0);
    oled.setCursor(0,0);
    oled.setCursor(0,index*12);
    oled.print("T");
    oled.print(index);
    oled.print(" ");
    oled.print(mytemp);
    oled.display();

}

void oPrintTemp3(int index, float mytemp,  DeviceAddress passedAddress){
    oled.setFontType(0);
    oled.setCursor(0,0);
    oled.setCursor(0,index*12);
    oled.print(passedAddress[7],HEX);  // prints the last byte
    oled.print(" ");
    if (mytemp > 0 ) {
    oled.print(mytemp);
    }

    if (mytemp < 0 ) {
    oled.print("     ");

    Spark.publish("onewireloose", String(index) );
    Serial << "loose of onewire " << mytemp << "index " << index << endl;
    }
    oled.display();

}

void oPrintInfo() {
    oled.clear(PAGE);
    oled.setCursor(0,0);
    oled.print(FILENAME);
    oled.setCursor(0,10);
   // oled.print("v");
    //oled.print(MYVERSION);
    //oled.print(Spark.deviceID());
    oled << freqChecker() << " Hz";
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

    if(command == "auto") {
    // sets and prints the device array
        for(int i=0; i < deviceCount; i++ ) {
            sensor.getAddress(deviceIndexArray[i], i);
            Serial.print("Device Index ");
            Serial.print(i);
            printAddress(deviceIndexArray[i]);
            Serial.print("\n");
        }
        Serial.print("----------------------------------\n");
        return 1;
    }

    if(command == "array" ) {
    // prints the device Array but does not set it
        Serial << "Display deviceIndexArray " << endl << "----------------------------------\n"  << endl;
        for ( int i=0; i < 5; i++ ) {
            Serial << "\n deviceIndexArray[" << i << "]: ";
            printAddress(deviceIndexArray[i]);

        }
        Serial <<  "\n----------------------------------\n" << endl;
    }



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

double freqChecker() {
    double myperiod = (thistime - lastime);
    // Serial << "time diff in milli seconds " << thistime - lastime << endl;
    // Serial << "my debug calc peiriod x1000 " << 1 / (thistime - lastime / 1000 )<< endl;
    double myfrequency = ( 1 / myperiod ) * 1000;
    // Serial << "mypieriod " << myperiod << endl;
    // Serial << "myfrequency " << myfrequency << " Hz " << endl;
    return myfrequency;
}

/* 8.12.2015 Kyle Bowerman
* Last updated on 9.2.2015
* sparkcore temprature sensors to UBidots
* from: https://particle.hackster.io/AgustinP/logging-temperature-data-using-the-spark-core
* from: spark-temp, OLEDTEST, 6SPARKTEMP
* purpose:
*   1.  uses hardcoded address in array instead of calling by index.
*   2.  fix getting, prinint and pushing temp values when they are disconnected (-196)
*   3.  fix ubidots to always go to the right source
*/

#include "application.h"
 #include "lib/streaming/firmware/spark-streaming.h"
 #include "lib/SparkFun_Micro_OLED_Particle_Library/firmware/SparkFunMicroOLED.h"
 #include "math.h"
 #include "lib/HttpClient/firmware/HttpClient.h"
 #include "lib/OneWire/OneWire.h"
 #include "lib/SparkDallas/spark-dallas-temperature.h"
 #include "endevor2.h"

//Declarations
MicroOLED oled;
 OneWire oneWire(ONE_WIRE_BUS);
 DallasTemperature sensor(&oneWire);
 HttpClient http;

//Globals
bool debug = true;
  bool gettempflag = true;
  char* ubivar[]={"564a6d137625425cf86b4ce4", "xxx", "xxx","xxx"};
  char resultstr[64];
  int button = D4;
  int buttonvalue = 0;
  int displayMode = 2;
  int deviceCount, lastDeviceCount, lastime, mycounter,thistime, lasttime = 0;
  int prevPos = 0;
  int value = 0;
  int encoderA = A0;
  int encoderB = A1;
  int mydelay = 250;
  int relay = D3;
  float temperature = 0.0;
  int relayHoldDown = 30000;

 //devices
  // encolusre address   deviceIndexArray[0]:  28 7E F7 25 03 00 00 77
  DeviceAddress deviceIndexArray[5];  //dynamic Array
 //  DeviceAddress outsideAddress = { 0x28, 0xe, 0x52, 0x58, 0x6, 0x0, 0x0, 0xe };
  //DeviceAddress floorAddress = { 0x28, 0x56, 0xB1, 0x3A, 0x06, 0x00, 0x00, 0x82 };
  //DeviceAddress pitAddress = { 0x28, 0x31, 0x26, 0x59, 0x06, 0x00, 0x00, 0x3A };
  DeviceAddress boardAddress = { 0x28, 0x49, 0x2E, 0xE3, 0x02, 0x00, 0x00, 0x29 };
  DeviceAddress*  deviceAddressArray[1] =  { &boardAddress } ;
  //String deviceNames[4]= { "out", "flr", "pit", "brd" };
  String deviceNames[1]= { "brd" };

  http_header_t headers[] = {
        { "Content-Type", "application/json" },
        { "X-Auth-Token" , TOKEN },
      { NULL, NULL } // NOTE: Always terminate headers will NULL
  };
  http_request_t request;
  http_response_t response;

  volatile bool A_set = false;
  volatile bool B_set = false;
  volatile int encoderPos = 0;
  Timer relayTimer(relayHoldDown, expireRelay);


void setup()
{


  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  oled.display();  // Display what's in the buffer (splashscreen)
  delay(1000);     // Delay 1000 ms
  oled.clear(PAGE); // Clear the buffer.
  // display the version at boot for 2 seconds
  oled.setFontType(1);
  oled.setCursor(0,8);
  oled.print(FILENAME);
  oled.setCursor(0,24);
  oled.print(MYVERSION);
  oled.display();
  oled.setFontType(0);
  delay(5000);

  request.port = 80;
  request.hostname = "things.ubidots.com";
  Serial.begin(9600);
  sensor.begin();

  Particle.variable("count_devices", &deviceCount, INT);
  Particle.function("q", queryDevices);
  Particle.function("setmode", setModeFunc);
  Particle.function("printEEProm", printEEPROMFunc);
  Particle.function("relay", relayFunc);

  //Need to set the device Index Array at startup
  deviceCount = getDeviceCount();
  queryDevices("auto");

  Particle.publish("reboot",Time.timeStr() );

     //encoder
  pinMode(encoderA, INPUT_PULLUP);
  pinMode(encoderB, INPUT_PULLUP);
  pinMode(button,INPUT_PULLUP);
  pinMode(relay, OUTPUT);
  attachInterrupt(encoderA, doEncoderA, CHANGE);
  attachInterrupt(encoderB, doEncoderB, CHANGE);

 // temperatureJob();  // do this one time so the first screen gets displayed





}

void loop()
{

  //  New block to identify device count changes
  lastDeviceCount = deviceCount;
  deviceCount = getDeviceCount();
  //end New block
  mycounter++;

  if(lastDeviceCount != deviceCount ) {  //device count changes   this never works
    oled.clear(ALL); // Clear the display's internal memory
    oled.display();  // Display what's in the buffer (splashscreen)
    oled.setCursor(0,0);
    //oled << "Count " << endl << "changed " << endl <<  lastDeviceCount << " " << deviceCount << endl;
    oled.display();
    delay(5000);     // Delay 1000 ms
    oled.clear(PAGE); // Clear the buffer.
    Serial << " The device Count Changed " << lastDeviceCount << " " <<  deviceCount << endl;

  }
  // only do these things every GETTEMPFEQ loops or before I get to the first GETTEMPFREQ
  if (mycounter % GETTEMPFEQ == 0 ||  mycounter < GETTEMPFEQ ) {
    deviceCount = getDeviceCount();
    if ( deviceCount > 0 ) {
      temperatureJob();  // do the main temprature job
    }
    //  I think this is wrong
    //lastDeviceCount = getDeviceCount();  // used to detect
  }
  buttonvalue =  digitalRead(button);
  if( debug ) {
    Serial << mycounter << " freq: " << freqChecker() << "Hz | enocderPos: ";
    Serial << encoderPos << " | buttonvalue: " << buttonvalue << endl;
  }

  //encoder
  if (prevPos != encoderPos) {
        prevPos = encoderPos;
        Serial << "encoder position: " << encoderPos << endl;
        dispatchEncoder();
  }

  if (encoderPos == 4 )  oPrintInfo();
  if (encoderPos == 5 )  oPrintInfo5();
  if (deviceCount == 0 && encoderPos < 4 && encoderPos > 0 ) oPrintNoDevices() ;

  lastime = thistime;
  delay(mydelay);
  thistime = millis();

}

char *formatTempToBody(float temperature, int tempIndex) {
    static char retbuf[64];
    String s = "{\"value\": ";
    s.concat(String(temperature));
    s.concat("}");
    s.toCharArray(retbuf, 64);
    oDispatch(tempIndex, temperature);
    return retbuf;

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

void debugSerial(int i ) {
      Serial << "count: " << mycounter << " index " << i << " " << request.body << " dcount "<<  deviceCount << " lcount " << lastDeviceCount << endl;
}

void dispatchEncoder(){
    if (encoderPos > 5 ) encoderPos = 5;
    if (encoderPos < 0 ) encoderPos = 0;
    setModeFunc(String(encoderPos));
    if (deviceCount > 0 ) temperatureJob();
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

double freqChecker() {
    double myperiod = (thistime - lastime);
    double myfrequency = ( 1 / myperiod ) * 1000;
    return myfrequency;
}

int getDeviceCount() {
     sensor.begin();
    deviceCount = sensor.getDeviceCount();
    return deviceCount;
}

void oPrintInfo() {
    oled.clear(PAGE);
    oled.setCursor(0,0);
    oled.print(MYVERSION);
    oled.setCursor(0,10);
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

void oPrintInfo5() {
  uint32_t freemem = System.freeMemory();
    oled.clear(PAGE);
    oled.setCursor(0,0);
    oled.print(MYVERSION);
    oled.setCursor(0,10);
    oled << "MEMORY: " << endl << freemem << endl;
    oled << "BUTTON: " << buttonvalue << endl;
    oled << "sVer:" << System.version().c_str() << endl;
    oled.display();
}

void oPrintNoDevices() {
  oled.clear(PAGE);
  oled.setCursor(0,0);
  oled.setFontType(1);
  oled << "NO" << endl << "DEVICES" << endl;
  oled.setFontType(0);
  oled.display();
}

void oDispatch(int tempIndex, float temperature) {

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
        oPrintTemp3(tempIndex, temperature);
    }
    if (displayMode == 4 )  oPrintInfo();
    if (displayMode == 5 )  oPrintInfo5();

  Serial << "oled dispatch called " << endl;
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
    oled.setColor(BLACK);
    oled.print("T");
    oled.print(index);
    oled.setColor(WHITE);
    oled.print(" ");
    oled.print(mytemp);
    oled.display();

}

void oPrintTemp3(int index, float mytemp){
    oled.setFontType(0);
    oled.setCursor(0,0);
    oled.setCursor(0,index*12);
    String name = deviceNames[index];
    String shortname = name.substring(0,4);
    oled << shortname;
    oled.print(" ");
    if (mytemp > 0 ) {
    oled.print(mytemp);
    }

    if (mytemp < 0 ) {
    oled.print("     ");

    Particle.publish("onewireloose", String(index) );
    Serial << "loose of onewire " << mytemp << "index " << index << endl;
    }
    oled.display();

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
        return 1;
    }

    if(command == "invert" ) {
      oled.command(INVERTDISPLAY);
        Serial << "normal display " << endl;
      return 1;
    }

    if(command == "normal" ) {
      Serial << "normal display " << endl;
      oled.command(NORMALDISPLAY);
      return 1;
    }


    if(command == "sysver" ) {
    Serial << "System Version " << System.version().c_str() << endl;
      return 1;
    }

    else return -1;

}

int relayFunc(String command) {
  if(command == "on" ) {
    digitalWrite(relay, HIGH);
    //Timer timer(5000, expireRelay);
    relayTimer.start();
    return 1;
  }
  if(command == "off") {
    expireRelay();
    return 0;
  }

}

void expireRelay(){
  digitalWrite(relay, LOW);
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

void temperatureJob() {
    float gotTemp = 0;
    Serial << "the device count is " << deviceCount << endl;
    sensor.requestTemperatures();  // get all the tempratures first to speed up, moved up from getTemp()
    for (int i =0; i < deviceCount; i++ ) {
        gotTemp = sensor.getTempF(*deviceAddressArray[i]);
        if (gotTemp < -195 ) continue;
        Serial << "gotTemp() = "  << i << " " << gotTemp << endl;
        request.body = formatTempToBody(gotTemp, i);
      //  if (mycounter % PUSHFREQ == 0  && PUSHTOUBIFLAG == 1 ) {
       if (mycounter % PUSHFREQ == 0  && PUSHTOUBIFLAG == 1) {
            String mypath = String("/api/v1.6/variables/");
            mypath.concat(ubivar[i]);
            mypath.concat("/values");
            Serial << "going to push "<< request.body << " to " << mypath << endl;
            request.path = mypath;
            http.post(request, response, headers);
            if( debug ) Serial << "http body: " << request.body << endl;

            Serial << " Did we reboot?    I hope not ";
        }
      if( debug) debugSerial(i);

    }
}

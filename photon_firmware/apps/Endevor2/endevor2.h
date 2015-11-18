

//defines
#define ONE_WIRE_BUS D2
//OneWire oneWire(ONE_WIRE_BUS);
//DallasTemperature sensor(&oneWire);

#define VARIABLE_ID "55cbc6f2762542069b2798a9"  // temp 1
#define TEMP2_VARIABLE_ID "55cbd3277625421c8977ecea"
#define TOKEN "hsFRLnMcucOZlfLsQbH9BRbJpveccOc37ksq7eLOtjztxoEpZDA1D2wnWiuP"
#define UBIVARSIZE 24
#define PUSHFREQ 300
#define FILENAME "Endevor2"
#define MYVERSION "0.7.01"
#define GETTEMPFEQ 5
#define PUSHTOUBIFLAG 1





//Prototypes
char *formatTempToBody(float temperature, int tempIndex);
String convertMillisToHuman(int ms);
void debugSerial(int i );
void dispatchEncoder();
void doEncoderA();
void doEncoderB();
void expireRelay();
double freqChecker();
int getDeviceCount();
void oDispatch(int tempIndex, float mytemp);
void oPrintInfo();
void oPrintInfo5();
void oPrintNoDevices();
void oPrintTemp(int index, float mytemp);
void oPrintTemp2(int index, float mytemp);
void oPrintTemp3(int index, float mytemp);
void printAddress(DeviceAddress deviceAddress);
int printEEPROMFunc(String command);
int queryDevices(String command);
int regDevice(String command);
int regDeviceFunc(String command);
int relayFunc(String command);
int setModeFunc(String command);
int setmode(String command);
void temperatureJob();


/* Device Addresses
    //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
    Device Index 0 28 56 B1 3A 06 00 00 82
    Device Index 1 28 0E 52 58 06 00 00 0E
    Device Index 2 28 31 26 59 06 00 00 3A
    Device Index 3 28 49 2E E3 02 00 00 29

*/

/*  Notes
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
*      D3   Relay
*      D4  Encoder Button
*      D5
*      D6   OLED D/C
*      D7   OLED reset
*
*
*
*
*/

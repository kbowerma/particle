#include "application.h"
#include "lib/OneWire/OneWire.h"
#include "lib/SparkDallas/spark-dallas-temperature.h"


SYSTEM_MODE(AUTOMATIC);
#define ONE_WIRE_BUS D2
/*OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);*/

void setup()
{
  WiFi.selectAntenna(ANT_INTERNAL);
  Serial.begin(9600);
  /*sensors.begin();*/
}

void loop()
{
  Serial.println(WiFi.RSSI());
  delay(1000);
}

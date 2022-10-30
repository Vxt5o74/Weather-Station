#include <M5Stack.h>
#include "M5_ENV.h"

SHT3X sht30;
QMP6988 qmp6988;

float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;


void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Power.begin();
  M5.lcd.setTextSize(2);
  Wire.begin();  
    qmp6988.init();
    M5.lcd.println(F("ENVIII Unit test"));
}


void loop() {
  // put your main code here, to run repeatedly:
  pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {  
        tmp = sht30.cTemp;                      
        hum = sht30.humidity;  
                               
    } else {
        tmp = 0, hum = 0;
    }

    M5.lcd.setCursor(0, 20);
    M5.Lcd.printf("Temp: %2.1fC \r\nTemp: %2.1fF \r\nHumi: %2.0f%% \r\nPressure: %2.0fPa \r\nPressure: %2.0fmmHg",
                  tmp, tmp*(9/5)+32, hum, pressure, pressure/1.333);
    delay(100);
}

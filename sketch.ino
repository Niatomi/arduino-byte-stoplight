#include <EEPROM.h>

#define RED 3
#define GREEN 5
#define YELLOW 6

unsigned long globalTimeBuffer = 0;
byte dataLight[] = {
  //0bRGYTime
    0b0101111,
    0b0001000,
    0b0101000,
    0b0001000,
    0b1001000,
    0b1011000,
};
byte dataLightSize = sizeof(dataLight);

void setup() {
	Serial.begin(9600);

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(YELLOW, OUTPUT);

    digitalWrite(RED, 0);
    digitalWrite(YELLOW, 0);
    digitalWrite(GREEN, 0);
                 
}

void dataLightParseAndTurnLights(byte data[]) {
    for (int i = 0; i < dataLightSize; i++) {
        digitalWrite(RED, bitRead(data[i], 6));
        digitalWrite(GREEN, bitRead(data[i], 5));
        digitalWrite(YELLOW, bitRead(data[i], 4));
        
        int timeWait = (bitRead(data[i], 3) * 1000) + 
            (bitRead(data[i], 2) * 1000) + 
            (bitRead(data[i], 1) * 1000) + 
            (bitRead(data[i], 0) * 1000);
        
        delay(timeWait);
    }
}

void loop() {
    dataLightParseAndTurnLights(dataLight);
}

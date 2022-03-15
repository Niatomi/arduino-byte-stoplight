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

void loop() {
    dataLightParseAndTurnLights(dataLight);
}


void dataLightParseAndTurnLights(byte data[]) {
    for (int i = 0; i < dataLightSize; i++) {
        digitalWrite(RED, bitRead(data[i], 6));
        digitalWrite(GREEN, bitRead(data[i], 5));
        digitalWrite(YELLOW, bitRead(data[i], 4));
        
        int timeWait = timeCalculate(i);
        
        delay(timeWait);
    }
}

unsigned int timeCalculate(int index) {
    byte countTimeBytes = 0;
    for (int i = 0; i < 4; i++) {
        if (bitRead(dataLight[i], i) == 1) countTimeBytes++;
    }
    
    unsigned int timeWait = _BV(countTimeBytes) * 1000;
    return timeWait;
}
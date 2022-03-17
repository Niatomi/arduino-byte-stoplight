#include <QList.h>
#include <EEPROM.h>

#define RED 3
#define YELLOW 4
#define GREEN 5
#define DEBUG_LIGHT 6

#define REQUEST_WRITE_BUTTON 2

byte blinkDebugLightTimes = 4; 
int awaitTime = 5000;

unsigned long globalTimeBuffer = 0;
unsigned long secondaryGlobalTimeBuffer = 0;

struct StoplightScenes {    
    byte scenePosition = 0;
    // 163, 128, 160, 128, 160, 128, 195, 208
    byte dataLight[8] = {
      //0b1RGYTime
        0b10100011,
        //blinking
        0b10000000,
        0b10100000,
        0b10000000,
        0b10100000,
        0b10000000,
        //////////
        0b11000011,
        0b11010000,
    };
};

StoplightScenes stoplightCycle;

void setup() {
    digitalWrite(DEBUG_LIGHT, 1);

	Serial.begin(19200);

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(YELLOW, OUTPUT);
    pinMode(DEBUG_LIGHT, OUTPUT);

    pinMode(REQUEST_WRITE_BUTTON, INPUT_PULLUP);
    attachInterrupt(0, enterWriteNewScenesMode, CHANGE);

    digitalWrite(RED, bitRead(stoplightCycle.dataLight[stoplightCycle.scenePosition], 6));
    digitalWrite(GREEN, bitRead(stoplightCycle.dataLight[stoplightCycle.scenePosition], 5));
    digitalWrite(YELLOW, bitRead(stoplightCycle.dataLight[stoplightCycle.scenePosition], 4));
    digitalWrite(DEBUG_LIGHT, 0);

    digitalWrite(DEBUG_LIGHT, 0);

    enterWriteNewScenesMode();
}

unsigned int timeCalculate(int index) {
    byte countTimeBytes = 0;
    for (int i = 0; i < 4; i++) {
        if (bitRead(stoplightCycle.dataLight[index], i)) countTimeBytes++;
    }
    
    unsigned int timeWait = _BV(countTimeBytes) * 1000;
    return timeWait;
}

void dataLightParseAndTurnLights(byte data[]) {
    for (int i = 0; i < sizeof(stoplightCycle.dataLight); i++) {

        digitalWrite(RED, bitRead(data[i], 6));
        digitalWrite(GREEN, bitRead(data[i], 5));
        digitalWrite(YELLOW, bitRead(data[i], 4));
        
        int timeWait = timeCalculate(i);
        globalTimeBuffer = millis();
        boolean exitState = true;
        while (exitState) {
            if (millis() - globalTimeBuffer > timeWait) exitState = false;
        }   
    }
}

// ================ attachInterrupt ================ //
QList<byte> readNewByteScenes() {
    globalTimeBuffer = millis();
    Serial.println("Enter your bytes: ");
    QList<byte> list;
    
    boolean exitState = true;
    while (exitState) {
    
        if (Serial.available() > 1) {

            unsigned int pseudoByte = Serial.parseInt();
            Serial.flush();
            list.push_back(pseudoByte);
            
            Serial.print(pseudoByte);
            Serial.println(" - scene saved");
            
            boolean cooldownState = true;
            globalTimeBuffer = millis();

            while (cooldownState) {
                if (millis() - globalTimeBuffer > 50) 
                    cooldownState = false;
            }


        }
        
        if (millis() - globalTimeBuffer > awaitTime) {
            serialPrintOptimizer("You've left update scene mode");
            exitState = false;
        }
        
    }
    return list;
}

void serialPrintOptimizer(String string) {
    Serial.flush();
    for (int i = 0; i < string.length(); i++) {
        if (i % 4 == 0) Serial.flush();
        Serial.print(string.charAt(i));
    }
}

void enterWriteNewScenesMode() {
    Serial.println("You now in update scene mode");
    
    disableStoplightUntilUpdateSceneAndTurnOnDebugLight();

    QList<byte> list = readNewByteScenes();

    showList(list);

    digitalWrite(DEBUG_LIGHT, 0);
}

void disableStoplightUntilUpdateSceneAndTurnOnDebugLight() {
    digitalWrite(RED, 0);
    digitalWrite(GREEN, 0);
    digitalWrite(YELLOW, 0);

    digitalWrite(DEBUG_LIGHT, 1);
}

void showList(QList<byte> list) {
    Serial.println();
    for (int i = 0; i < list.size(); i++) {
        Serial.println(list.at(i));
    }
    Serial.println();
}

// ================ ================ ================ //


void loop() {
    // dataLightParseAndTurnLights(stoplightCycle.dataLight);
}
#include <QList.h>
#include <EEPROM.h>

#define RED 4
#define YELLOW 5
#define GREEN 6
#define DEBUG_LIGHT 7

byte blinkDebugLightTimes = 4; 
volatile int debugWaitTime = 10000;

volatile unsigned long globalTimeBufferMillis = 0;

boolean enterWriteNewSceneModeState = false;

struct StoplightScenes {    
    byte scenePosition = 0;
    // 163, 128, 160, 128, 160, 128, 195, 208
    byte dataLight[16] = {
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
        0b00000000,
    };
};

struct StopLightBuffer {    
    byte scenePosition = 0;
    byte dataLight[16] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
};

StoplightScenes stoplightScenes;

StopLightBuffer stopLightBuffer;


void setup() {
    digitalWrite(DEBUG_LIGHT, 1);

	Serial.begin(19200);

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(YELLOW, OUTPUT);
    pinMode(DEBUG_LIGHT, OUTPUT);

    pinMode(3, INPUT_PULLUP);
    attachInterrupt(1, changeEnterWriteNewSceneModeState, FALLING);

    digitalWrite(RED, bitRead(stoplightScenes.dataLight[stoplightScenes.scenePosition], 6));
    digitalWrite(GREEN, bitRead(stoplightScenes.dataLight[stoplightScenes.scenePosition], 5));
    digitalWrite(YELLOW, bitRead(stoplightScenes.dataLight[stoplightScenes.scenePosition], 4));
    digitalWrite(DEBUG_LIGHT, 0);

}

void loop() {
    dataLightParseAndTurnLights(stoplightScenes.dataLight);
    if (enterWriteNewSceneModeState) enterWriteNewScenesMode()
        ;
}

void improvedDelay(int waitTime) {
    globalTimeBufferMillis = millis();
    boolean cooldownState = true;

    while (cooldownState) {
        if (millis() - globalTimeBufferMillis > waitTime) 
            cooldownState = false;
    }
}

unsigned int timeCalculate(int index) {
    byte countTimeBytes = 0;
    for (int i = 0; i < 4; i++) {
        if (bitRead(stoplightScenes.dataLight[index], i)) countTimeBytes++;
    }
    
    unsigned int timeWait = _BV(countTimeBytes) * 1000;
    return timeWait;
}

void dataLightParseAndTurnLights(byte data[]) {
    for (int i = 0; i < sizeof(stoplightScenes.dataLight); i++) {

        digitalWrite(RED, bitRead(data[i], 6));
        digitalWrite(GREEN, bitRead(data[i], 5));
        digitalWrite(YELLOW, bitRead(data[i], 4));
        
        int timeWait = timeCalculate(i);
        
        improvedDelay(timeWait); 
    }
}

// ================ attachInterrupt ================ //
void changeEnterWriteNewSceneModeState() {
    enterWriteNewSceneModeState = true;
}
// ================ ================ ================ //

// ================ Scene update on interrupt ================ //
void enterWriteNewScenesMode() {
    serialPrintOptimizer("You now in update scene mode");
    
    disableStoplightUntilUpdateSceneAndTurnOnDebugLight();

    readNewByteScenes();

    showScenesConfiguration();

    digitalWrite(DEBUG_LIGHT, 0);

    writeDataIntoStruct();

    enterWriteNewSceneModeState = false;
}

void disableStoplightUntilUpdateSceneAndTurnOnDebugLight() {
    digitalWrite(RED, 0);
    digitalWrite(GREEN, 0);
    digitalWrite(YELLOW, 0);

    digitalWrite(DEBUG_LIGHT, 1);
}

void readNewByteScenes() {
    globalTimeBufferMillis = millis();
    serialPrintOptimizer("Enter your bytes: ");
    byte indexPoint = -1;
    Serial.flush();
    boolean exitState = true;
    while (exitState) {

        if (Serial.available() > 1) {
            indexPoint++;
            stopLightBuffer.dataLight[indexPoint] = Serial.parseInt();
            Serial.flush();
            
            Serial.print(stopLightBuffer.dataLight[indexPoint]);
            serialPrintOptimizer(" - scene saved");
            
            // pseudoByte = NULL;
            improvedDelay(50);

        }
        
        if (indexPoint == 16) exitState = false
            ;

        if (millis() - globalTimeBufferMillis > debugWaitTime) {
            serialPrintOptimizer("You've left update scene mode");
            exitState = false;
        }
        
    }
}

void serialPrintOptimizer(String string) {
    Serial.flush();
    for (int i = 0; i < string.length(); i++) {
        Serial.print(string.charAt(i));
        if (i % 3 == 0) Serial.flush();
    }
    Serial.println("");
}


void showScenesConfiguration() {
    Serial.println("");
    Serial.println("New scenes configuration: ");
    for (int i = 0; i < sizeof(stopLightBuffer.dataLight); i++) {
        Serial.println(stopLightBuffer.dataLight[i]);
    }
    Serial.println();
}

void writeDataIntoStruct() {
    for (int i = 0; i < sizeof(stoplightScenes.dataLight); i++) {
        stoplightScenes.dataLight[i] = stopLightBuffer.dataLight[i];
    }
    serialPrintOptimizer("Data transfered to main struct");
}

// ================ ================ ================ //


#include <EEPROM.h>

#define RED 6
#define YELLOW 5
#define GREEN 4
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
        0b10100000,
        //blinking
        0b10000000,
        0b10100000,
        0b10000000,
        0b10100000,
        0b10000000,
        //////////
        0b11000000,
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

byte activeSpace = sizeof(stoplightScenes.dataLight);

void setup() {
    digitalWrite(DEBUG_LIGHT, 1);

	Serial.begin(19200);
    
    EEPROM.get(0, stoplightScenes.scenePosition);
    EEPROM.get(1, stoplightScenes.dataLight);

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

    Serial.println(stopLightBuffer.scenePosition);

}

byte index = 0;

void loop() {
    dataLightParseAndTurnLights(stoplightScenes.dataLight[index]);
    index++;
    stoplightScenes.scenePosition = index;

    saveEEPROM();

    if (enterWriteNewSceneModeState) {
        enterWriteNewScenesMode();
    }
}

void dataLightParseAndTurnLights(byte data) {

    digitalWrite(RED, bitRead(data, 6));
    digitalWrite(GREEN, bitRead(data, 5));
    digitalWrite(YELLOW, bitRead(data, 4));
    
    unsigned int timeWait = timeCalculate(data);
    
    if (data == 0) {
        index = 0;
        timeWait = 0;
    }

    improvedDelay(timeWait); 

}

unsigned int timeCalculate(byte data) {
    byte countTimeBytes = 0;
    for (int i = 0; i <= 4; i++) {
        if (bitRead(data, i)) countTimeBytes++;
    }
    
    unsigned int timeWait = _BV(countTimeBytes) * 1000;
    return timeWait;
}

void improvedDelay(unsigned int waitTime) {
    globalTimeBufferMillis = millis();
    boolean cooldownState = true;

    while (cooldownState) {
        if (millis() - globalTimeBufferMillis > waitTime) 
            cooldownState = false;
    }
}

void saveEEPROM() {
    Serial.println("Data saved to EEPROM");
    EEPROM.update(0, stoplightScenes.scenePosition);
    EEPROM.put(1, stoplightScenes.dataLight);
}

// ================ attachInterrupt ================ //
void changeEnterWriteNewSceneModeState() {
    enterWriteNewSceneModeState = true;
}
// ================ ================ ================ //

// ================ Scene update on interrupt ================ //
void enterWriteNewScenesMode() {
    clearingBuffer();

    serialPrintOptimizer("You now in update scene mode");
    
    disableStoplightUntilUpdateSceneAndTurnOnDebugLight();

    readNewByteScenes();

    showScenesConfiguration();

    digitalWrite(DEBUG_LIGHT, 0);

    writeDataIntoStruct();

    enterWriteNewSceneModeState = false;

}

void clearingBuffer() {
    stopLightBuffer.scenePosition = 0;
    for (int i = 0; i < sizeof(stopLightBuffer.dataLight); i++) {
        stopLightBuffer.dataLight[i] = 0;
    }
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
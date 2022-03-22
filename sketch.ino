#include <EEPROM.h>

#define GREEN       4
#define YELLOW      5
#define RED         6
#define DEBUG_LIGHT 7

byte blinkDebugLightTimes = 4; 
volatile int debugWaitTime = 10000;

volatile unsigned long globalTimeBufferMillis = 0;

boolean enterWriteNewSceneModeState = false;


byte index = 0;

/*
* Сцены светофора.
* 
* Для понимания дальнейшего следует
* понимать концпецию сцены.
* В данной структуре имеет три параметра:
* @param scenePosition позиция сцены с которой следует начинать работу 
* @param isUpdated переменная, указывающая на то, был ли набор сцен 
* получен из режима обновления сцен
* @param dataLight[] сами сцены, где первый бит(считаем слева направо) 
* используется просто в качестве сохранения целостности байта,
* биты со второго по четвёртый отвечают за значение цвета светофора -
* красный, зелёный, жёлтый соостветственно. 
* Последний четыре бита отвечают за время работа в степени двойки
*/
struct StoplightScenes {    
    byte scenePosition = 0;
    boolean isUpdated = false;
    // 172, 128, 160, 128, 160, 128, 195, 208
    // Стандартный режим работы
    byte dataLight[16] = {
      //0b1RGYTime
        0b10101100,
        //Мигание
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
    boolean isUpdated = false;
    byte dataLight[16] = {
        // Yellow blinking
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
};

StoplightScenes stoplightScenes;

StopLightBuffer stopLightBuffer;

byte activeSpace = sizeof(stoplightScenes.dataLight);


/*
* Чтение информации с EEPROM. 
* 
* Считываем булевое значение в переменную isUpdated. 
* Данная переменная указывает на то была ли информация из EEPROM 
* записана ранее через UpdateSceneMode. 
* 
* Если переменная сцены не были записаны ранее, 
* тогда записывается стандартный набор сцен
* 
* В глобальную переменную index устанавливается последнее 
* задействованное положние сцены
* 
* Установка пинов в нужные режимы и предустановка нужных 
* значений для каждого цвета
*/
void setup() {

	Serial.begin(19200);

    EEPROM.get(1, stopLightBuffer.isUpdated);
    EEPROM.get(2, stopLightBuffer.dataLight);

    if (stopLightBuffer.isUpdated) {
        EEPROM.get(0, stoplightScenes.scenePosition);
        EEPROM.get(1, stoplightScenes.isUpdated);
        EEPROM.get(2, stoplightScenes.dataLight);
    } else if (stopLightBuffer.dataLight[0] != stoplightScenes.dataLight[0]) {
        EEPROM.put(0, stoplightScenes.scenePosition);
        EEPROM.put(1, stoplightScenes.isUpdated);
        EEPROM.put(2, stoplightScenes.dataLight);
    } 

    index = stoplightScenes.scenePosition;


    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(YELLOW, OUTPUT);
    pinMode(DEBUG_LIGHT, OUTPUT);

    pinMode(3, INPUT_PULLUP);
    attachInterrupt(1, changeEnterWriteNewSceneModeState, FALLING);

    digitalWrite(RED, bitRead(stoplightScenes.dataLight[stoplightScenes.scenePosition], 6));
    digitalWrite(GREEN, bitRead(stoplightScenes.dataLight[stoplightScenes.scenePosition], 5));
    digitalWrite(YELLOW, bitRead(stoplightScenes.dataLight[stoplightScenes.scenePosition], 4));

}

/*
* Поочерёдно обращаемся к элементам структуры сцен и 
* записываем последнее положение сцены в память EEPROM.
* 
* Проверка enterWriteNewSceneModeState используется для
* определения - нужно ли зайти в режим перезаписи сцен
*
*/
void loop() {

    dataLightParseAndTurnLights(stoplightScenes.dataLight[index]);
    index++;
    stoplightScenes.scenePosition = index;

    savePositionSceneToEEPROM();

    if (enterWriteNewSceneModeState) {
        enterWriteNewScenesMode();
    }
    
}

/*
* Сохранение позиции в память EEPROM.
*/
void savePositionSceneToEEPROM() {

    Serial.println("Scene position saved to EEPROM");
    EEPROM.update(0, stoplightScenes.scenePosition);

}

/*
* Парсинг сцены.
*
* С помощью bitRead() определяем в каком состоянии 
* находится бит, отвечающий за каждый цвет и 
* в зависимости от бита ставим значение через 
* digitalWrite()
*
* Определяем время действия сцены при помощи 
* timeCalculate(byte data)
*
* Всего сцен может быть максимум 16,
* но при условии если сцен меньше чем 16
* незанятые элементы равны 0. В таком 
* случае, если парсер дойдёт до первого такого
* элемента - переменная index = 0, а ожидания не
* будет
*/
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

/*
* Рассчёт времени.
*
* С помощью bitRead() считываем состояние
* битов отвечающих за время ожидания.
*
* Время ожидания - степень двойки.
*
* Умножаем полученную степень двойки 
* умножаем на 1000, чтобы получить секунды.

* @param data сцена
* @return необходимое время ожидания
*/
int timeCalculate(byte data) {
 
    byte countTimeBytes = 0;
    for (int i = 0; i <= 4; i++) {
        if (bitRead(data, i)) countTimeBytes++;
    }
    
    unsigned int timeWait = _BV(countTimeBytes) * 1000;
    return timeWait;

}

/*
* Улучшенная версия delay().
*
* Пока millis() - globalTimeBufferMillis не превысит
* время ожидания, цикл не остановится
*
* @param waitTime время ожидания
*/
void improvedDelay(unsigned int waitTime) {
    
    globalTimeBufferMillis = millis();
    boolean cooldownState = true;

    while (cooldownState) {
        if (millis() - globalTimeBufferMillis > waitTime) 
            cooldownState = false;
    }

}

/* 
* attachInterrupt метод.
* 
* При активации изменяет значение глобального 
* параметра enterWriteNewSceneModeState на true
* благодаря чему можно зайти в режим обновления сцен
*/
void changeEnterWriteNewSceneModeState() {
    enterWriteNewSceneModeState = true;
}

/*
* Режим обновления сцен.
*/
void enterWriteNewScenesMode() {
    
    clearingBuffer();

    serialPrintOptimizer("You now in update scene mode");
    
    disableStoplightUntilUpdateSceneAndTurnOnDebugLight();

    readNewByteScenes();

    showScenesConfiguration();

    digitalWrite(DEBUG_LIGHT, 0);

    writeDataIntoStruct();

    saveStructIntoEEPROM();

    enterWriteNewSceneModeState = false;

}

/*
* Очищает буферную структуру
* идентичную основной структуре сцен
*/
void clearingBuffer() {
    
    stopLightBuffer.scenePosition = 0;
    for (int i = 0; i < sizeof(stopLightBuffer.dataLight); i++) {
        stopLightBuffer.dataLight[i] = 0;
    }

}

/*
* Отключает основные светодиоды и включает светодиод режма обновления
*/
void disableStoplightUntilUpdateSceneAndTurnOnDebugLight() {
    
    digitalWrite(RED, 0);
    digitalWrite(GREEN, 0);
    digitalWrite(YELLOW, 0);

    digitalWrite(DEBUG_LIGHT, 1);

}


/*
* Считываение новых сцен.
*
* На вход через Serial подаются новые сцены, которые 
* записываются в буферную структуру
*
* Если колличество всех сцен стало максимальным или
* по истичении времени бездействия происходит выход 
* из режима записи новых сцен
*/
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

/*
* Более оптимизированный метод вывода строк
* @param string выводимая строка
*/
void serialPrintOptimizer(String string) {
    
    Serial.flush();
    for (int i = 0; i < string.length(); i++) {
        Serial.print(string.charAt(i));
        if (i % 3 == 0) Serial.flush();
    }
    Serial.println("");

}

/*
* Показывает итоговый вариант кофигурации сцен
*/
void showScenesConfiguration() {
    
    Serial.println("");
    Serial.println("New scenes configuration: ");
    for (int i = 0; i < sizeof(stopLightBuffer.dataLight); i++) {
        Serial.println(stopLightBuffer.dataLight[i]);
    }
    Serial.println();

}

/*
* Записывает новую конфигурацию в основную структуру
*/
void writeDataIntoStruct() {
    stoplightScenes.isUpdated = true;
    stoplightScenes.scenePosition = 0;
    for (int i = 0; i < sizeof(stoplightScenes.dataLight); i++) {
        stoplightScenes.dataLight[i] = stopLightBuffer.dataLight[i];
    }
    serialPrintOptimizer("Data transfered to main struct");

}

/*
* Сохранение обновлённой сцены в EEPROM
*/
void saveStructIntoEEPROM() {
    EEPROM.put(0, stoplightScenes.scenePosition);
    EEPROM.put(1, stoplightScenes.isUpdated);
    EEPROM.put(2, stoplightScenes.dataLight);
}
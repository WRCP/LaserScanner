//  Лазерное сканирование космических тел
//  Прототип
//  Создано 12/03/2019

//  В основе - проект FabScan - http://hci.rwth-aachen.de/fabscan
//  Created by Francis Engelmann on 7/1/11.
//  Copyright 2011 Media Computing Group, RWTH Aachen University. All rights reserved.
//  https://codebender.cc/sketch:186175

#include <Stepper.h>  //  Подключаем библиотеку 

const int stepsPerRevolution = 200; // Установить количество шагов на поворот вала ШД
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);  // Подключить управление ШД (шилд) к пинам 8-11

#define LIGHT_PIN A3  //  Свет
#define LASER_PIN A4  //  Лазер
#define MS_PIN    A5  //  Микрошаги

#define ENABLE_PIN_0  2   //  ШД 1, ШД оси астероида
#define STEP_PIN_0    3
#define DIR_PIN_0     4

#define ENABLE_PIN_1  5   //  ШД 2, ШД лазера
#define STEP_PIN_1    6
#define DIR_PIN_1     7

#define ENABLE_PIN_2  11  //  ШД 3, не используется
#define STEP_PIN_2    12
#define DIR_PIN_2     13

#define ENABLE_PIN_3  A0  //  ШД 4, не используется
#define STEP_PIN_3    A1
#define DIR_PIN_3     A2

//  Протокол обмена информацией через последовательный порт с FabScan:
//    Принимается один байт, чтобы назначить необходимое действие.
//    Если действие унарное (например, выключение света) необходим только один байт.
//    Если мы хотим дать указание ШД повернуться, второй байт используется для указания количества шагов.

//  Первый байт
#define TURN_LASER_OFF      200
#define TURN_LASER_ON       201
#define PERFORM_STEP        202   // Требует второй байт TURN_TABLE_STEPS
#define SET_DIRECTION_CW    203
#define SET_DIRECTION_CCW   204
#define TURN_STEPPER_ON     205
#define TURN_STEPPER_OFF    206
#define TURN_LIGHT_ON       207   // Требует второй байт LIGHT_INTENSITY
#define TURN_LIGHT_OFF      208
#define ROTATE_LASER        209
#define FABSCAN_PING        210
#define FABSCAN_PONG        211
#define SELECT_STEPPER      212   // Требует второй байт STEPPER_ID
#define LASER_STEPPER       11
#define TURNTABLE_STEPPER   10

//  Возможные значения byteType
#define ACTION_BYTE         1    // Нормальный байт, первое новое действие
//  Для выбора второго байта byteType
#define LIGHT_INTENSITY     2    // Изменение освещения в  соответствии с вторым байтом
#define TURN_TABLE_STEPS    3    // Вращение ШД оси астероида в соответствии с вторым байтом
#define LASER1_STEPS        4   
#define LASER2_STEPS        5
#define LASER_ROTATION      6
#define STEPPER_ID          7     // Установление текущего ШД в соответствии с вторым байтом 

int incomingByte = 0; //  Входной байт
int byteType = 1;     //  Тип входного байт
int currStepper;      //  Текущий ШД

//  Поворот текущего ШД на один шаг

void step() { //  Используется для шилда L298N
  myStepper.setSpeed(1);
  myStepper.step(1);
}

//void step() { //  Используется для шилда FabScan
//  if(currStepper == TURNTABLE_STEPPER) {
//    digitalWrite(STEP_PIN_0, LOW);
//  }else if(currStepper == LASER_STEPPER) {
//    digitalWrite(STEP_PIN_1, LOW);
//  }
//  delay(3);
//  if(currStepper == TURNTABLE_STEPPER) {
//    digitalWrite(STEP_PIN_0, HIGH);
//  }else if(currStepper == LASER_STEPPER) {
//    digitalWrite(STEP_PIN_1, HIGH);
//  }
//  delay(3);
//}

//  Включение текущего ШД <count> раз
void step(int count) {
  for (int i = 0; i < count; i++) {
    Serial.print((String) i + " ");
    step();  
  }
  Serial.println();  
}

void setup() {
  Serial.begin(9600); // Установить соединение через последовательный порт
  
  pinMode(LASER_PIN, OUTPUT); 
  pinMode(LIGHT_PIN, OUTPUT);

  pinMode(MS_PIN, OUTPUT);
  digitalWrite(MS_PIN, HIGH);  // HIGH для микрошагов, LOW без микрошагов

  //  ШД 1, ШД оси астероида
  pinMode(ENABLE_PIN_0, OUTPUT);  // 2
  pinMode(DIR_PIN_0, OUTPUT);     // 3
  pinMode(STEP_PIN_0, OUTPUT);    // 4

  //  ШД 2, ШД лазера
  pinMode(ENABLE_PIN_1, OUTPUT);  // 5
  pinMode(DIR_PIN_1, OUTPUT);     //
  pinMode(STEP_PIN_1, OUTPUT);    //

  //  ШД 3, не используется
  pinMode(ENABLE_PIN_2, OUTPUT);
  pinMode(DIR_PIN_2, OUTPUT);
  pinMode(STEP_PIN_2, OUTPUT);

  //  ШД 4, не используется
  pinMode(ENABLE_PIN_3, OUTPUT);
  pinMode(DIR_PIN_3, OUTPUT);
  pinMode(STEP_PIN_3, OUTPUT);

  // Отключить все ШД при запуске
  digitalWrite(ENABLE_PIN_0, HIGH);  // HIGH - выключить
  digitalWrite(ENABLE_PIN_1, HIGH);  // HIGH - выключить
  digitalWrite(ENABLE_PIN_2, HIGH);  // LOW - включить
  digitalWrite(ENABLE_PIN_3, HIGH);  // LOW - включить

  digitalWrite(LIGHT_PIN, LOW); //  Включить свет

  digitalWrite(LASER_PIN, HIGH);  //  Моргнуть лазером
  delay(200);
  digitalWrite(LASER_PIN, LOW);
  delay(200);
  digitalWrite(LASER_PIN, HIGH);
  delay(200);
  digitalWrite(LASER_PIN, LOW);
  delay(200);
  digitalWrite(LASER_PIN, HIGH);
  delay(200);
  digitalWrite(LASER_PIN, LOW);
  delay(200);
  digitalWrite(LASER_PIN, HIGH);  //  Включить лазер
  
//  Serial.write(300);
  Serial.write(FABSCAN_PONG); //  Отправить PONG на FabScan (установлен на компьютере): настройка завершена, ожидается сигнал от FabScan

  currStepper = TURNTABLE_STEPPER;  //  Установить текущий ШД - Ось астероида
}

void loop() {
  if (Serial.available() > 0) { //  Если в буфере последовательного порта есть доступные данные
    incomingByte = Serial.read(); //  Считываем байт
    Serial.print((String) incomingByte + " ");    
    switch (byteType) {
      //  Обработка первого байта
      case ACTION_BYTE: 
        switch (incomingByte) {   

          // Лазер
          case TURN_LASER_OFF:
            digitalWrite(LASER_PIN, LOW);    // Выключить лазер
            Serial.println("Laser Off");
            break;
          case TURN_LASER_ON:
            digitalWrite(LASER_PIN, HIGH);   // Включить лазер
            Serial.println("Laser On");
            break;
          case ROTATE_LASER: // Неиспользуется
            byteType = LASER_ROTATION;
            break;

          case PERFORM_STEP:  //  Читать второй байт и выполнить требуемое вращение ШД оси астероида
            byteType = TURN_TABLE_STEPS;
            Serial.print("Make steps: ");
            break;

          case SET_DIRECTION_CW: // Установить направление вращения ШД по часовой стрелке
            if (currStepper == TURNTABLE_STEPPER) {
              digitalWrite(DIR_PIN_0, HIGH);
            } else if (currStepper == LASER_STEPPER) {
              digitalWrite(DIR_PIN_1, HIGH);
            }
            Serial.println("CW Direction");
            break;
          case SET_DIRECTION_CCW: // Установить направление вращения ШД против часовой стрелки
            if (currStepper == TURNTABLE_STEPPER) {
              digitalWrite(DIR_PIN_0, LOW);
            } else if (currStepper == LASER_STEPPER) {
              digitalWrite(DIR_PIN_1, LOW);
            }
            Serial.println("CCW Direction");
            break;

          case TURN_STEPPER_ON:
            if (currStepper == TURNTABLE_STEPPER) {
              digitalWrite(ENABLE_PIN_0, LOW);
            } else if (currStepper == LASER_STEPPER) {
              digitalWrite(ENABLE_PIN_1, LOW);
            }
            break;
          case TURN_STEPPER_OFF:
            if (currStepper == TURNTABLE_STEPPER) {
              digitalWrite(ENABLE_PIN_0, HIGH);
            } else if (currStepper == LASER_STEPPER) {
              digitalWrite(ENABLE_PIN_1, HIGH);
            }
            break;

          case TURN_LIGHT_ON:  //  Читать второй байт и установить требуемое освещение
            byteType = LIGHT_INTENSITY;
            break;
          case TURN_LIGHT_OFF:
            digitalWrite(LIGHT_PIN, LOW); //  Выключить освещение
            break;

          case FABSCAN_PING:
            delay(1);
            Serial.println("PONG");
            Serial.write(FABSCAN_PONG);   // Отправить 211
            break;

          case SELECT_STEPPER:  //  Читать второй байт и установить требуемый текущий ШД
            byteType = STEPPER_ID;
            Serial.print("Default Stepper: ");
            break;
          
          default:
            Serial.println("Unknown");
        }
        break;
      
      //  Обработка второго байта по требованию
      
      case LIGHT_INTENSITY: //  Установить освещение в соответствие с <incomingByte>   
        analogWrite(LIGHT_PIN, incomingByte);
        byteType = ACTION_BYTE;  // Сброс byteType
        break;

      case TURN_TABLE_STEPS:  // Поворот ШД оси астероида на <incomingByte> шагов
        Serial.println();
        Serial.println("Stepper start");
        step(incomingByte);
        Serial.println("Stepper stop");
        byteType = ACTION_BYTE;  // Сброс byteType
        break;

      case STEPPER_ID:  // Установитьтекущий ШД в соответствие с <incomingByte> 
        Serial.write(incomingByte);
        currStepper = incomingByte; //Переключить текущий ШД
        byteType = ACTION_BYTE;  // Сброс byteType
        break;
    }
  }
}

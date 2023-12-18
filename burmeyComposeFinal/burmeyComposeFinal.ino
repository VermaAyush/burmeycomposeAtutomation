#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "OneButton.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Tone.h>
#include <EEPROM.h>

#define Btn_up 10
#define Btn_down 11
#define Btn_ok 12
#define buzzer 13
#define IrPin 2
#define tempPin 3
#define mixerPin 5
#define pumpPin 4
#define coverPin 6
#define moistPin A3


#define AirValue 533
#define WaterValue 220

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS tempPin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

LiquidCrystal_I2C lcd(0x27, 20, 4);

Tone tone2;

volatile bool Setting = false;
const int DTMF_freq1[] = { 1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1477 };
const int DTMF_freq2[] = { 941, 697, 697, 697, 770, 770, 770, 852, 852, 852 };

bool HOME = true;
bool coverState = false;
bool coverisopend = false;
bool doOnce = true;
volatile unsigned long startTime, stopTime;
unsigned long rotationTime;
unsigned long lastCoverTime;
unsigned long lastUpdateTime;
unsigned long previousPumpMilli = 0;

volatile bool statePin12 = LOW;
volatile bool statePin11 = LOW;
volatile bool statePin10 = LOW;
volatile bool isrCalled = true;
static bool pumpStat = false;

float temperature = 0.0;
int moisture = 0;

struct Settings {
  bool isEnable;
  float lowerTemp;
  float higherTemp;
  float lowerMoist;
  float higherMoist;
  int coverTime;
  unsigned long  mannualPumpOnTime;
  unsigned long  mannualPumpOffTime;
  bool save;
  bool exit;
};
bool isRunning = false;
bool isclosen = false;

int selectItem = 0;

Settings setting = { false, 25, 35, 60, 80, 3000, 1000,1000, false, false };

void playDTMF(uint8_t number, long duration) {
  tone2.play(DTMF_freq1[number], duration);
}

void setup() {
 // EEPROM.begin();
 // EEPROM.put(0, setting);

  pinMode(buzzer, OUTPUT);
 tone2.begin(buzzer);
  tone2.play(500,1000);
  EEPROM.get(0,setting);
  setting.save = false;
  delay(1000);
  // put your setup code here, to run once:
  pinMode(Btn_up, INPUT);
  pinMode(Btn_down, INPUT);
  pinMode(Btn_ok, INPUT);
  pinMode(IrPin, INPUT_PULLUP);
  pinMode(mixerPin, OUTPUT);
  pinMode(coverPin, OUTPUT);
  pinMode(moistPin, INPUT);

  if (digitalRead(Btn_ok)) {
    setting.isEnable = true;
    pinMode(Btn_ok, OUTPUT);
    digitalWrite(Btn_ok, HIGH);
    pinMode(Btn_up, OUTPUT);
    digitalWrite(Btn_up, HIGH);
    pinMode(Btn_down, OUTPUT);
    digitalWrite(Btn_down, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(Btn_ok, LOW);
    digitalWrite(Btn_up, LOW);
    digitalWrite(Btn_down, LOW);
    digitalWrite(buzzer, LOW);

  } else {
    setting.isEnable = false;
  }
  pinMode(Btn_up, INPUT);
  pinMode(Btn_down, INPUT);
  pinMode(Btn_ok, INPUT);
  digitalWrite(pumpPin, OUTPUT);
  digitalWrite(pumpPin, pumpStat);
  //  pinMode(pumpPin, OUTPUT);
  // Enable Pin Change Interrupts for the respective pins
  PCICR |= (1 << PCIE0);                                    // Enable PCINT for Port B
  PCMSK0 |= (1 << PCINT4) | (1 << PCINT2) | (1 << PCINT3);  // Enable PCINT for pins 12, 10, 11

  sei();
  // Enable global interrupts
// tone1.begin(pumpPin);
  
  lcd.init();
  lcd.backlight();
  if (!setting.isEnable) {

    lcd.clear();
    lcd.setCursor(6, 1);
    lcd.print("welcome");

    if (!digitalRead(IrPin)) {
      digitalWrite(coverPin, HIGH);
      delay(3000);
    }

    digitalWrite(coverPin, HIGH);
    coverState = false;
  }
  Serial.begin(9600);
  Serial.println("begin");
  sensors.begin();

  lcd.clear();
  lcd.print("Initalizing");

  if (setting.isEnable) {
    uint8_t phone_number[] = { 8, 6, 7, 5, 3, 0, 9 };

    for (int i = 0; i < sizeof(phone_number); i++) {
      Serial.print(phone_number[i], 10);
      Serial.print(' ');
      playDTMF(phone_number[i], 500);
      delay(100);
    }
    lcd.clear();
    lcd.print("setting page opened");
  }
}
//void updateScreen(){
//   if(menuEn){ //display list of menus

//   }else{

//   }
// }

void loop() {
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  moisture = map(analogRead(moistPin), AirValue, WaterValue, 0, 100);
  if (moisture >= 100) {
    moisture = 100;
  } else if (moisture <= 0) {
    moisture = 0;
  }
  if (!setting.isEnable) {
    if (millis() - lastUpdateTime > 300) {

      lcd.clear();
      lcd.print("Temp:");
      lcd.print(temperature);
      lcd.write(0xDf);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("moist:");
      lcd.print(moisture);
      lcd.print("%");
      lcd.setCursor(0, 2);
      lcd.print("pump   mixer   cover");
      lcd.setCursor(0, 3);
      lcd.print(pumpStat ? "ON" : "OFF");
     lcd.setCursor(8, 3);
      lcd.print(digitalRead(mixerPin) ? "ON" : "OFF");
      lcd.setCursor(13, 3);
      lcd.print(coverState ? "closed" : "open");
      lastUpdateTime = millis();
    }
    if (temperature < setting.lowerTemp) {
      if (coverState) {
        coverState = false;
        lastCoverTime = millis();
        digitalWrite(coverPin, HIGH);
      }
      if (!digitalRead(IrPin)) {
        digitalWrite(coverPin, HIGH);
        delay(3000);
      }
      // coverState = false;
      // if (digitalRead(IrPin)) {
      //   digitalWrite(coverPin, HIGH);
      //   if (doOnce) {
      //     lastCoverTime = millis();
      //     doOnce = false;
      //   }
      // }
      //Serial.println("please open cover");
    }
    if (temperature > setting.higherTemp) {
      //   coverState = true;
     // Serial.println("please close cover");
      coverState = true;
    }
    if (moisture < setting.lowerMoist ) {
      if (pumpStat == HIGH){
          if(millis() -  previousPumpMilli >= setting.mannualPumpOnTime){
            pumpStat = LOW;
            previousPumpMilli = millis();
          }
           analogWrite(pumpPin, pumpStat ? 150 : 0);
      
      }
      else 
      { 
        if(millis()- previousPumpMilli >= setting.mannualPumpOffTime){
          pumpStat = HIGH;
          previousPumpMilli = millis();
        } analogWrite(pumpPin, pumpStat ? 150 : 0);
      

      
      }
      
     // digitalWrite(pumpPin, pumpStat);
    } else if (moisture > setting.higherMoist ) {
        pumpStat = LOW;
         analogWrite(pumpPin, pumpStat ? 150 : 0);
    }
    
  } else {

    // if (millis() - lastUpdateTime > 300) {
    //   updateScreen();
    //   lastUpdateTime = millis();
    // }
    if(isrCalled){
      updateScreen();
      isrCalled = false;
    }
  }


  if (coverState) {
    digitalWrite(coverPin, HIGH);
    if (!digitalRead(IrPin)) {
      digitalWrite(coverPin, LOW);
      //coverisopend = false;
      delay(100);
    }
  } else {


    if (millis() - lastCoverTime > setting.coverTime) {
      digitalWrite(coverPin, LOW);
      doOnce = true;
    }
  }



  // Serial.println(rotationTime);
}


ISR(PCINT0_vect) {  // Pin change interrupt vector for PCINT2 (pin 10), PCINT3 (pin 11), PCINT4 (pin 12)
isrCalled = true;
  if (bitRead(PINB, PINB2)) {
    Serial.println("Pin 10 state: HIGH");
    if (setting.isEnable) {
      switch (selectItem) {
        case 0:
          selectItem = 1;
          break;
        case 1:
          setting.lowerTemp++;
          break;
        case 2:
          setting.higherTemp++;
          break;
        case 3:
          setting.lowerMoist++;
          break;
        case 4:
          setting.higherMoist++;
          break;
        case 5:
          setting.coverTime =  setting.coverTime+50;
          break;
          ;
        case 6:
          setting.mannualPumpOnTime = setting.mannualPumpOnTime + 10;
          break;
        case 7:
          setting.mannualPumpOffTime = setting.mannualPumpOffTime + 50;
          break;
        case 8:
          setting.save = true;
          if(setting.save){
            EEPROM.put(0, setting);
            delay(1000);
          }
          break;
      }
    } else {
      coverState = !coverState;
      if (!coverState) {
        digitalWrite(coverPin, HIGH);
        lastCoverTime = millis();
      }
    }
  } else {
    Serial.println("Pin 10 state: LOW");
  }

  if (bitRead(PINB, PINB3)) {

    Serial.println("Pin 11 state: HIGH");
    if (setting.isEnable) {
      selectItem++;
      if (selectItem > 8) {
        selectItem = 1;
      }
    } else {
      digitalWrite(mixerPin, !digitalRead(mixerPin));
    }
  } else {
    Serial.println("Pin 11 state: LOW");
  }

  if (bitRead(PINB, PINB4)) {
    Serial.println("Pin 12 state: HIGH");
    if (setting.isEnable) {
      switch (selectItem) {
    
        case 1:
          setting.lowerTemp--;
          break;
        case 2:
          setting.higherTemp--;
          break;
        case 3:
          setting.lowerMoist--;
          break;
        case 4:
          setting.higherMoist--;
          break;
        case 5:
          setting.coverTime = setting.coverTime -50;
          break;
          ;
        case 6:
          setting.mannualPumpOnTime= setting.mannualPumpOnTime-10;
          break;
        case 7:
          setting.mannualPumpOffTime = setting.mannualPumpOffTime - 50;
          break;
        case 8:
          setting.save = false;
          break;
      }
      }
      else {
        analogWrite(pumpPin, 170);
      }

    } else {
      analogWrite(pumpPin, 0);
      Serial.println("Pin 12 state: LOW");
    }

    beep();
  }
  void beep() {
    if (!tone2.isPlaying()) {

      tone2.play(50, 100);
    }
  }

  void updateScreen() {

    switch (selectItem) {

      case 1:
        lcd.clear();
        lcd.print(" Curr Temp ");
        lcd.print(temperature);
        lcd.print((char)223);
        lcd.print("C");
        lcd.setCursor(0, 2);
        lcd.print(">low thres  ");
        lcd.print(setting.lowerTemp);
        lcd.setCursor(0, 3);
        lcd.print(" high thers ");
        lcd.print(setting.higherTemp);
        break;
      case 2:
        lcd.clear();
        lcd.print(" CurrentTemp ");
        lcd.print(temperature);
        lcd.print((char)223);
        lcd.print("C");
        lcd.setCursor(0, 2);
        lcd.print(" low thres   ");
        lcd.print(setting.lowerTemp);
        lcd.setCursor(0, 3);
        lcd.print(">high thers  ");
        lcd.print(setting.higherTemp);
        break;
      case 3:
        lcd.clear();
        lcd.print(" currentMoist ");
        lcd.print(moisture);
        lcd.print("%");
        lcd.setCursor(0, 2);
        lcd.print(">low thres  ");
        lcd.print(setting.lowerMoist);
        lcd.setCursor(0, 3);
        lcd.print(" high thres ");
        lcd.print(setting.higherMoist);
        break;

      case 4:
        lcd.clear();
        lcd.print(" currentMoist ");
        lcd.print(moisture);
        lcd.print("%");
        lcd.setCursor(0, 2);
        lcd.print(" low thres  ");
        lcd.print(setting.lowerMoist);
        lcd.setCursor(0, 3);
        lcd.print(">high thres ");
        lcd.print(setting.higherMoist);
        break;
      case 5:
        lcd.clear();
        lcd.print(" Cover Opening");
        lcd.setCursor(0, 1);
        lcd.print(">time  ");
        lcd.print(setting.coverTime);
        lcd.print("ms");
        lcd.setCursor(0, 2);
        lcd.print(" Pump On");
        lcd.setCursor(0, 3);
        lcd.print(" time  ");
        lcd.print(setting.mannualPumpOnTime);
        lcd.print("ms");

        break;
      case 6:
        lcd.clear();
        lcd.print(" Cover Opening");
        lcd.setCursor(0, 1);
        lcd.print(" time  ");
        lcd.print(setting.coverTime);
        lcd.print("ms");
        lcd.setCursor(0, 2);
        lcd.print(" Pump On");
        lcd.setCursor(0, 3);
        lcd.print(">time  ");
        lcd.print(setting.mannualPumpOnTime);
        lcd.print("ms");
        break;

      case 7:
        lcd.clear();
        lcd.print(" Pump On");
        lcd.setCursor(0, 1);
        lcd.print(" time  ");
        lcd.print(setting.mannualPumpOnTime);
        lcd.print("ms");
        lcd.setCursor(0, 2);
        lcd.print(" Pump Off");
        lcd.setCursor(0, 3);
        lcd.print(">time  ");
        lcd.print(setting.mannualPumpOffTime);
        lcd.print("ms");
        break;

      
      case 8:
        lcd.clear();
        lcd.print("turn off for default");
        lcd.setCursor(0, 3);
        lcd.print(" >save ");
        lcd.print(setting.save ? "yes" : "no");
        break;
    }
  }
#include<SoftwareSerial.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include "pitches.h"

HX711_ADC LoadCell(6, 7); //LoadCell(DT,SCK)
LiquidCrystal_I2C lcd(0x27,16,2); //lcd(Address,columns,rows)
RTC_DS3231 rtc;
SoftwareSerial bt(3,2); //bt(RX,TX)

void timedisplay(bool);
void Btcheck(void);
void Isalarm(void);
void WMode(bool);
void fullcalibrate(void);
void changeSavedCalFactor(void);
void BTSet_tare(void);
void BT_Calibrate(void);
String Btgetmsg(void);
char Btrcv_char(void);

long alarm[25]={43200, 43500, 43800};
boolean _tare = true;

const int calVal_eepromAdress = 0;
unsigned long t = 0;

const long msg_timeout = 5;
long counter = 0;

const int WTrigger = 1000;
const int W_Ala_off = 30000;
const long Freq_change_delay = 180000;

short melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
short noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  bt.begin(9600);
  lcd.init(); 
  lcd.backlight();
  pinMode(4,INPUT);

  if (! rtc.begin()) {
    Serial.println(F("RTC Connection Issue"));
    Serial.flush();
    while (1) delay(10);
  }
  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, setting the system time!"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  //implicit setting
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); // explicit setting (YYYY,MM,DD,HR,MIN,SEC)
  }

    
  LoadCell.begin();
  LoadCell.start(2000, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println(F("Timeout, check MCU>HX711 wiring and pin designations"));
    lcd.setCursor(1,0);
    lcd.print(F("ERROR    ERROR"));
    lcd.setCursor(2,1);
    lcd.print(F("HX711 wiring"));
    while (1);
  }
  else {
    LoadCell.setCalFactor(1.0);
    Serial.println(F("Startup of Load cell is complete"));
  }
  while (!LoadCell.update());
  if(EEPROM.read(calVal_eepromAdress) != 0){    //Should be ==
    fullcalibrate();
    }


  Serial.println(F("Setup Finished"));
  lcd.setCursor(1,0);
  lcd.print(F("Setup Properly"));
  lcd.setCursor(2,1);
  lcd.print(F("Initialised"));
  delay(2000);
  

}

void loop() {
  if(digitalRead(4) == 1)
  {
    if(counter >= msg_timeout){
      bt.println(F("Hello User! Send '*' to initiate connection"));
      counter = 0;
    }
    counter = counter + 1;
    Btcheck();
  }
  timedisplay(1);
  delay(1000);
//  Isalarm();
  timedisplay(0);
  delay(1000);
//  WMode(0);
//  delay(50);

}

/*--------------------------------------------------------------------------------------------------------------------------------------------*/
/*-----------------------Function Definition----------------------------------------Function Definition---------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------------------------------*/


void timedisplay(bool sw){  // Generic display in LCD screen the time date and temperature in a scrolling manner
    DateTime now = rtc.now();
    if(sw){
      Serial.print(now.day(), DEC);
      Serial.print(F("/"));
      Serial.print(now.month(), DEC);
      Serial.print(F("/"));
      Serial.print(now.year(), DEC);
      Serial.print(F("\t"));
      Serial.print(now.hour(), DEC);
      Serial.print(F(":"));
      Serial.print(now.minute(), DEC);
      Serial.print(F(":"));
      Serial.println(now.second(), DEC);
      
      for(short i=0; i<=16; i++){     // Performs the shifting function
        lcd.print(F(" "));
        lcd.setCursor(i,0);
        lcd.print(F(" "));
        lcd.setCursor(i,1);
        delay(20);
      }
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("Date:"));
      lcd.print(now.day(),DEC);
      lcd.print(F("/"));
      lcd.print(now.month(),DEC);
      lcd.print(F("/"));
      lcd.print(now.year(),DEC);
      lcd.setCursor(0,1);
      lcd.print(F("Time:"));
      lcd.print(now.hour(),DEC);
      lcd.print(F(":"));
      lcd.print(now.minute(),DEC);
      lcd.print(F(":"));
      lcd.print(now.second(),DEC);
    }
    else{
      for(short i=16; i>=0; i--){     // Performs the shifting function
        lcd.print(F(" "));
        lcd.setCursor(i,0);
        lcd.print(F(" "));
        lcd.setCursor(i,1);
        delay(20);
      }
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("Room Temperature: "));
      lcd.setCursor(4,1);
      lcd.print(rtc.getTemperature());
      lcd.print(F("C"));
    }
  }

void Isalarm(){
  DateTime now = rtc.now();
  int TimeCurr = (int) ((now.hour()*3600)+(now.minute()*60)+(now.second()));
  Serial.println(TimeCurr);
  for(int i =0; i<=((int)(sizeof(alarm)/sizeof(alarm[0]))-1); i++)
  {
    if((alarm[i]-2)<=TimeCurr && (alarm[i]-2)>TimeCurr)   // window of 4 seconds
    {    
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("PLEASE STAND ON"));
      lcd.setCursor(0,1);
      lcd.print(F("    THE SCALE    "));
      long t_start = millis();
      while(LoadCell.getData() <= W_Ala_off)
      {
        if(millis()-t_start <= Freq_change_delay)
        {
          for (int thisNote = 0; thisNote < 8; thisNote++) 
          {
            int noteDuration = 1000 / noteDurations[thisNote];    // to calculate the note duration, take one second divided by the note type.(quarter note = 1000 / 4, eighth note = 1000/8, etc.)
            tone(5, melody[thisNote], noteDuration);              //tone(pin_number, Note_Frequency, Note_Duration);
            delay(noteDuration * 1.30);                           // to distinguish the notes, set a minimum time between them. 30% seems to work well
            noTone(5);                                            // stop the tone playing
          }
        }
        else
        {
          tone(5,880,1000/8); //play the note "A5" (LA5)
          delay(200);
          tone(5,698,1000/8); //play the note "F6" (FA5)
          delay(200);
        }
      }
    }
  }
}

void WMode(boolean avg_mode){
  float wsample=0;
  uint8_t counter = 0;
  if(avg_mode){
    long t_start = millis();
    while(millis() - t_start < 150){
      wsample = LoadCell.getData();
      counter = counter +1;
      delay(3);
    }
    wsample = wsample/counter;
  }
  else{
    wsample = LoadCell.getData();
  }
  if(wsample>=WTrigger){
    Serial.print(F("Entering Weighing Mode"));
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F(" WEIGHING MODE"));
    delay(1500);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Measured Weight"));
    lcd.setCursor(2,1);
    lcd.print(F("KG = "));
    while(1){
      lcd.setCursor(7,1);
      float i = LoadCell.getData();
      lcd.print(i/1000);
      Serial.print(F("Measured Weight = "));
      Serial.println(i);
      if(i<50){
        Serial.print(F("Exiting Weighing Mode"));
        lcd.clear();
        break;
      }
    }
  }
}

void Btcheck(){
  char BT_RCV ='#';
  boolean _resume = false; 
  BT_RCV = Btrcv_char();
  if(BT_RCV == '*'){
    bt.println(F("To Add Alarm, type '1'"));
    bt.println(F("To Delete Alarm, type '2'"));
    bt.println(F("To Show All Alarms, type '3'"));
    bt.println(F("To Tare The Scale, type '4'"));
    bt.println(F("To Add A New Calibration Value, type '5'"));
    bt.println(F("To Perform Full Calibration, type '6'"));
    bt.println(F("To Exit this calibration, type '*'"));
    while (_resume == false) {
      BT_RCV = '#';
      BT_RCV = Btrcv_char();
      switch(BT_RCV){
        case '*':
          _resume = true;
          break;
        case '1':
          //
          _resume = true;
          break;
        case '2':
          //
          _resume = true;
          break;
        case '3':
          bt.print(F("The saved Alarms are:"));
          for(int x=0; x<=((int)(sizeof(alarm)/sizeof(alarm[0]))-1); x++){
            bt.print((int)(alarm[x]/3600));
            bt.print(F(":"));
            bt.print((int)((alarm[x]/60)%60));
            bt.print(F(":"));
            bt.print((int)((alarm[x]%60)));
          }
          _resume = true;
          break;
        case '4':
          BTSet_tare();
          _resume = true;
          break;
        case '5':
          BT_Calibrate();
          _resume = true;
          break;  
        case '6':
          fullcalibrate();
          _resume = true;
          break;
        default:
          _resume = false;
          break;
                       
      }
      _resume = true;
    }
  }
}





    
char Btrcv_char(){
  bt.listen();
  String fetched_string = "";
  fetched_string = Btgetmsg();
  if(fetched_string.startsWith("*")){return('*');}
  else if(fetched_string.startsWith("1")){return('1');}
  else if(fetched_string.startsWith("2")){return('2');}
  else if(fetched_string.startsWith("3")){return('3');}
  else if(fetched_string.startsWith("4")){return('4');}
  else if(fetched_string.startsWith("5")){return('5');}
  else if(fetched_string.startsWith("6")){return('6');}
  else if(fetched_string.startsWith("7")){return('7');}
  else if(fetched_string.startsWith("8")){return('8');}
  else if(fetched_string.startsWith("9")){return('9');}
  else if(fetched_string.startsWith("0")){return('0');}
  else{return('#');} // invalid case
    
}

String Btgetmsg(){
  String BTstring ="";
  if (bt.available() > 0)
  {
    while(bt.available()) 
    {
      char c = bt.read();  
      BTstring += c; 
    }
    bt.print(F("You typed:"));
    bt.println(BTstring);
    Serial.println(BTstring);
    return(BTstring);
  }
  return("");
}


void BTSet_tare(){
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print(F("TARE"));
  lcd.setCursor(0,1);
  lcd.print(F("   WAITING...  "));
  bt.println(F("Send 't' to start tare"));
  boolean _resume = false;
  while (_resume == false) 
  {
    LoadCell.update();
    if (bt.available() > 0) 
    {
      if (bt.available() > 0) 
      {
        char inByte = bt.read();
        if (inByte == 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) 
    {
      Serial.println(F("Bluetooth Tare complete"));
      bt.println(F("Bluetooth Tare complete"));
      lcd.setCursor(0,1);
      lcd.print(F("    COMPLETE  "));
      delay(1000);
    }
  }
}

void BT_Calibrate(){
  bt.println(F("place a known mass on the loadcell."));
  bt.println(F("Then send the weight of this mass (i.e. 100.0) from serial monitor."));
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print(F("BLUETOOTH"));
  lcd.setCursor(0,1);
  lcd.print(F("MASS CALIBRATION"));

  float known_mass = 0;
  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (bt.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        bt.print(F("Known mass is: "));
        bt.println(known_mass);
        _resume = true;
      }
    }
  }
  LoadCell.refreshDataSet();
  LoadCell.setCalFactor(LoadCell.getNewCalibration(known_mass));
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print(F("CALIBRATION"));
  lcd.setCursor(1,1);
  lcd.print("VALUE UPDATED");
  delay(3000);
}


void fullcalibrate() {
  Serial.println(F("***"));
  Serial.println(F("Start calibration:"));
  Serial.println(F("Place the load cell an a level stable surface."));
  Serial.println(F("Remove any load applied to the load cell."));
  Serial.println(F("Send 't' from serial monitor to set the tare offset."));
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("FULL CALIBRATION"));
  lcd.setCursor(0,1);
  lcd.print(F("   IN PROGRESS  "));
  delay(3000);
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print(F("TARE"));
  lcd.setCursor(2,1);
  lcd.print(F("IN PROGRESS"));

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      lcd.setCursor(4,1);
      lcd.print(F("    COMPLETE  "));
      delay(1000);
      _resume = true;
    }
  }

  Serial.println(F("Now, place your known mass on the loadcell."));
  Serial.println(F("Then send the weight of this mass (i.e. 100.0) from serial monitor."));
  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print(F("KNOWN"));
  lcd.setCursor(0,1);
  lcd.print(F("MASS CALIBRATION"));

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print(F("Known mass is: "));
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  Serial.print(F("New calibration value has been set to: "));
  Serial.print(newCalibrationValue);
  Serial.println(F(", use this as calibration value (calFactor) in your project sketch."));
  Serial.print(F("Save this value to EEPROM adress "));
  Serial.print(calVal_eepromAdress);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("NEW CALIB VALUE"));
  lcd.setCursor(4,1);
  lcd.print(newCalibrationValue);
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("  EEPROM SAVE?"));
  Serial.println(F("? y/n"));

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print(F("Value "));
        Serial.print(newCalibrationValue);
        Serial.print(F(" saved to EEPROM address: "));
        Serial.println(calVal_eepromAdress);
        lcd.setCursor(3,1);
        lcd.print(F("DATA SAVED"));
        _resume = true;

      }
      else if (inByte == 'n') {
        Serial.println(F("Value not saved to EEPROM"));
        lcd.setCursor(2,1);
        lcd.print(F("SAVE ABORTED"));
        delay(3000);
        _resume = true;
      }
    }
  }

  Serial.println(F("End calibration"));
  lcd.setCursor(6,1);
  lcd.print(F("END"));
  lcd.setCursor(2,1);
  lcd.print(F("CALIBRATION"));
  delay(3000);
//  Serial.println("***");
//  Serial.println("To re-calibrate, send 'r' from serial monitor.");
//  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
//  Serial.println("***");
}

















void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}

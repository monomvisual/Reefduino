/*

 25.21 grad
 calib for 600 shorted
 ph 6.86  =  621    2.: 618 24,56c  3.: 615 24,06 c
 ph 9.18 =  534     2.: 530 24,56c  3.: 527 24,06c

m = (ph9,18 - ph6,86) / (530 - 618) 
m = -0,026363

pH = 9,18 - (530 - Po) * m

*/

#include <EEPROM.h>
// temp
#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2 //pin 2
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
DallasTemperature setResolution(9);

//lcd
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);

//scheduler
unsigned long ATOinterval=1;
unsigned long prevATOTime=0;      
int intervalTemp=5000; 
unsigned long prevTempTime=0;
int intervalPH=1000; 
unsigned long prevPHTime=0;
int intervalPage=5000; 
unsigned long prevPageTime=0;

int currentPage = 0;

// Pins
const int analogInPin = A0;  

const int floatSW = 27;
const int atoPumpe = 40;

const int downPin = 3;
const int upPin = 4;
const int enterPin = 5;
const int escPin = 6;

//Temp
float Temp;
float loTemp;
float hiTemp;

//pH
byte i =0;
int readingsPo[4];
int calLopH; //pH 6.86
int calHipH; //pH 9.18 
float m = -0.0263;              //bullshit
int Po;
float pH ;       
float lopH;
float hipH;
int PHcalLO=618;
int PHcalHI=530;

//ATO
float evaporationRate = 0;
int ATOcalib=0;
unsigned long topoffTimer;
float litersRefilled;
float Lps=0.01; //Liters per second ATO
float maxRefill=1.0;
bool ATOerror=0;
float ATOchange;
float lastTopoff;
//buttons
byte up=0;
byte down=0;
byte enter=0;
byte esc=0;
byte debounce= 50;
bool lastupState=1;
bool lastdownState=1;
bool lastenterState=1;
bool lastescState=1;

//***************************************************SETUP

void setup() {
  lcd.init();                      
   lcd.backlight();
  // lcd.setCursor(0, 0);  // col,row
   lcd.print("...YO BITCH...");
  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
  pinMode(enterPin, INPUT_PULLUP);
  pinMode(escPin, INPUT_PULLUP);
  
  Serial.begin(9600);
  //INIT TEMP
  sensors.begin(); 
  sensors.requestTemperatures();
  Temp = sensors.getTempCByIndex(0);
  hiTemp = Temp;
  loTemp = Temp;
  
  // INIT PH
  Po = analogRead(analogInPin);
  for (byte k = 0; k < 4; k++) {
      readingsPo[k]=Po;
      }
  pH = 9.18 - (PHcalHI - Po) * m;
  lopH = pH;
  hipH = pH;

  //INIT ATO
//  EEPROM.get( 0, ATOinterval );
}

//***************************************************LOOP

void loop() {
  //scheduler
   unsigned long currentMillis = millis();
 
   if ((currentMillis - prevPHTime) >= intervalPH) {
      calcPH();
      prevPHTime = currentMillis;
    //  Serial.println("PH measured");
      }
      
   if ((currentMillis - prevTempTime) >= intervalTemp) {
      calcTemp();
      prevTempTime = currentMillis; 
     // Serial.println("Temp measured");
      }
     
   if ((currentMillis - prevPageTime) >= intervalPage) {
      printPage(currentPage);
      prevPageTime = currentMillis; 
     // Serial.println("Page Refresh");
     }
      
   if ((currentMillis - prevATOTime) >= ATOinterval * 20000) {  //  ATOinterval*3600000
      Serial.println("ATO Start");
      runATO(); 
      prevATOTime =currentMillis;
      Serial.println("ATO done");
      Serial.print(litersRefilled);
      Serial.println(" L Refilled");   
      }      
  // scheduler

  checkbutton();
}

//***************************************************FUNCTIONS

//*************************************************** TEMP
void calcTemp(){
   sensors.requestTemperatures();
   Temp = sensors.getTempCByIndex(0);
   
   if(Temp<loTemp){
    loTemp=Temp;
   }
    if(Temp>hiTemp){
    hiTemp=Temp;
   }
}
//***************************************************  PH
void calcPH(){
  if (i < 4) {
     readingsPo[i] = analogRead(analogInPin);
     i++;
    }else{
         i=0;
         }
  
  Po = (readingsPo[0]+readingsPo[1]+readingsPo[2]+readingsPo[3])/4;
  
  pH = 9.18 - (PHcalHI - Po) * m;                         //+ Temp compensation factor
 
  if(pH<lopH){
    lopH=pH;
   }
  if(pH>hipH){
    hipH=pH;
   }
}
//*************************************************** Calibrate PH
void calibratePH(){
  }

//*************************************************** ATO
void runATO(){
  
  printPage(3);
  delay(500);
  lastTopoff=litersRefilled;
  unsigned long topoffStartTime=millis();
  while (digitalRead(floatSW) == LOW && digitalRead(escPin)==HIGH){   // float nicht ausgelöst 
         digitalWrite(atoPumpe, LOW);     // relais pumpe an
         unsigned long topoffTimer=millis();
         litersRefilled=(topoffTimer-topoffStartTime)/1000*Lps;
         lcd.setCursor(5, 1);  
         lcd.print(litersRefilled);
         if(litersRefilled>maxRefill){
            ATOerror=1;
            Serial.println("ATO ERROR");
            break;
           }
         ATOerror=0;  
         }
  digitalWrite(atoPumpe, HIGH);  // relais pumpe aus
  ATOchange=litersRefilled-lastTopoff;
//  topoffEndTime=millis();
//  topoffDuration=topoffEndTime-prevATOTime;
  }

//*************************************************** BUTTONS
void checkbutton(){

 bool upState = digitalRead(upPin);
 bool downState = digitalRead(downPin);
 bool enterState = digitalRead(enterPin);
 bool escState = digitalRead(escPin);
 
//up
if (upState != lastupState){
  if(upState == LOW){
    up=1;
    Serial.println("up");
    delay(debounce);
    }
    lastupState = upState;
  }
if(currentPage==2 && up==1 && ATOcalib==1){
  ATOinterval++;
  lcd.setCursor(12, 1);
  lcd.print(ATOinterval);
  Serial.println(ATOinterval);
  up=0;
  }
up=0;
      
//down
if (downState != lastdownState){
  if(downState == LOW){
    down=1;
    Serial.println("down");
    delay(debounce);
    }
    lastdownState = downState;
  }
if(currentPage==0 && down==1){
  currentPage++;
  printPage(currentPage);
    Serial.print("Page= ");
    Serial.println(currentPage);
  down=0;
  }    
if(currentPage==2 && down==1 && ATOcalib==1){
  ATOinterval--;
  lcd.setCursor(12, 1);
  lcd.print(ATOinterval);
  Serial.println(ATOinterval);
  down=0;
  }    
down=0;
  
//enter
if (enterState != lastenterState){
  if(enterState == LOW){
    enter=1;
    Serial.println("enter");
    delay(debounce);
    }
    lastenterState = enterState;
  }

if(currentPage==0 && enter==1){
  currentPage=2;
  printPage(currentPage);
    Serial.print("Page= ");
    Serial.println(currentPage);
  enter=0;
  }
if(currentPage==1 && enter==1){
  calibratePH();
  Serial.print("calibratePH ");
  enter=0;
  }
if(currentPage==2 && enter==1){
  ATOcalib=!ATOcalib;
  Serial.print("calibratePH ");
  enter=0;
  }
enter=0;

//esc
if (escState != lastescState){
   if(escState == LOW){
     currentPage=0;
     ATOcalib=0;
     lcd.noBlink();
     printPage(currentPage);
     Serial.print("esc");
     Serial.print("Page= ");
     Serial.println(currentPage);
     delay(debounce);
     }
   lastescState = escState;
  }   



if(currentPage==2 && ATOcalib==1){
  lcd.setCursor(12,1);
  lcd.blink();
  }
if(currentPage==2 && ATOcalib==0){
   lcd.noBlink();
  }

}  //voidend

//*************************************************** DISPLAY

void printPage(int Pnum){
  switch (Pnum){
    case 0:               //                        PH TEMP
    
      //lcd.setCursor(0, 0);  // col,row
      lcd.clear();
      lcd.print(" pH   low   hi      ");
              // 0     6     12      20
      lcd.setCursor(0, 1);
      lcd.print(pH);        
      lcd.setCursor(6, 1);
      lcd.print(lopH);
      lcd.setCursor(12, 1);
      lcd.print(hipH);        
      lcd.setCursor(0, 2);  
      lcd.print("Temp  low   hi ");
      lcd.setCursor(0, 3);
      lcd.print(Temp);
      lcd.setCursor(6, 3);
      lcd.print(loTemp);
      lcd.setCursor(12, 3);
      lcd.print(hiTemp);        
             
    break;            //              pH Calibration Mode

    case 1:
      //lcd.setCursor(0, 0);  // col,row
      lcd.clear();
      lcd.print(" pH Calibration Mode");
              // 0     6     12      20
      lcd.setCursor(0, 1);  
      lcd.print("      Old   New");
      lcd.setCursor(0, 2); 
      lcd.print("9.18");
      lcd.setCursor(6, 2); 
      lcd.print(PHcalHI);
      lcd.setCursor(0, 3); 
      lcd.print("6.86");
      lcd.setCursor(6, 3); 
      lcd.print(PHcalLO);
                    
    break;   

    case 2:                   //           TOPOFF
      lcd.clear();
      lcd.print("Auto Topoff");
              // 0     6     12      20
      if(ATOerror==1) {
        lcd.setCursor(14, 0);  
        lcd.print("ERROR");
           }else{
            lcd.setCursor(14, 0);  
            lcd.print("     ");
           }       
      lcd.setCursor(0, 1);  
      lcd.print("Interval");
      lcd.setCursor(12, 1);
      lcd.print(ATOinterval);
      lcd.setCursor(14, 1);
      lcd.print("h");
      lcd.setCursor(0, 2);  
      lcd.print("Evaporation Rate");
      lcd.setCursor(0, 3);  
      lcd.print(litersRefilled);       
      lcd.setCursor(6,3);  
      lcd.print("l/day"); 
      lcd.setCursor(14,3);  
      lcd.print(ATOchange);
      
    break;   

     case 3:                //         TOPOFF in PROGRESS
      lcd.clear();
              // 0     6     12      20
      lcd.setCursor(0, 0);  
      lcd.print(" TOPOFF IN PROGRESS");
      lcd.setCursor(10, 1); 
      lcd.print("ltrs"); 
    //  lcd.setCursor(16, 2);  
    //  lcd.print(litersRefilled);
      lcd.setCursor(0, 3);  
      lcd.print("ESC to abort");      
      
      
    break;   
  }    
  }

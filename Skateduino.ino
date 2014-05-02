/*

 ******************************************
 *                                        *
 *                                        *
 *-------------> SKATEDUINO <-------------*
 *                                        *
 *               V 0.5 BETA               *
 ******************************************
 THIS IS A BETA! I'm never resposibly for any damage to your stuff! This version is NOT tested.
 SkateDuino is a Arduino based controller for an electric skateboard.
 Functions:
 
 >  GPS speed.
 >  Maximum speed: 18km/h (Belgian laws)!
 >  LCD with the current speed, battery state & traffic indicator functions.
 >  Battery monitor for both batteries:
     * BATT 1 = Motor battery (22,4V @ 6 cells)
     * BATT 2 = Stuff battery (7,4 V @ 2 cells).
    
 >  Wii Nunchuck controller.
 >  3 TIP122 for controlling LED's on the electric skateboard: 
     *  Traffic indicator LEFT.
     *  Traffic indicator RIGHT.
     *  Tail lights + headlights.
 
 */

#include <LiquidCrystal.h>
#include <Servo.h>
#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <TinyGPS++.h>


Servo ESC;
ArduinoNunchuk nunchuk = ArduinoNunchuk();
TinyGPSPlus GPS;
LiquidCrystal LCD(A2, A1, 15, 14, 16, 10);

boolean LCDArrowLeft = true;
boolean LCDArrowRight = true;
boolean TrafficIndicatorLeftStatus = false;
boolean TrafficIndicatorRightStatus = false;

int ESCValue = 0;
int SpeedKMPH = 0;
int BatteryLevel = 0;
int BatteryLevelStuff = 0;
int BatteryLevelMotor = 0;
int BatteryNumber = 0;

const int TrafficIndicatorLeftOutput  = 5;
const int TrafficIndicatorRightOutput  = 6;
const int BatteryStuff = A0;
const int BatteryMotor = A3;
const int Lights  =  9;
const int Claxon  =  4;

long TrafficIndicatorspreviousMillis = 0;
long TrafficIndicatorsInterval = 300;
long BatteryLevelpreviousMillis = 0;
long BatteryLevelInterval = 1500;

byte ArrowLeftChar[8] = {
  B00000,
  B00100,
  B01100,
  B11111,
  B11111,
  B01100,
  B00100,
  B00000
};

byte ArrowRightChar[8] = {
  B00000,
  B00100,
  B00110,
  B11111,
  B11111,
  B00110,
  B00100,
  B00000
};

void setup() // BOOT PROCESS
{
  // Setup LCD screen.
  LCD.begin(16, 2);
  LCD.setCursor(0, 0);
  LCD.print("SkateDuino  V0.5");
  LCD.setCursor(0, 1);
  LCD.print(" Booting . . .  ");
  delay(500);

  // Setup GPS serial communication.
  Serial1.begin(9600); // Serial TTL on Arduino Pro Micro, Leonardo, Due.
  LCD.setCursor(0, 1);
  LCD.print("     GPS OK     ");
  delay(500);

  // Setup the Wii Nunchuck Controller.
  nunchuk.init();
  LCD.setCursor(0, 1);
  LCD.print("  NUNCHUCK OK   ");
  delay(500);

  // Intialise Li-Po monitor
  pinMode(BatteryMotor, INPUT);
  digitalWrite(BatteryMotor, HIGH);
  pinMode(BatteryStuff, INPUT);
  digitalWrite(BatteryStuff, HIGH);
  LCD.setCursor(0, 1);
  LCD.print("Li-Po monitor OK");
  delay(500);

  // Arm the ESC.
  ESC.attach(7);   // Attaches the ESC on pin 9 to the servo object.
  LCD.setCursor(0, 1);
  LCD.print("ESC ARMING . . .");
  ESC.write(180);  // Set "TX" to full speed.
  delay(3000);
  ESC.write(27);   // Set "TX" to low. The ESC will arm NOW!
  delay(3000);
  LCD.setCursor(0, 1);
  LCD.print("   ESC ARMED    ");
  delay(500);

  // END Setup process
  pinMode(TrafficIndicatorLeftOutput, OUTPUT);
  pinMode(TrafficIndicatorRightOutput, OUTPUT);
  pinMode(Lights, OUTPUT);
  digitalWrite(Lights, HIGH);
  LCD.createChar(1, ArrowRightChar);
  LCD.createChar(2, ArrowLeftChar);

  LCD.setCursor(0, 1);
  LCD.print("  Booting DONE  ");
  delay(1000);
  LCD.clear();
  LCD.setCursor(0, 1);
  LCD.print("Speed:");
  LCD.setCursor(12, 1);
  LCD.print("km/h");
  LCD.setCursor(2, 0);
  LCD.print("BATT  :");
  LCD.setCursor(13, 0);
  LCD.print("%");
}


void loop()
{
  // Update motor& nunchuck stuff
  nunchuk.update();
  ESCValue = map(nunchuk.analogY, 129, 255, 30, 160);

  if (nunchuk.zButton == 1 && ESCValue > 29)
  {
    ESC.write(ESCValue);
  }
  else
  {
    ESC.write(30);
  }

  // Update GPS stuff
  while (Serial1.available() > 0)
    if (GPS.encode(Serial1.read()))

  // Update battery level
  BatteryLevelUpdate();

  // Update LCD
  LCDUpdate();
  
  // Update traffic indicators
  TrafficIndicatorsUpdate();

}

void TrafficIndicatorsUpdate() // Switch the traffic indicators ON/OFF with the nunchuck accelerometer
{
  unsigned long TrafficIndicatorscurrentMillis = millis();
  if (nunchuk.accelX > 500)
  {
    if (TrafficIndicatorscurrentMillis - TrafficIndicatorspreviousMillis > TrafficIndicatorsInterval) {
      TrafficIndicatorspreviousMillis = TrafficIndicatorscurrentMillis;
      TrafficIndicatorLeftStatus = !TrafficIndicatorLeftStatus;
      LCDArrowLeft = !LCDArrowLeft;
      LCDArrowRight = false;
      digitalWrite(TrafficIndicatorLeftOutput, TrafficIndicatorLeftStatus);
    }
  }
  else
  {
    if (nunchuk.accelX < 400)
    {
      if (TrafficIndicatorscurrentMillis - TrafficIndicatorspreviousMillis > TrafficIndicatorsInterval) {
        TrafficIndicatorspreviousMillis = TrafficIndicatorscurrentMillis;
        TrafficIndicatorRightStatus = !TrafficIndicatorRightStatus;
        LCDArrowRight = !LCDArrowRight;
        LCDArrowLeft = false;
        digitalWrite(TrafficIndicatorRightOutput, TrafficIndicatorRightStatus);
      }
    }
    else
    {
      digitalWrite(13, LOW);
      digitalWrite(TrafficIndicatorLeftOutput, LOW);
      digitalWrite(TrafficIndicatorRightOutput, LOW);
      LCDArrowRight = false;
      LCDArrowLeft = false;
      LCD.setCursor(0, 0);
      LCD.print(" ");
      LCD.setCursor(15, 0);
      LCD.print(" ");
    }
  }
}

void BatteryLevelUpdate() // Batterylevel check every 1,5 second.
{
  unsigned long BatteryLevelcurrentMillis = millis();

  if (BatteryLevelcurrentMillis - BatteryLevelpreviousMillis > BatteryLevelInterval)
  {
    BatteryLevelpreviousMillis = BatteryLevelcurrentMillis;
    if(BatteryNumber == 1)
    {
     LCD.setCursor(7, 0);
     LCD.print("2");
     BatteryLevelStuff = analogRead(BatteryStuff);  // Read battery level and map the value to percents.
     BatteryLevelStuff = map(BatteryLevelStuff, 720, 1008, 0, 100);
     if (BatteryLevelMotor == 100)
    {
      LCD.setCursor(10, 0);
      LCD.print(BatteryLevelMotor);
    }
    else
    {
      LCD.setCursor(10, 0);
      LCD.print("   ");
      LCD.setCursor(11, 0);
      LCD.print(BatteryLevelMotor);
    }
     BatteryNumber = 2;
    }
    else
    {
     LCD.setCursor(7, 0);
     LCD.print("1");
     BatteryLevelMotor = analogRead(BatteryMotor);  // Read battery level and map the value to percents.
     BatteryLevelMotor = map(BatteryLevelMotor, 720, 1008, 0, 100);
     
     if (BatteryLevelStuff == 100)
    {
      LCD.setCursor(10, 0);
      LCD.print(BatteryLevelStuff);
    }
    else
    {
      LCD.setCursor(10, 0);
      LCD.print("   ");
      LCD.setCursor(11, 0);
      LCD.print(BatteryLevelStuff);
    }
     BatteryNumber = 1; 
    }
    
   if(BatteryLevelMotor < 10 || BatteryLevelStuff < 10)
   {
     LCD.setCursor(10, 0);
     LCD.print("LOW ");
   }
   else
   {
     LCD.setCursor(13, 0);
     LCD.print("%");
   }
  }
}

void LCDUpdate() // Update LCD information
{
  if (GPS.speed.isValid() && GPS.speed.age() < 1500) // Show the current speed based on GPS information
  {
    SpeedKMPH = GPS.speed.kmph();
    LCD.setCursor(6, 1);
    LCD.print("     ");
    LCD.setCursor(9, 1);
    LCD.print(SpeedKMPH);
  }
  else
  {
    LCD.setCursor(7, 1);
    LCD.print("N/A  ");
  }

  if (LCDArrowLeft == true)  // LCD arrow left blink
  {
    LCD.setCursor(15, 0);
    LCD.write(1);
    LCD.setCursor(0, 0);
    LCD.print(" ");
  }
  else
  {
    LCD.setCursor(15, 0);
    LCD.print(" ");
  }

  if (LCDArrowRight == true) // LCD arrow right blink
  {
    LCD.setCursor(0, 0);
    LCD.write(2);
    LCD.setCursor(15, 0);
    LCD.print(" ");
  }
  else
  {
    LCD.setCursor(0, 0);
    LCD.print(" ");
  }
}

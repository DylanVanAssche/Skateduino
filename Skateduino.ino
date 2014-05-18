/*

 ******************************************
 *                                        *
 *                                        *
 *-------------> SKATEDUINO <-------------*
 *                                        *
 *             V 1.0 RELEASE              *
 ******************************************
 THIS IS A RELEASE! I'm never resposibly for any damage to your stuff! This version is tested.
 SkateDuino is a Arduino based controller for an electric skateboard.
 
      ///////////////
     // Features //
    ///////////////
 
   >  GPS
   >  Maximum speed: 18km/h (Belgian laws)! 
   >  Acceleration assist.
   >  PSM (Power Save Mode) for saving the battery.
   >  LCD screen with some usefull information:
 
     * GPS speed
     * Battery state
     * Traffic indicator
     * GPS time
     * GPS satellites
     * GPS altitude
     * Motor power in %
 
   >  Battery monitor for both batteries:
 
     * BATT 1 = Motor battery (22,4V @ 6 cells)
     * BATT 2 = Stuff battery (7,4 V @ 2 cells).
 
   >  Wii Nunchuck controller.
   >  3 TIP122 for controlling LED's on the electric skateboard: 
 
     *  Traffic indicator LEFT.
     *  Traffic indicator RIGHT.
     *  Tail lights + headlights. 
 
   >  Simple anti-theft lock
 
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
boolean TrafficIndicatorLeftStatus = true;
boolean TrafficIndicatorRightStatus = true;
boolean Lock = true;

int ESCValue = 0;
int SpeedKMPH = 0;
int BatteryLevel = 0;
int BatteryLevelStuff = 0;
int BatteryLevelMotor = 0;
int BatteryNumber = 0;
int Navigation = 3;
int PSMTimer = 0;
int Height = 0;
int MotorValue = 30;
int MotorValueDisplay = 0;

const int TrafficIndicatorLeftOutput  = 5;
const int TrafficIndicatorRightOutput  = 6;
const int BatteryStuff = A0;
const int BatteryMotor = A3;
const int Lights  =  9;
const int Claxon  =  4;
const int MaximumSpeed = 160; // Declare here the ESCValue when you reach 18 km/h.

long TrafficIndicatorspreviousMillis = 0; // Timing stuff
long TrafficIndicatorsInterval = 300;
long BatteryLevelpreviousMillis = 0;
long BatteryLevelInterval = 1500;
long NavigationPositivepreviousMillis = 0;
long NavigationPositiveInterval = 500;
long NavigationNegativepreviousMillis = 0;
long NavigationNegativeInterval = 250;
long MotorAccelerationNegativepreviousMillis = 0;
long MotorAccelerationNegativeInterval = 15;
long MotorAccelerationPositivepreviousMillis = 0;
long MotorAccelerationPositiveInterval = 50;
long PSMpreviousMillis = 0;
long PSMInterval = 1000;

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

byte LockChar1[8] = {
  B00000,
  B00000,
  B00111,
  B00100,
  B00100,
  B00100,
  B00100,
  B11111
};

byte LockChar2[8] = {
  B00000,
  B00000,
  B11100,
  B00100,
  B00100,
  B00100,
  B00100,
  B11111
};

byte LockChar3[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000
};

byte LockChar4[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000
};

void setup() // BOOT PROCESS
{
  // Setup LCD screen.
  LCD.begin(16, 2);
  LCD.setCursor(0, 0);
  LCD.print("SkateDuino  V1.0");
  LCD.setCursor(0, 1);
  LCD.print(" Booting . . .  ");
  delay(350);

  // Setup GPS serial communication.
  Serial1.begin(9600); // Serial TTL on Arduino Pro Micro, Leonardo, Due.
  LCD.setCursor(0, 1);
  LCD.print("     GPS OK     ");
  delay(250);

  // Setup the Wii Nunchuck Controller.
  nunchuk.init();
  LCD.setCursor(0, 1);
  LCD.print("  NUNCHUCK OK   ");
  delay(250);

  // Intialise Li-Po monitor
  pinMode(BatteryMotor, INPUT);
  digitalWrite(BatteryMotor, HIGH);
  pinMode(BatteryStuff, INPUT);
  digitalWrite(BatteryStuff, HIGH);
  LCD.setCursor(0, 1);
  LCD.print("Li-Po monitor OK");
  delay(250);

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
  delay(250);

  // END Setup process
  pinMode(TrafficIndicatorLeftOutput, OUTPUT);
  pinMode(TrafficIndicatorRightOutput, OUTPUT);
  digitalWrite(TrafficIndicatorLeftOutput, LOW);
  digitalWrite(TrafficIndicatorRightOutput, LOW);
  pinMode(Lights, OUTPUT);
  digitalWrite(Lights, HIGH);
  pinMode(Claxon, OUTPUT);
  digitalWrite(Claxon, LOW);

  LCD.createChar(1, ArrowRightChar);
  LCD.createChar(2, ArrowLeftChar);
  LCD.createChar(3, LockChar1);
  LCD.createChar(4, LockChar2);
  LCD.createChar(5, LockChar3);
  LCD.createChar(6, LockChar4);
  LCD.setCursor(0, 1);
  LCD.print("  Booting DONE  ");
  delay(350);
  LCD.clear();
}

void loop()
{
  if(Lock == true) // Simple lock to prevent that someone can use the skateboard.
  {
    LockFunction();
  }
  else
  {
    // Update motor & nunchuck stuff.
    nunchuk.update();
    ESCValue = map(nunchuk.analogY, 135, 255, 30, 160);

    if (nunchuk.zButton == 1 && ESCValue > 29)
    {
      if(MotorValue < ESCValue) 
      {
        unsigned long MotorAccelerationPositivecurrentMillis = millis();
        if(MotorAccelerationPositivecurrentMillis - MotorAccelerationPositivepreviousMillis > MotorAccelerationPositiveInterval)
        {
          MotorAccelerationPositivepreviousMillis = MotorAccelerationPositivecurrentMillis;
          if(MotorValue < 160)
          {
            MotorValue++;
          }
          else
          {
            MotorValue = 160; 
          }
          ESC.write(MotorValue);
        }
      }
    }
    else
    {
      unsigned long MotorAccelerationNegativecurrentMillis = millis();
      if(MotorAccelerationNegativecurrentMillis - MotorAccelerationNegativepreviousMillis > MotorAccelerationNegativeInterval)
      {
        MotorAccelerationNegativepreviousMillis = MotorAccelerationNegativecurrentMillis;
        if(MotorValue > 30)
        {
          MotorValue--;
        }
        else
        {
          MotorValue = 30;
        }
        ESC.write(MotorValue);
      }
    }

    if(nunchuk.cButton == 1)
    {
      digitalWrite(Claxon,HIGH); 
    }
    else
    {
      digitalWrite(Claxon,LOW); 
    }

    if(nunchuk.cButton == 1 && nunchuk.analogY >= 200 )
    {
      Lock = true;
    }

    if(nunchuk.analogX <= 100 && nunchuk.zButton == 0 && Navigation > 1)
    {
      unsigned long NavigationNegativecurrentMillis = millis();
      if (NavigationNegativecurrentMillis - NavigationNegativepreviousMillis > NavigationNegativeInterval) {
        NavigationNegativepreviousMillis = NavigationNegativecurrentMillis;
        Navigation--;
      }
    }
    if(nunchuk.analogX >= 150 && nunchuk.zButton == 0 && Navigation < 5)
    {
      unsigned long NavigationPositivecurrentMillis = millis();
      if (NavigationPositivecurrentMillis - NavigationPositivepreviousMillis > NavigationPositiveInterval) {
        NavigationPositivepreviousMillis = NavigationPositivecurrentMillis;
        Navigation++;
      }
    }

    // Update GPS stuff
    while (Serial1.available() > 0)
      if (GPS.encode(Serial1.read()))

        // Update battery level.
        BatteryLevelUpdate();

    // Update LCD.
    LCDUpdate();

    // Update traffic indicators.
    TrafficIndicatorsUpdate();

    // Activate PSM when we aren't driving.
    PowerSaveMode();  


  }
}

void TrafficIndicatorsUpdate() // Switch the traffic indicators ON/OFF with the nunchuck accelerometer.
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
    LCD.setCursor(2, 0);
    LCD.print("BATT  :");
    LCD.setCursor(13, 0);
    LCD.print("%");

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
      LCD.print("LOW !");
    }
    else
    {
      LCD.setCursor(13, 0);
      LCD.print("% ");
    }
  }
}

void LCDUpdate() // Update LCD information
{
  switch(Navigation)
  {
  case 3:
    if (GPS.speed.isValid() && GPS.speed.age() < 1500) // Show the current speed based on GPS information.
    {
      SpeedKMPH = GPS.speed.kmph();
      LCD.setCursor(0, 1);
      LCD.print("Speed: ");
      LCD.setCursor(11, 1);
      LCD.print(" km/h");
      LCD.setCursor(6, 1);
      LCD.print("     ");
      LCD.setCursor(9, 1);
      LCD.print(SpeedKMPH);
    }
    else
    {
      LCD.setCursor(0, 1);
      LCD.print("Speed: ");
      LCD.setCursor(11, 1);
      LCD.print(" km/h");
      LCD.setCursor(7, 1);
      LCD.print("N/A ");
    }
    break;

  case 2:
    if (GPS.satellites.isValid() && GPS.satellites.age() < 1500) // Show the number of satellites.
    {
      LCD.setCursor(0, 1);
      LCD.print("#Satellites:    ");
      LCD.setCursor(13, 1);
      LCD.print(GPS.satellites.value());
    }
    else
    {
      LCD.setCursor(0, 1);
      LCD.print("#Satellites:    ");
      LCD.setCursor(13, 1);
      LCD.print("N/A ");
    }
    break;

  case 1:
    if (GPS.speed.isValid() && GPS.speed.age() < 1500) // Show the current time based on GPS information.
    {
      LCD.setCursor(0, 1);
      LCD.print("                ");
      LCD.setCursor(0, 1);
      LCD.print("Time:           ");
      LCD.setCursor(7, 1);
      LCD.print("h");
      LCD.setCursor(11, 1);
      LCD.print("m");
      LCD.setCursor(15, 1);
      LCD.print("s");
      LCD.setCursor(5, 1);
      LCD.print(GPS.time.hour() + 2);
      LCD.setCursor(9, 1);
      LCD.print(GPS.time.minute());
      LCD.setCursor(13, 1);
      LCD.print(GPS.time.second());
    }
    else
    {
      LCD.setCursor(0, 1);
      LCD.print("Time:           ");
      LCD.setCursor(9, 1);
      LCD.print("N/A");
    }
    break;

  case 4:
    if (GPS.altitude.isValid() && GPS.altitude.age() < 1500) // Show the current time based on GPS information.
    {
      Height = GPS.altitude.meters();
      LCD.setCursor(0, 1);
      LCD.print("                ");
      LCD.setCursor(0, 1);
      LCD.print("Height:         ");  
      LCD.setCursor(9, 1);
      LCD.print("   ");
      LCD.setCursor(12, 1);
      LCD.print("m");
      LCD.print(Height);
    }
    else
    {
      LCD.setCursor(0, 1);
      LCD.print("Height:         "); 
      LCD.setCursor(9, 1);
      LCD.print("N/A");
      LCD.setCursor(14, 1);
      LCD.print("m");
    }
    break;

  case 5:
    LCD.setCursor(0, 1);
    LCD.print("Motor power:    "); 
    LCD.setCursor(15, 1);
    LCD.print("%");
    MotorValueDisplay = map(MotorValue, 30, 160, 0, 100);
    if(MotorValueDisplay == 100)
    {
      LCD.setCursor(12, 1);
      LCD.print(MotorValueDisplay);
    }
    else
    {
      LCD.setCursor(13, 1);
      LCD.print(MotorValueDisplay);
    }
    break;
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

void PowerSaveMode()
{
  if(nunchuk.zButton == 0 && MotorValue <= 31)
  {
    unsigned long PSMcurrentMillis = millis();
    if (PSMcurrentMillis - PSMpreviousMillis > PSMInterval) // Blink the lights every 4 seconds for 1 second (= 12 blinks/minute).
    {
      PSMpreviousMillis = PSMcurrentMillis;
      PSMTimer++;
      if(PSMTimer == 4)
      {
        digitalWrite(Lights, HIGH); 
      }
      else
      {
        digitalWrite(Lights, LOW);  
      }

      if(PSMTimer > 2) // Reset the timer...
      {
        PSMTimer = 0; 
      }
    }
  }  
  else
  {
    digitalWrite(Lights, HIGH); 
  }
}

void LockFunction()
{
  LCD.clear();
  LCD.setCursor(1, 0);
  LCD.write(3);
  LCD.setCursor(2, 0);
  LCD.write(4);
  LCD.setCursor(1, 1);
  LCD.write(5);
  LCD.setCursor(2, 1);
  LCD.write(6);

  LCD.setCursor(13, 0);
  LCD.write(3);
  LCD.setCursor(14, 0);
  LCD.write(4);
  LCD.setCursor(13, 1);
  LCD.write(5);
  LCD.setCursor(14, 1);
  LCD.write(6);

  LCD.setCursor(5, 0);
  LCD.print("Please");
  LCD.setCursor(5, 1);
  LCD.print("unlock");   

  nunchuk.update();
  if(nunchuk.cButton == 1 && nunchuk.analogY <= 15 )
  {
    Lock = false; 
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("    Identity    ");
    LCD.setCursor(0, 1);
    LCD.print("    CONFIRMED   ");
    delay(750);
    LCD.clear();
  } 
}

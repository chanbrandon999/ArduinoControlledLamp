#include <IRremote.h>

const int RECV_PIN = 3;
IRrecv irrecv(RECV_PIN);
decode_results results;

enum actions {potentiometerTurn, switchToggle, sleepSetting, remoteCommand, newSetPoint};
enum actions actionPerformed;

//Digital Pins
const int PIN_BRIGHTNESS = 5;
const int PIN_LAMPTOGGLE = 8;

//Analog Pins
const int PIN_POT = 0;
const int PIN_STICK_Y = 0;
const int PIN_STICK_X = 0;

//Lamp control variables
int lampBrightness = 255;
bool lampStatus = false;
bool lampStatusPrev = false;
int prevPotRead = 0;

//Analog Stick Variables
int prevStickReadX = 0;
int prevStickReadY = 0;
int prevSwitchRead = 0;

//Sleep mode variables
bool sleepModeActive = false;
bool isGoingSleep = false;

//Setpoint variables
bool tgtModeActive = false;
bool isSettingTgt = false;

//Remote variables
int dcrc;
unsigned int remoteRead = 0;
unsigned int serialRead = 0;
unsigned int readCode = 0;
unsigned int readCodePrev = 0;

//Method prototypes
int calculateBrightness(int value);
unsigned int getButton(int val);
void setLamp();

class PID
{
  private:
    float kp = 0;
    float ki = 0;
    float kd = 0;

    int error = 0;
    int prevError = 0;
    int totalError = 0;
    int initialMeasure = -1;

  public:
    int target = 0;

  public:
    PID()
    {}
    void PIDreset()
    {
      target = 0;
      error = 0;
      prevError = 0;
      totalError = 0;
      initialMeasure = -1;
      kp = 0;
      ki = 0;
      kd = 0;
    }
    void setTarget(const int newTarget)
    {
      target = newTarget;
    }

    void setK(const float newP, const float newI, const float newD)
    {
      kp = newP;
      ki = newI;
      kd = newD;
    }
    float calcNext(const int measured)
    {
      if (initialMeasure == -1)
      {
        initialMeasure = measured;
      }
      error = target - measured;
      totalError += error;
      ///*
      Serial.print(measured);
      Serial.print(", ");
      Serial.print(error);
      Serial.print(", ");
      Serial.print(totalError);
      Serial.print(", ");      
      Serial.print(ki);
      Serial.print(", ");
      Serial.print((ki * totalError));
      Serial.print(", ");
      Serial.println((kp * error) + (ki * totalError) + (kd * (error - prevError)) + initialMeasure);
      //*/
      return ((kp * error) + (ki * totalError) + (kd * (error - prevError))) + initialMeasure;
    }

};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);

  //analogWrite(5, 255);
  pinMode(PIN_BRIGHTNESS, OUTPUT);
  pinMode(7, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(8, OUTPUT);
  //digitalWrite(8, HIGH);



  prevSwitchRead = digitalRead(7);

}


PID pidSleep;
PID pidSetTgt;



void loop() {


  dcrc = irrecv.decode(&results);

  //Detect potentiometer settings
  if (abs(prevPotRead - analogRead(0)) > 15)
  {
    actionPerformed = potentiometerTurn;
    Serial.print("LB: ");
    Serial.print(lampBrightness);
    Serial.print("\tRead: ");
    Serial.print(analogRead(0));
    Serial.print("\tPrevRead: ");
    Serial.println(prevPotRead);
    //Analog reads from 0 - 1023
    prevPotRead = analogRead(0);
    lampStatus = true;
    lampBrightness = calculateBrightness(prevPotRead / 4);

    if (lampBrightness == 0)
    {
      lampStatus = false;
    }

    //analogWrite between 0-256
    analogWrite(PIN_BRIGHTNESS, lampBrightness);
  }
  else if (analogRead(1) < 100 || 800 < analogRead(1))
  {
    actionPerformed = switchToggle;
    if (analogRead(1) < 100)
    {
      lampStatus = false;

      //Serial.println("Toggle Off");
    }
    else if (800 < analogRead(1))
    {
      lampStatus = true;
      //Serial.println("Toggle On");
    }
    //prevStickReadX = analogRead(1);
    //Serial.println(analogRead(1));
  }
  else if (analogRead(2) < 100 || 800 < analogRead(2))
  {
    if (analogRead(2) < 100)
    {
      //lampStatus = false;
      Serial.println("Toggle Left");
    }
    else if (800 < analogRead(2))
    {
      //lampStatus = true;
      Serial.println("Toggle Right");
    }
    //prevStickReadX = analogRead(2);
    //Serial.println(analogRead(2));
  }
  ///*
  else if (dcrc || Serial.available() > 0) {
    if (dcrc || Serial.available() > 0)
    {
      remoteRead = results.value;
      readCode = getButton(remoteRead);

      while (Serial.available() > 0)
      {
        serialRead = Serial.parseInt();
        if (serialRead != 0)
        {
          readCode = serialRead;
          Serial.print(readCode, HEX);
          Serial.print(", ");
          delay(1); 
        }
      }
      
      Serial.println (" ");

      if (readCode != 0xFFFF)
      {
        readCodePrev = readCode;
        
        Serial.print("READ success! ");
        //Serial.print((getButton(remoteRead) == getButton(readCodePrev)));
        //Serial.print(", ");
        Serial.print(readCode, HEX);
        Serial.print(", ");
        Serial.println(readCode);

        if (0 < readCode && readCode < 10)
        {
          Serial.println("Target mode initial run!");
          isSettingTgt = true;
          
          pidSetTgt.PIDreset();
          pidSetTgt.setK(0, 0.1, 0);
          pidSetTgt.setTarget(readCode * 30 - 15);

          if (!lampStatus)
          {
            lampBrightness = 0;
          }
          
          actionPerformed = newSetPoint;
        }
        else if (readCode == 10)
        {
          Serial.println("Toggle lamp from remote");
          lampStatus = !lampStatus;
          actionPerformed = switchToggle;
        }
        else if (readCode == 0 || readCode == 11)
        {
          Serial.println("Turn on sleep mode from remote");
          //isSettingTarget = false;
          sleepModeActive = true;
          actionPerformed = sleepSetting;
        }        
        else if (readCode == 12)
        {
          Serial.println("PARTY MODE ACTIVATED");

          
          for (int i = 0; i < 50; i++)
          {
            lampStatus = !lampStatus;
            setLamp();
            delay(2);
          }
        }
      }
      irrecv.resume();
    }
  }
  //Target mode
  else if (tgtModeActive || isSettingTgt)
  {
    //Cancels sleep mode if another action is performed while in action
    if (isSettingTgt && actionPerformed != newSetPoint)
    {
      Serial.println("Target mode interrupted");
      tgtModeActive = false;
      isSettingTgt = false;
    }
    else
    {

      lampBrightness = (int)pidSetTgt.calcNext(lampBrightness);
      lampStatus = true;
      
      if (abs(lampBrightness - pidSetTgt.target) <= 1)
      {
        Serial.println("Target mode done");
        tgtModeActive = false;
        isSettingTgt = false;
      }
      delay(100);
    }
  }
  //Put to sleep mode
  else if (sleepModeActive || isGoingSleep)
  {
    //Cancels sleep mode if another action is performed while in action
    if (isGoingSleep && actionPerformed != sleepSetting)
    {
      Serial.println("Sleep mode interrupted");
      sleepModeActive = false;
      isGoingSleep = false;
      pidSleep.PIDreset();
    }
    //Fist run of sleepMode
    else if (!isGoingSleep)
    {
      //Flash lamp for ack
      lampStatus = false;
      setLamp();
      delay(50);
      lampStatus = true;
      setLamp();
      
      pidSleep.PIDreset();
      pidSleep.setK(0, 0.01, 0);
      Serial.println("Sleep mode initial run!");
      actionPerformed = sleepSetting;
      isGoingSleep = true;
      sleepModeActive = false;
      lampStatus = true;
    }
    //1 run of sleep mode
    else
    {
      lampBrightness = (int)pidSleep.calcNext(lampBrightness);
      analogWrite(PIN_BRIGHTNESS, lampBrightness);

      if (lampBrightness == 0)
      {
        Serial.println("Sleep mode done");
        sleepModeActive = false;
        isGoingSleep = false;
        lampStatus = false;
        pidSleep.PIDreset();
        lampBrightness = 100;
      }
      //Serial.println(lampBrightness);
      delay(100);
    }
  }
  //*/


  //################################################
  setLamp();
  //##############################################
  delay(10);

}
/////////##############################################################################
void setLamp()
{
  if (lampStatusPrev != lampStatus)
  {
    if (lampStatus)
    {
      Serial.println("Toggle On");
      delay(20);
      digitalWrite(8, HIGH);
    }
    else if (!lampStatus)
    {
      digitalWrite(8, LOW);
      Serial.println("Toggle Off");
      //analogWrite(PIN_BRIGHTNESS, 0);
    }
    lampStatusPrev = lampStatus;
    delay(50);
  }
  
  analogWrite(PIN_BRIGHTNESS, lampBrightness);
}


//Calculate brightness expects between 0-255
int calculateBrightness(int value)
{
  float rc = 0.0034 * value * value + 0.1369 * value - 8E-13;
  return rc;
}

unsigned int getButton(int val)
{
  unsigned int foo;

  switch (val) {
    case 0x9867:
      foo = 0;
      break;
    case 0xA25D:
    case 0x261B:
      foo = 1;
      break;
    case 0x629D:
    case 0x1DBB:
      foo = 2;
      break;
    case 0xE21D:
    case 0x6D7F:
      foo = 3;
      break;
    case 0x22DD:
    case 0xD14F:
      foo = 4;
      break;
    case 0x2FD:
    case 0xFF:
    case 0x4B1B:
      foo = 5;
      break;
    case 0xC23D:
    case 0x4DBB:
    case 0xA086:
      foo = 6;
      break;
    case 0xE01F:
    case 0x559E:
      foo = 7;
      break;
    case 0xA857:
    case 0x778A:
      foo = 8;
      break;
    case 0x906F:
    case 0xBD7F:
    case 0xA4E4:
    case 0x18ED:
    case 0xBFAD:
    case 0x1643:
      foo = 9;
      break;
    case 0x38C7:  //on/off (OK) button
    case 0x3CBB:  //on/off (OK) button
      foo = 10;
      break;
    case 0x6897:  //sleep mode (*) button
    case 0xE57B:  //sleep mode (*) button
    case 0x616E:  //sleep mode (*) button
      foo = 11;
      break;
    case 0xB04F:  // (#) button
      foo = 12;
      break;
    default:
      foo = -1;
  }
  return foo;
}


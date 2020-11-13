#include <Arduino.h>
#include "MegapiDCMotors.h"
//#include "TimerFive.h"
byte prev_A1;
byte prev_B1;

byte prev_A2;
byte prev_B2;

byte prev_A3;
byte prev_B3;

byte prev_A4;
byte prev_B4;

volatile int16_T EncValue1 = 0;
volatile int16_T EncValue2 = 0;
volatile int16_T EncValue3 = 0;
volatile int16_T EncValue4 = 0;
 
volatile long EncPeriod1 = 0;
volatile long EncPeriod2 = 0;
volatile long EncPeriod3 = 0;
volatile long EncPeriod4 = 0;

unsigned long  EncLastIntTime1 = 0;
unsigned long  EncLastIntTime2 = 0;
unsigned long  EncLastIntTime3 = 0;
unsigned long  EncLastIntTime4 = 0;


void QuadratureDecode(byte valA, byte valB,byte &prev_A,byte &prev_B,volatile int &pos)
{ 
    if ((valA != prev_A) || (valB != prev_B)) //change?
      if (valA ^ prev_B) // negative direction?
        pos--;
      else
        pos++;
    prev_A = valA;
    prev_B = valB;
}

void isr_fullQuadrature() {
  QuadratureDecode(NE1_1_val?1:0,NE2_1_val?1:0,prev_A1,prev_B1,EncValue1);
  QuadratureDecode(NE1_2_val?1:0,NE2_2_val?1:0,prev_A2,prev_B2,EncValue2);
  QuadratureDecode(NE1_3_val?1:0,NE2_3_val?1:0,prev_A3,prev_B3,EncValue3);
  QuadratureDecode(NE1_4_val?1:0,NE2_4_val?1:0,prev_A4,prev_B4,EncValue4);
}


/* Interrupt routine on timer 1 overflow.
 * Timer 1 is also used for motor 1 PWM so the rate is fixed at 31372.55Hz
 * The encoder pooling is done at a rate of 31372.55Hz / 3.
 */
ISR(TIMER1_OVF_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  static byte n = 0;
  if(n==0){
	isr_fullQuadrature();
	n=2;
  }else
	n--;
}

extern "C" void FullQuadratureInit(){
  pinMode(NE2_1, INPUT_PULLUP);
  pinMode(NE1_1, INPUT_PULLUP);
  pinMode(NE2_2, INPUT_PULLUP);
  pinMode(NE1_2, INPUT_PULLUP);
  pinMode(NE2_3, INPUT_PULLUP);
  pinMode(NE1_3, INPUT_PULLUP);
  pinMode(NE2_4, INPUT_PULLUP);
  pinMode(NE1_4, INPUT_PULLUP);
  prev_A1= digitalRead(NE1_1);
  prev_B1= digitalRead(NE2_1);
  prev_A2= digitalRead(NE1_2);
  prev_B2= digitalRead(NE2_2);
  prev_A3= digitalRead(NE1_3);
  prev_B3= digitalRead(NE2_3);
  prev_A4= digitalRead(NE1_4);
  prev_B4= digitalRead(NE2_4);
  TCCR1B = TCCR1B & 0b11111000 | 0x01; //Sets the frequency of the PWM to 31372.55Hz (share with motor 1 PWM)
  TIMSK1 = _BV(TOIE1); // enable Timer 1 overflow interrupts
}

extern "C" void EncPort1Reset()
{
    noInterrupts();
    EncValue1=0;
    interrupts();
}

extern "C" void EncPort2Reset()
{
    noInterrupts();
    EncValue2=0;
    interrupts();
}
extern "C" void EncPort3Reset()
{
    noInterrupts();
    EncValue3=0;
    interrupts();
}

extern "C" void EncPort4Reset()
{
    noInterrupts();
    EncValue4=0;
    interrupts();
}


extern "C" void EncPort1Setup()
{
    EncValue1 = 0;
    pinMode(NE2_1, INPUT_PULLUP);
    pinMode(NE1_1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(NE2_1), Enc1INT, FALLING);
    EncLastIntTime1 = micros();
    EncPeriod1 = MAX_int16_T;
}

void Enc1INT ()
{
    unsigned long timeInt = micros();
    #ifdef MEGAPI_FULL_ENC_MODE
    interrupts();
    #endif
    
    
    if(digitalRead(NE1_1))
    {
        #ifndef MEGAPI_FULL_ENC_MODE
        EncValue1 ++;
        #endif
        EncPeriod1 = timeInt - EncLastIntTime1;
    }
    else
    {
        #ifndef MEGAPI_FULL_ENC_MODE
        EncValue1--;
        #endif
        EncPeriod1 = EncLastIntTime1- timeInt;
    }
    
    EncLastIntTime1 = timeInt;
    
//EncValue1 += digitalRead(NE1_1) ? 1 : -1;
}

extern "C" int32_T EncPeriodRead1()
{ 
    int32_T lastPeriod;
    unsigned long lastIntTime;
    
    noInterrupts();
    lastPeriod = EncPeriod1;
    lastIntTime = EncLastIntTime1;
    interrupts();
    
    int32_T actualPeriod = micros() - EncLastIntTime1;
    if(actualPeriod<0)
        actualPeriod = MAX_int32_T;
    if (actualPeriod>abs(lastPeriod))
        return (lastPeriod>0? actualPeriod : -actualPeriod);
    else
        return lastPeriod;
    //return lastPeriod;
}

extern "C" int16_T EncPortRead1()
{ 
    int16_T temp;
    noInterrupts();
    temp = EncValue1;
    interrupts();
    return temp;
}

extern "C" void EncPort2Setup()
{
    EncValue2 = 0;
    pinMode(NE2_2, INPUT_PULLUP);
    pinMode(NE1_2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(NE2_2), Enc2INT, FALLING);
}

void Enc2INT ()
{
    if(digitalRead(NE1_2))
        EncValue2 ++;
    else
        EncValue2--;
    //EncValue2 += digitalRead(NE1_2) ? 1 : -1;
}

extern "C" int16_T EncPortRead2()
{ 
    int16_T temp;
    noInterrupts();
    temp = EncValue2;
    interrupts();
    return temp;
}

extern "C" void EncPort3Setup()
{
    EncValue3 = 0;
    pinMode(NE2_3, INPUT_PULLUP);
    pinMode(NE1_3, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(NE2_3), Enc3INT, FALLING);
}

void Enc3INT ()
{
    if(digitalRead(NE1_3))
        EncValue3 ++;
    else
        EncValue3--;
    //EncValue2 += digitalRead(NE1_2) ? 1 : -1;
}

extern "C" int16_T EncPortRead3()
{ 
    int16_T temp;
    noInterrupts();
    temp = EncValue3;
    interrupts();
    return temp;
}

extern "C" void EncPort4Setup()
{
    EncValue4 = 0;
    pinMode(NE2_4, INPUT_PULLUP);
    pinMode(NE1_4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(NE2_4), Enc4INT, FALLING);
}

void Enc4INT ()
{
    if(digitalRead(NE1_4))
        EncValue4 ++;
    else
        EncValue4--;
    //EncValue2 += digitalRead(NE1_2) ? 1 : -1;
}

extern "C" int16_T EncPortRead4()
{ 
    int16_T temp;
    noInterrupts();
    temp = EncValue4;
    interrupts();
    return temp;
}

extern "C" void DCMotor1Setup()
{ 
    pinMode(BI1_1, OUTPUT);
    pinMode(BI2_1, OUTPUT);
    pinMode(PWMB_1, OUTPUT);
      TCCR1B = TCCR1B & 0b11111000 | 0x01;//Sets the frequency of the PWM to 31372.55Hz
}

extern "C" void DCMotor1Set(int16_T value)
{ 
    if(value>0)
    {
    digitalWrite(BI1_1,LOW);
    digitalWrite(BI2_1,HIGH);
    }
    else{
    digitalWrite(BI2_1,LOW);
    digitalWrite(BI1_1,HIGH);
    }
    
    
    analogWrite(PWMB_1, constrain(abs(value),0,MAX_uint8_T));
}


extern "C" void DCMotor2Setup()
{ 
    pinMode(BI1_2, OUTPUT);
    pinMode(BI2_2, OUTPUT);
    pinMode(PWMB_2, OUTPUT);
    TCCR4B = TCCR4B & 0b11111000 | 0x01;//Sets the frequency of the PWM to 31372.55Hz
}   

extern "C" void DCMotor2Set(int16_T value)
{ 
    if(value>0)
    {
    digitalWrite(BI1_2,LOW);
    digitalWrite(BI2_2,HIGH);
    }
    else{
    digitalWrite(BI2_2,LOW);
    digitalWrite(BI1_2,HIGH);
    }
    
    
    analogWrite(PWMB_2, constrain(abs(value),0,MAX_uint8_T));
}

extern "C" void DCMotor3Setup()
{ 
    pinMode(BI1_3, OUTPUT);
    pinMode(BI2_3, OUTPUT);
    pinMode(PWMB_3, OUTPUT);
    TCCR2B = TCCR2B & 0b11111000 | 0x01;//Sets the frequency of the PWM to 31372.55Hz
}

extern "C" void DCMotor3Set(int16_T value)
{ 
    if(value>0)
    {
    digitalWrite(BI1_3,LOW);
    digitalWrite(BI2_3,HIGH);
    }
    else{
    digitalWrite(BI2_3,LOW);
    digitalWrite(BI1_3,HIGH);
    }
    
    
    analogWrite(PWMB_3, constrain(abs(value),0,MAX_uint8_T));
}

extern "C" void DCMotor4Setup()
{ 
    pinMode(BI1_4, OUTPUT);
    pinMode(BI2_4, OUTPUT);
    pinMode(PWMB_4, OUTPUT);
    TCCR2B = TCCR3B & 0b11111000 | 0x01;//Sets the frequency of the PWM to 31372.55Hz
}

extern "C" void DCMotor4Set(int16_T value)
{ 
    if(value>0)
    {
    digitalWrite(BI1_4,LOW);
    digitalWrite(BI2_4,HIGH);
    }
    else{
    digitalWrite(BI2_4,LOW);
    digitalWrite(BI1_4,HIGH);
    }
    
    
    analogWrite(PWMB_4, constrain(abs(value),0,MAX_uint8_T));
}

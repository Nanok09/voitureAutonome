#ifndef _MEGADCMOTOR_ARDUINO_H_
#define _MEGADCMOTOR_ARDUINO_H_
#include "rtwtypes.h"

#ifdef __cplusplus
extern "C" {
#endif



// Motor 1
#define NE1_1 31
#define NE2_1 18
#define NE1_1_val PINC & 0b01000000 // direct register acces to pin 31
#define NE2_1_val PIND & 0b00001000 // direct register acces to pin 18
#define PWMB_1 12
#define BI1_1 34
#define BI2_1 35

// Motor 2
#define NE1_2 38
#define NE2_2 19
#define NE1_2_val PIND & 0b10000000 // direct register acces to pin 38
#define NE2_2_val PIND & 0b00000100 // direct register acces to pin 19
#define PWMB_2 8
#define BI1_2 37
#define BI2_2 36
    
// Motor 3    
#define NE1_3 49
#define NE2_3 3
#define NE1_3_val PINL & 0b00000001 // direct register acces to pin 49
#define NE2_3_val PINE & 0b00100000 // direct register acces to pin 3
#define PWMB_3 9
#define BI1_3 43
#define BI2_3 42
    
// Motor 4
#define NE1_4 A1
#define NE2_4 2
#define NE1_4_val PINF & 0b00000010 // direct register acces to pin A1
#define NE2_4_val PINE & 0b00010000 // direct register acces to pin 2
#define PWMB_4 5
#define BI1_4 A4
#define BI2_4 A5
    
void EncPort1Setup();
void EncPort2Setup();
void EncPort3Setup();
void EncPort4Setup();

int16_T EncPortRead1();
int16_T EncPortRead2();
int16_T EncPortRead3();
int16_T EncPortRead4();


void FullQuadratureInit();

int32_T EncPeriodRead1();

void Enc1INT ();
void Enc2INT ();
void Enc3INT ();
void Enc4INT ();

void EncPort1Reset();
void EncPort2Reset();
void EncPort3Reset();
void EncPort4Reset();

void DCMotor1Setup();
void DCMotor1Set(int16_T);

void DCMotor2Setup();
void DCMotor2Set(int16_T);

void DCMotor3Setup();
void DCMotor3Set(int16_T);

void DCMotor4Setup();
void DCMotor4Set(int16_T);

#ifdef __cplusplus
}
#endif
#endif 

#pragma once
#include <stdbool.h>

void rhs_hal_io_init(void);

bool IN0_IS_HIGH(void);
bool IN1_IS_HIGH(void);
bool IN2_IS_HIGH(void);
bool IN3_IS_HIGH(void);
bool IN4_IS_HIGH(void);
bool IN0_IS_LOW(void);
bool IN1_IS_LOW(void);
bool IN2_IS_LOW(void);
bool IN3_IS_LOW(void);
bool IN4_IS_LOW(void);

void OUT0_ON(void);
void OUT0_OFF(void);
void OUT1_ON(void);
void OUT1_OFF(void);
void OUT2_ON(void);
void OUT2_OFF(void);
void OUT3_ON(void);
void OUT3_OFF(void);
void OUT4_ON(void);
void OUT4_OFF(void);

/** KEYs defines **/
void KEY0_ON(void);
void KEY0_OFF(void);
void KEY1_ON(void);
void KEY1_OFF(void);
void KEY2_ON(void);
void KEY2_OFF(void);
void KEY3_ON(void);
void KEY3_OFF(void);
void KEY4_ON(void);
void KEY4_OFF(void);

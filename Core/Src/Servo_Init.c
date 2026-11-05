#include "main.h"
#include <math.h>

#define FAN_T_MIN_C 30.0f /* sub asta: inchis */
#define FAN_T_MAX_C 60.0f /* peste asta: deschis complet */
#define SERVO_MIN_US 500U
#define SERVO_MAX_US 2500U

/* ---------- Servo ventilator ---------- */
void Servo_Init(void)
{
    /* 1. Activare ceasuri
       RCC_IOPENR_GPIOBEN  = bit 1 din RCC->IOPENR  -> ceas pentru GPIOB (PB4)
       RCC_APBENR1_TIM3EN  = bit 1 din RCC->APBENR1 -> ceas pentru TIM3 */
    RCC->IOPENR |= (1U << 1);
    RCC->APBENR1 |= (1U << 1);
    (void)RCC->APBENR1;

    /* 2. PB4 -> alternate function AF1 (TIM3_CH1)
       MODER: 2 biti/pin, PB4 = biti [9:8]; 10 = alternate function
       AFR[0]: 4 biti/pin, PB4 = biti [19:16]; AF1 = 0001 */
    GPIOB->MODER &= ~(3U << (4 * 2));
    GPIOB->MODER |= (2U << (4 * 2));
    GPIOB->AFR[0] &= ~(0xFU << (4 * 4));
    GPIOB->AFR[0] |= (1U << (4 * 4)); // AF1

    TIM3->PSC = 47u; /* 48 MHz -> 47  */
    TIM3->ARR = 19999;
    TIM3->CCR1 = 1000u;

    /* Canal 1 in PWM mode 1
       CCMR1: biti [6:4] ; TIM_CCMR1_OC1M_Pos = 4 -> PWM mode 1 (HIGH cat CNT < CCR1, apoi LOW) */
    TIM3->CCMR1 = (6U << TIM_CCMR1_OC1M_Pos);

    /* CCER: bit 0 = CC1E -> conecteaza iesirea canalului 1 la pin (PB4) */
    TIM3->CCER |= TIM_CCER_CC1E;

    /* EGR: bit 0 = UG -> genereaza update event, incarca PSC/ARR/CCR1 in registrele active */
    TIM3->EGR |= TIM_EGR_UG;

    /* CR1: bit 0 = CEN -> porneste numaratorul */
    TIM3->CR1 |= TIM_CR1_CEN;
}

/* u = 0..100 [%]  ->  SERVO_MIN_US..SERVO_MAX_US */
void Servo_SetPercent(float u)
{
    if (u < 0.0f)   u = 0.0f;
    if (u > 100.0f) u = 100.0f;

    TIM3->CCR1 = SERVO_MIN_US + (uint32_t)(u * 0.01f * (SERVO_MAX_US - SERVO_MIN_US));
}
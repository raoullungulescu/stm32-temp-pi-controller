#include "main.h"
#include <math.h>

/* ---------- Control LED-uri ---------- */
void LED_Init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN;
    (void)RCC->IOPENR;

    /* Configurare LED T-WARN (PA9) */
    LED_TWARN_GPIO_Port->MODER &= ~(3U << (LED_TWARN_Pin * 2));
    LED_TWARN_GPIO_Port->MODER |=  (1U << (LED_TWARN_Pin * 2));

    /* Configurare LED P-WARN (PB5) */
    LED_PWARN_GPIO_Port->MODER &= ~(3U << (LED_PWARN_Pin * 2));
    LED_PWARN_GPIO_Port->MODER |=  (1U << (LED_PWARN_Pin * 2));

    /* Configurare LED Alarm (PA15) */
    LED_ALARM_GPIO_Port->MODER &= ~(3U << (LED_ALARM_Pin * 2));
    LED_ALARM_GPIO_Port->MODER |=  (1U << (LED_ALARM_Pin * 2));

    /* Configurare buzzer */
    BUZZER_GPIO_Port->MODER &= ~(3U << (BUZZER_Pin * 2));
    BUZZER_GPIO_Port->MODER |=  (1U << (BUZZER_Pin * 2));

    /* Stinge LED-ul de presiune la pornire */
    LED_PWARN_GPIO_Port->BSRR = (1U << (LED_PWARN_Pin + 16));

    LED_ALARM_GPIO_Port->BSRR = (1U << (LED_ALARM_Pin + 16));
}

void LED_TWarn_Set(uint8_t on)
{
    if (on)
        LED_TWARN_GPIO_Port->BSRR = (1U << LED_TWARN_Pin);
    else
        LED_TWARN_GPIO_Port->BSRR = (1U << (LED_TWARN_Pin + 16));
}

void LED_PWarn_Set(uint8_t on)
{
    if (on)
        LED_PWARN_GPIO_Port->BSRR = (1U << LED_PWARN_Pin);
    else
        LED_PWARN_GPIO_Port->BSRR = (1U << (LED_PWARN_Pin + 16));
}

void LED_Alarm(uint8_t on)
{
    if (on)
        LED_ALARM_GPIO_Port->BSRR = (1U << LED_ALARM_Pin);
    else
        LED_ALARM_GPIO_Port->BSRR = (1U << (LED_ALARM_Pin + 16));
}

void Buzzer_Alarm(uint8_t on)
{
    if (on)
        BUZZER_GPIO_Port->BSRR = (1U << BUZZER_Pin);
    else
        BUZZER_GPIO_Port->BSRR = (1U << (BUZZER_Pin + 16));
}
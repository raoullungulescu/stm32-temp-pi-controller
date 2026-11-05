#include <stdint.h>
#ifndef __MAIN_H
#define __MAIN_H

#define F_CPU 48000000U  
#define BTN_EV_NONE 0U
#define BTN_EV_UP   1U
#define BTN_EV_DN   2U   
         
float Temperature_Read(void);
extern float g_temp_bmp;
extern volatile uint32_t ms_tick;
extern float g_T_sim;
extern float g_dT;

/* ADC_Init.c */
void ADC_Init(void);
uint16_t ADC_Read(void);

// Led init
void LED_Init(void);
void LED_TWarn_Set(uint8_t on);
void LED_PWarn_Set(uint8_t on);
void LED_Alarm(uint8_t on);
void Buzzer_Alarm(uint8_t on);

// Button init
void Buttons_Init(void);
uint8_t Buttons_GetEvents(void);

/* Servo_Init.c */
void Servo_Init(void);
void Servo_SetPercent(float u);

/* OLED.c */
void OLED_Init(void);
void OLED_ShowTemp(float temp_C);
void OLED_ShowPress(float press_hPa);
void OLED_ALARM(uint8_t on);
void OLED_AVARIE(float temp_sim, float dt);
void OLED_ShowTsim(float t_sim);

/* ---------- Funcții Senzor Presiune BMP180 ---------- */
int BMP180_Init(void);
int BMP180_Read(float *temp_C, float *press_hPa);

/* ---------- Software I2C comun (soft_i2c.c) ---------- */
void    SoftI2C_Init(void);
void    SoftI2C_Start(void);
void    SoftI2C_Stop(void);
void    SoftI2C_WriteByte(uint8_t byte);
uint8_t SoftI2C_ReadByte(uint8_t ack);
void IWDG_Init(void);
void IWDG_Feed(void);

/* ---------- UART ---------- */
void UART_PutFixed2(int32_t v);
void UART_Init(void);
void UART_PutC(char c);
void UART_PutU32(uint32_t v);
void UART_PutI32(int32_t v);

/* ---------- Regulator ---------- */
void Regulator_Init(void);
void Regulator_Run(void);


#ifdef __cplusplus
extern "C" {
#endif

/* Header-ul CMSIS esențial care conține toate definițiile registrelor STM32C0 */
#include "stm32c0xx.h"

/* Definiții Pini și Porturi - Valori numerice pentru bit-shifting (0-15) */
#define RCC_OSCX_IN_Pin          14
#define RCC_OSCX_IN_GPIO_Port    GPIOC
#define RCC_OSCX_OUT_Pin         15
#define RCC_OSCX_OUT_GPIO_Port   GPIOC

#define RCC_OSC_IN_Pin           0
#define RCC_OSC_IN_GPIO_Port     GPIOF
#define RCC_OSC_OUT_Pin          1
#define RCC_OSC_OUT_GPIO_Port    GPIOF

#define SD_CS_Pin                0
#define SD_CS_GPIO_Port          GPIOB

#define BTN_MODE_Pin             10
#define BTN_MODE_GPIO_Port       GPIOB

#define BUZZER_Pin               9
#define BUZZER_GPIO_Port         GPIOA

#define LED_TWARN_Pin            7
#define LED_TWARN_GPIO_Port      GPIOC

#define BTN_UP_Pin               10
#define BTN_UP_GPIO_Port         GPIOA
#define BTN_UP_EXTI_IRQn         EXTI4_15_IRQn

#define DEBUG_SWDIO_Pin          13
#define DEBUG_SWDIO_GPIO_Port    GPIOA
#define DEBUG_SWCLK_Pin          14
#define DEBUG_SWCLK_GPIO_Port    GPIOA

#define LED_ALARM_Pin            15
#define LED_ALARM_GPIO_Port      GPIOA

#define BTN_DN_Pin               3
#define BTN_DN_GPIO_Port         GPIOB
#define BTN_DN_EXTI_IRQn         EXTI2_3_IRQn

#define LED_PWARN_Pin            5
#define LED_PWARN_GPIO_Port      GPIOB

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
#include "main.h"

#define SCL_HIGH()   (GPIOB->BSRR = GPIO_BSRR_BS8)
#define SCL_LOW()    (GPIOB->BSRR = GPIO_BSRR_BR8)
#define SDA_HIGH()   (GPIOB->BSRR = GPIO_BSRR_BS9)
#define SDA_LOW()    (GPIOB->BSRR = GPIO_BSRR_BR9)
#define SDA_READ()   (GPIOB->IDR & GPIO_IDR_ID9)

#define SDA_INPUT()  (GPIOB->MODER &= ~(3U << (9 * 2)))
#define SDA_OUTPUT() do { GPIOB->MODER &= ~(3U << (9 * 2)); GPIOB->MODER |= (1U << (9 * 2)); } while(0)

static void I2C_Delay(void) {
    for (volatile int i = 0; i < 15; i++) { __NOP(); }
}

void SoftI2C_Init(void) {
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    GPIOB->MODER  &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOB->MODER  |=  ((1U << (8 * 2)) | (1U << (9 * 2)));   /* output */
    GPIOB->OTYPER |=  ((1U << 8) | (1U << 9));               /* open-drain */
    GPIOB->PUPDR  &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOB->PUPDR  |=  ((1U << (8 * 2)) | (1U << (9 * 2)));   /* pull-up */

    SCL_HIGH();
    SDA_HIGH();
    I2C_Delay();
}

void SoftI2C_Start(void) {
    SDA_OUTPUT();
    SDA_HIGH(); SCL_HIGH(); I2C_Delay();
    SDA_LOW();  I2C_Delay();
    SCL_LOW();  I2C_Delay();
}

void SoftI2C_Stop(void) {
    SDA_OUTPUT();
    SDA_LOW();  SCL_LOW();  I2C_Delay();
    SCL_HIGH(); I2C_Delay();
    SDA_HIGH(); I2C_Delay();
}

void SoftI2C_WriteByte(uint8_t byte) {
    SDA_OUTPUT();
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & 0x80) SDA_HIGH();
        else             SDA_LOW();
        I2C_Delay();
        SCL_HIGH(); I2C_Delay();
        SCL_LOW();  I2C_Delay();
        byte <<= 1;
    }
    /* ignorăm ACK de la slave (simulator) */
    SDA_HIGH(); I2C_Delay(); SCL_HIGH(); I2C_Delay(); SCL_LOW(); I2C_Delay();
}

uint8_t SoftI2C_ReadByte(uint8_t ack) {
    uint8_t byte = 0;

    SDA_INPUT();
    I2C_Delay();
    for (uint8_t i = 0; i < 8; i++) {
        byte <<= 1;
        SCL_HIGH(); I2C_Delay();
        if (SDA_READ()) byte |= 0x01;
        SCL_LOW();  I2C_Delay();
    }

    SDA_OUTPUT();
    if (ack) SDA_LOW(); else SDA_HIGH();
    I2C_Delay();
    SCL_HIGH(); I2C_Delay();
    SCL_LOW();  I2C_Delay();
    SDA_HIGH();

    return byte;
}

// watchdog
void IWDG_Init(void)
{
    IWDG->KR  = 0x5555;   /* deblochează PR/RLR */
    IWDG->PR  = 3;        /* prescaler /32 */
    IWDG->RLR = 2000;     /* ~2 s */
    while (IWDG->SR) {}
    IWDG->KR  = 0xCCCC;   /* start */
}

void IWDG_Feed(void)
{
    IWDG->KR = 0xAAAA;
}
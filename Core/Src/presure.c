#include "main.h"

#define BMP180_ADDR      0x77   /* Adresa I2C 7 biți pentru BMP180 */

/* --- Registre BMP180 --- */
#define BMP180_REG_CALIB 0xAA
#define BMP180_REG_CTRL  0xF4
#define BMP180_REG_DATA  0xF6

#define CMD_READ_TEMP    0x2E
#define CMD_READ_PRESS0  0x34

/* Coeficienți de calibrare */
static int16_t  AC1, AC2, AC3, B1, B2, MB, MC, MD;
static uint16_t AC4, AC5, AC6;

static void Delay_Loop(volatile uint32_t count) {
    while (count--) { __NOP(); }
}

static void BMP180_WriteReg(uint8_t reg, uint8_t val) {
    SoftI2C_Start();
    SoftI2C_WriteByte(BMP180_ADDR << 1);
    SoftI2C_WriteByte(reg);
    SoftI2C_WriteByte(val);
    SoftI2C_Stop();
}

static void BMP180_ReadBuf(uint8_t reg, uint8_t *buf, uint8_t len) {
    SoftI2C_Start();
    SoftI2C_WriteByte(BMP180_ADDR << 1);
    SoftI2C_WriteByte(reg);

    SoftI2C_Start();
    SoftI2C_WriteByte((BMP180_ADDR << 1) | 0x01);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SoftI2C_ReadByte(i < (len - 1));
    }
    SoftI2C_Stop();
}

int BMP180_Init(void) {
    uint8_t b[22];

    SoftI2C_Init();

    BMP180_ReadBuf(BMP180_REG_CALIB, b, 22);

    AC1 = (int16_t)((b[0]  << 8) | b[1]);
    AC2 = (int16_t)((b[2]  << 8) | b[3]);
    AC3 = (int16_t)((b[4]  << 8) | b[5]);
    AC4 = (uint16_t)((b[6] << 8) | b[7]);
    AC5 = (uint16_t)((b[8] << 8) | b[9]);
    AC6 = (uint16_t)((b[10] << 8) | b[11]);
    B1  = (int16_t)((b[12] << 8) | b[13]);
    B2  = (int16_t)((b[14] << 8) | b[15]);
    MB  = (int16_t)((b[16] << 8) | b[17]);
    MC  = (int16_t)((b[18] << 8) | b[19]);
    MD  = (int16_t)((b[20] << 8) | b[21]);

    if (AC1 == 0 || AC1 == -1) return -1;
    return 0;
}

int BMP180_Read(float *temp_C, float *press_hPa) {
    uint8_t buf[3];
    int32_t UT, UP;
    int32_t X1, X2, X3, B3, B5, B6, p;
    uint32_t B4, B7;

    /* 1. Conversie Temperatură */
    BMP180_WriteReg(BMP180_REG_CTRL, CMD_READ_TEMP);
    Delay_Loop(80000);
    BMP180_ReadBuf(BMP180_REG_DATA, buf, 2);
    UT = (buf[0] << 8) | buf[1];

    /* 2. Conversie Presiune */
    BMP180_WriteReg(BMP180_REG_CTRL, CMD_READ_PRESS0);
    Delay_Loop(80000);
    BMP180_ReadBuf(BMP180_REG_DATA, buf, 3);
    UP = ((buf[0] << 16) | (buf[1] << 8) | buf[2]) >> 8;

    /* Algoritm calcul temperatură */
    X1 = ((UT - (int32_t)AC6) * (int32_t)AC5) >> 15;
    X2 = ((int32_t)MC << 11) / (X1 + (int32_t)MD);
    B5 = X1 + X2;
    if (temp_C) {
        *temp_C = ((B5 + 8) >> 4) / 10.0f;
    }

    /* Algoritm calcul presiune */
    B6 = B5 - 4000;
    X1 = ((int32_t)B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = ((int32_t)AC2 * B6) >> 11;
    X3 = X1 + X2;
    B3 = ((((int32_t)AC1 * 4 + X3)) + 2) / 4;

    X1 = ((int32_t)AC3 * B6) >> 13;
    X2 = ((int32_t)B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    B4 = ((uint32_t)AC4 * (uint32_t)(X3 + 32768)) >> 15;

    if (B4 == 0) return -1;

    B7 = ((uint32_t)UP - B3) * 50000;

    if (B7 < 0x80000000) {
        p = (B7 * 2) / B4;
    } else {
        p = (B7 / B4) * 2;
    }

    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p = p + ((X1 + X2 + 3791) >> 4);

    if (press_hPa) {
        *press_hPa = p / 100.0f;
    }

    return 0;
}
#include "main.h"

#define OLED_ADDR 0x3C /* SSD1306, adresa 7 biti (i2cAddress din diagram.json) */

/* ---------- Scriere pe bus: adresa + date ---------- */
static int I2C1_Write(uint8_t addr, const uint8_t *data, uint8_t len)
{
    SoftI2C_Start();
    SoftI2C_WriteByte(addr << 1); /* 0x3C shiftat la stânga devine 0x78 (write) */

    for (uint8_t i = 0; i < len; i++)
    {
        SoftI2C_WriteByte(data[i]);
    }

    SoftI2C_Stop();
    return 0;
}

/* ---------- SSD1306 ---------- */
static void OLED_Cmd(const uint8_t *cmds, uint8_t n)
{
    uint8_t buf[32];
    buf[0] = 0x00; /* control: comenzi */
    for (uint8_t i = 0; i < n; i++)
        buf[1 + i] = cmds[i];
    I2C1_Write(OLED_ADDR, buf, n + 1);
}

static void OLED_SetCursor(uint8_t col, uint8_t page)
{
    const uint8_t c[] = {(uint8_t)(0xB0 | page), (uint8_t)(col & 0x0F), (uint8_t)(0x10 | (col >> 4))};
    OLED_Cmd(c, sizeof(c));
}

static void OLED_Clear(void)
{
    uint8_t buf[17] = {0x40}; /* control: date, restul 0 */
    for (uint8_t i = 1; i < 17; i++)
        buf[i] = 0x00;

    for (uint8_t page = 0; page < 8; page++)
    {
        OLED_SetCursor(0, page);
        for (uint8_t k = 0; k < 8; k++)
        {
            I2C1_Write(OLED_ADDR, buf, sizeof(buf)); /* 16 coloane x 8 = 128 */
        }
    }
}

void OLED_Init(void)
{
    SoftI2C_Init();

    for (volatile int i = 0; i < 50000; i++)
        ; /* pauză inițializare fizică display */

    static const uint8_t init[] = {
        0xAE,       /* display OFF */
        0xD5, 0x80, /* clock divide */
        0xA8, 0x3F, /* multiplex 64 randuri */
        0xD3, 0x00, /* offset 0 */
        0x40,       /* start line 0 */
        0x8D, 0x14, /* charge pump ON */
        0x20, 0x02, /* page addressing mode */
        0xA0,       /* segment remap (ROTAȚIE corectă) */
        0xC0,       /* COM scan invers (ROTAȚIE corectă) */
        0xDA, 0x12, /* COM pins */
        0x81, 0xCF, /* contrast */
        0xD9, 0xF1, /* pre-charge */
        0xDB, 0x40, /* VCOMH */
        0xA4,       /* afiseaza RAM */
        0xA6,       /* normal (nu invers) */
        0xAF        /* display ON */
    };
    OLED_Cmd(init, sizeof(init));
    OLED_Clear();
}

/* ---------- Font 5x7 (doar caracterele necesare) ---------- */
static const char font_chars[] = " -.:0123456789CTemp*PhaALRMVIEstd"; /* '*' = simbolul grade */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* ' ' */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* '-' */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* '.' */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* ':' */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* '0' */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* '1' */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* '2' */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* '3' */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* '4' */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* '5' */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* '6' */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* '7' */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* '8' */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* '9' */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* 'C' */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* 'T' */
    {0x38, 0x54, 0x54, 0x54, 0x18}, /* 'e' */
    {0x7C, 0x04, 0x18, 0x04, 0x78}, /* 'm' */
    {0x7C, 0x14, 0x14, 0x14, 0x08}, /* 'p' */
    {0x00, 0x06, 0x09, 0x09, 0x06}, /* '*' = grade */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* 'P' */
    {0x7F, 0x08, 0x04, 0x04, 0x78}, /* 'h' */
    {0x20, 0x54, 0x54, 0x54, 0x78}, /* 'a' */
    {0x7E, 0x09, 0x09, 0x09, 0x7E}, /* 'A' */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* 'L' */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* 'R' */
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, /* 'M' */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* 'V' */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* 'I' */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* 'E' */
    {0x48, 0x54, 0x54, 0x54, 0x20}, /* 's' */
    {0x04, 0x3F, 0x44, 0x40, 0x20}, /* 't' */
    {0x38, 0x44, 0x44, 0x48, 0x7F}, /* 'd' */
};

static void OLED_PutChar(char c)
{
    uint8_t buf[7] = {0x40, 0, 0, 0, 0, 0, 0}; /* control + 5 coloane glif + 1 spatiu */
    for (uint8_t i = 0; font_chars[i] != '\0'; i++)
    {
        if (font_chars[i] == c)
        {
            for (uint8_t j = 0; j < 5; j++)
                buf[1 + j] = font5x7[i][j];
            break;
        }
    }
    I2C1_Write(OLED_ADDR, buf, sizeof(buf));
}

/* ce e desenat acum pe linie: 0 = nimic, 1 = temperatura, 2 = presiune, 3 = alarm, 4 = avarie */
static uint8_t shown = 0;

/* helper: formateaza "<prefix>-1234.5<sufix>" cu o zecimala, completat cu spatii pana la 12 caractere */
static void OLED_FmtFloat(char *s, const char *prefix, float v, const char *suffix)
{
    uint8_t n = 0;
    int32_t v10 = (int32_t)(v * 10.0f + (v >= 0.0f ? 0.5f : -0.5f));
    uint32_t a = (v10 < 0) ? (uint32_t)(-v10) : (uint32_t)v10;
    uint32_t ip = a / 10;

    while (*prefix)
        s[n++] = *prefix++;
    if (v10 < 0)
        s[n++] = '-';
    if (ip >= 1000)
        s[n++] = '0' + (ip / 1000) % 10;
    if (ip >= 100)
        s[n++] = '0' + (ip / 100) % 10;
    if (ip >= 10)
        s[n++] = '0' + (ip / 10) % 10;
    s[n++] = '0' + ip % 10;
    s[n++] = '.';
    s[n++] = '0' + a % 10;
    while (*suffix)
        s[n++] = *suffix++;
    while (n < 12)
        s[n++] = ' '; /* sterge ramasitele de la valoarea veche */
    s[n] = '\0';
}

/* ---------- Afisare temperatura ---------- */
void OLED_ShowTemp(float temp_C)
{
    static int32_t last = 0x7FFFFFFF;

    /* temperatura x10, rotunjita (ex: 25.3 -> 253) */
    int32_t t10 = (int32_t)(temp_C * 10.0f + (temp_C >= 0.0f ? 0.5f : -0.5f));
    if (shown == 1 && t10 == last)
        return; /* redesenam doar cand se schimba */
    if (shown == 4)
        OLED_Clear(); /* avaria foloseste mai multe linii */
    shown = 1;
    last = t10;

    char s[16];
    OLED_FmtFloat(s, "T: ", temp_C, "*C");

    OLED_SetCursor(0, 3); /* coloana 0, pagina 3 */
    for (uint8_t i = 0; s[i] != '\0'; i++)
    {
        OLED_PutChar(s[i]);
    }
}

/* ---------- Afisare presiune ---------- */
void OLED_ShowPress(float press_hPa)
{
    static int32_t last = 0xFF;

    int32_t p10 = (int32_t)(press_hPa * 10.0f + 0.5f); /* hPa x10 (ex: 1013.2 -> 10132) */
    if (p10 < 0)
        p10 = 0;
    if (shown == 2 && p10 == last)
        return;
    if (shown == 4)
        OLED_Clear();
    shown = 2;
    last = p10;

    char s[16];
    OLED_FmtFloat(s, "P: ", press_hPa, "hPa");

    OLED_SetCursor(0, 3);
    for (uint8_t i = 0; s[i] != '\0'; i++)
    {
        OLED_PutChar(s[i]);
    }
}

/* ---------- Afisare Alarma ---------- */
void OLED_ALARM(uint8_t on)
{
    static uint8_t last = 0x7FFFFFFF;

    if (shown == 3 && on == last)
        return;
    if (shown == 4)
        OLED_Clear();
    shown = 3;
    last = on;

    char s[13];
    uint8_t n = 0;

    if (on)
    {
        s[n++] = 'A';
        s[n++] = 'L';
        s[n++] = 'A';
        s[n++] = 'R';
        s[n++] = 'M';
    }
    while (n < 12)
        s[n++] = ' ';
    s[n] = '\0';

    OLED_SetCursor(0, 3);
    for (uint8_t i = 0; s[i] != '\0'; i++)
    {
        OLED_PutChar(s[i]);
    }
}

/* ---------- Afisare Avarie ---------- */
void OLED_AVARIE(float temp_sim, float dt)
{
    static int32_t last_t = 0x7FFFFFFF;
    static int32_t last_d = 0x7FFFFFFF;

    int32_t t10 = (int32_t)(temp_sim * 10.0f + (temp_sim >= 0.0f ? 0.5f : -0.5f));
    int32_t d10 = (int32_t)(dt * 10.0f + (dt >= 0.0f ? 0.5f : -0.5f));

    if (shown == 4 && t10 == last_t && d10 == last_d)
        return;
    shown = 4;
    last_t = t10;
    last_d = d10;

    char s[16];

    /* pagina 2: AVARIE */
    s[0] = 'A';
    s[1] = 'V';
    s[2] = 'A';
    s[3] = 'R';
    s[4] = 'I';
    s[5] = 'E';
    for (uint8_t i = 6; i < 12; i++)
        s[i] = ' ';
    s[12] = '\0';
    OLED_SetCursor(0, 2);
    for (uint8_t i = 0; s[i] != '\0'; i++)
    {
        OLED_PutChar(s[i]);
    }

    /* pagina 3: temperatura simulata */
    OLED_FmtFloat(s, "Ts: ", temp_sim, "*C");
    OLED_SetCursor(0, 3);
    for (uint8_t i = 0; s[i] != '\0'; i++)
    {
        OLED_PutChar(s[i]);
    }

    /* pagina 4: dt */
    OLED_FmtFloat(s, "dT: ", dt, "*C");
    OLED_SetCursor(0, 4);
    for (uint8_t i = 0; s[i] != '\0'; i++)
    {
        OLED_PutChar(s[i]);
    }
}

/* ---------- Afisare T_sim (dupa avarie) ---------- */
void OLED_ShowTsim(float t_sim)
{
    static int32_t last = 0x7FFFFFFF;

    int32_t t10 = (int32_t)(t_sim * 10.0f + (t_sim >= 0.0f ? 0.5f : -0.5f));
    if (shown == 5 && t10 == last)
        return;
    if (shown == 4)
        OLED_Clear(); /* avaria foloseste pagini 2-4 */
    shown = 5;
    last = t10;

    char s[16];
    OLED_FmtFloat(s, "Ts: ", t_sim, "*C");

    OLED_SetCursor(0, 3);
    for (uint8_t i = 0; s[i] != '\0'; i++)
    {
        OLED_PutChar(s[i]);
    }
}
#include "main.h"
#include <math.h>

#define ALARM_TEMP_C 70.0f
#define ALARM_PRESS_HPA 1000.0f /* 1 bar = 1000 hPa */
#define AVARIE_DT_C 0.1f        /* avarie cat timp |dT| > 0.1 degC */

typedef enum
{
    SHOW_TEMP = 0,
    SHOW_PRESS,
    SHOW_ALARM,
    SHOW_AVARIE,
    SHOW_TSIM /* T_sim afișat după ce dispare avaria */
} DispMode_t;

static DispMode_t disp_mode = SHOW_TEMP;
static float g_temp_C = -273.15f;

static float g_press_hPa = 0.0f;
static uint8_t g_press_ok = 0;

static uint8_t g_alarm_ok = 0;
static uint8_t g_blink = 0;

static uint8_t g_avarie = 0;
static uint8_t g_tsim_pending = 0; /* avaria a fost activă, așteaptă afișarea T_sim */

float g_temp_bmp = 30.0f; /* temperatura ambient, folosită de regulator */

/* setate de Regulator_Run, folosite pentru afisaj (extern in main.h) */
float g_T_sim = 0.0f;
float g_dT = 0.0f;

/* ---------- Temperatura ---------- */
float Temperature_Read(void)
{
    uint16_t adc_val = ADC_Read();

    if (adc_val == 0 || adc_val >= 4095)
    {
        return -273.15f;
    }

    float resistance = 10000.0f * adc_val / (4095.0f - adc_val);
    float temp_K = 1.0f / (1.0f / 298.15f + (1.0f / 3950.0f) * logf(resistance / 10000.0f));

    return temp_K - 273.15f;
}

/* ---------- Afișaj OLED (funcție de mod) ---------- */
static void Display_Update(void)
{
    switch (disp_mode)
    {
    case SHOW_TEMP:
        if (g_temp_C > -273.0f)
        {
            OLED_ShowTemp(g_temp_C);
        }
        break;

    case SHOW_PRESS:
        if (g_press_ok)
        {
            OLED_ShowPress(g_press_hPa);
        }
        break;

    case SHOW_ALARM:
        OLED_ALARM(g_blink);
        break;

    case SHOW_AVARIE:
        OLED_AVARIE(g_T_sim, g_dT);
        break;

    case SHOW_TSIM:
        OLED_ShowTsim(g_T_sim);
        break;

    default:
        break;
    }
}

/* ---------- Procesare evenimente butoane (apelată din main, nu din ISR) ---------- */
static void Buttons_Process(void)
{
    uint8_t ev = Buttons_GetEvents();   /* consumă mereu, ca să nu rămână evenimente vechi */

    if (g_alarm_ok || g_avarie)
        return;                         /* în alarmă/avarie butoanele sunt ignorate */

    DispMode_t m = disp_mode;

    if (ev & BTN_EV_UP)
        m = SHOW_TEMP;
    else if (ev & BTN_EV_DN)
        m = SHOW_PRESS;

    if (m != disp_mode)
    {
        disp_mode = m;
        Display_Update();               /* schimbare imediată */
    }
}

/* ---------- Verificare Alarme ---------- */
void Alarm_Check(void)
{
    /* 1. Temperatura */
    float temp_C = Temperature_Read();

    if (temp_C > -273.0f)
    {
        g_temp_C = temp_C;
        LED_TWarn_Set(temp_C > ALARM_TEMP_C);
    }

    /* 2. Presiune (BMP180) */
    float temp_bmp = 0.0f;
    float press_hPa = 0.0f;

    if (BMP180_Read(&temp_bmp, &press_hPa) == 0)
    {
        g_temp_bmp = temp_bmp; /* referință pentru regulator */
        g_press_hPa = press_hPa;
        g_press_ok = 1;
        LED_PWarn_Set(press_hPa > ALARM_PRESS_HPA);
    }

    /* 3. Alarmă */
    uint8_t alarm = (g_temp_C > ALARM_TEMP_C) &&
                    g_press_ok &&
                    (g_press_hPa > ALARM_PRESS_HPA);

    g_alarm_ok = alarm;

    if (alarm)
    {
        g_blink ^= 1; /* alternează la fiecare ciclu */
        LED_Alarm(g_blink);
        Buzzer_Alarm(g_blink);
    }
    else
    {
        g_blink = 0;
        LED_Alarm(0);
        Buzzer_Alarm(0);
    }

    /* 4. Avarie: cat timp dT = T_sim - T_bmp e in afara [-0.1, 0.1] */
    g_avarie = (fabsf(g_dT) > AVARIE_DT_C && g_dT >= 0);
    if (g_avarie)
        g_tsim_pending = 1; /* ține minte și dacă alarma o maschează */

    /* 5. Modul de afișare: ALARMA > AVARIE > T_sim (după avarie) */
    if (alarm)
        disp_mode = SHOW_ALARM;
    else if (g_avarie)
        disp_mode = SHOW_AVARIE;
    else if (g_tsim_pending)
    {
        disp_mode = SHOW_TSIM;
        g_tsim_pending = 0;
    }
    else if (disp_mode == SHOW_ALARM)
        disp_mode = SHOW_TEMP;

    /* 6. Afișaj conform modului activ */
    Display_Update();
}

/* ---------- Main ---------- */
int main(void)
{
    LED_Init();
    Buttons_Init();
    ADC_Init();
    Servo_Init();
    OLED_Init();
    BMP180_Init();
    Regulator_Init();   /* pornește SysTick (ms_tick), necesar pentru debounce */
    IWDG_Init();

    while (1)
    {
        Alarm_Check();
        Regulator_Run();
        Buttons_Process();
        IWDG_Feed();
    }
}
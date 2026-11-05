#include "main.h"

/* ====================================================================
 *  Regulator PI pe diferenta dT = T_sim - T_bmp
 *  Perturbatia = sliderul NTC (T_load), comanda = servo/ventilator (u)
 * ==================================================================== */

#define TS_MS      250U     /* perioada tinta de esantionare [ms]; Ts real se masoara mai jos */
#define DT_SET     0.0f     /* referinta pentru dT = T_sim - T_bmp [degC] */
#define PLANT_TAU  10.0f    /* planta simulata: constanta de timp [s] */
#define PLANT_KF   35.0f    /* ventilator 100% => -30 degC pe temperatura de echilibru */

/* ---------- Structura regulatorului PI ---------- */
typedef struct {
    float Kp;               /* castig proportional [%/degC] */
    float Ki;               /* castig integral [%/(degC*s)] */
    float Ts;               /* perioada de esantionare reala [s] */
    float integ;            /* starea integratorului [%] */
    float out_min;          /* limita inferioara a comenzii [%] */
    float out_max;          /* limita superioara a comenzii [%] */
} PI_t;

static PI_t reg = {
    .Kp      = 20.0f,
    .Ki      = 2.0f,
    .integ   = 0.0f,
    .out_min = 0.0f,
    .out_max = 100.0f
};

/* ---------- Un pas de regulator PI (cu anti-windup) ---------- */
static float PI_Step(PI_t *r, float ref, float meas)
{
    /* 1. Eroarea. Proces de racire: pozitiva cand e prea cald => ventilator mai mult */
    float e = meas - ref;

    /* 2. Termenul proportional */
    float p_term = r->Kp * e;

    /* 3. Comanda inainte de limitare = P + I */
    float v = p_term + r->integ;

    /* 4. Limitarea comenzii la [out_min, out_max] (servo: 0..100 %) */
    float u_sat = v;
    if (u_sat > r->out_max) u_sat = r->out_max;
    if (u_sat < r->out_min) u_sat = r->out_min;

    /* 5. Anti-windup (conditional integration):
          nu integrezi cand esti saturat si eroarea te impinge si mai adanc in saturatie */
    uint8_t integreaza = 1;
    if (v > r->out_max && e > 0.0f) integreaza = 0;   /* saturat sus, eroarea cere si mai mult */
    if (v < r->out_min && e < 0.0f) integreaza = 0;   /* saturat jos, eroarea cere si mai putin */

    if (integreaza) {
        r->integ += r->Ki * e * r->Ts;
    }

    /* 6. Comanda finala */
    return u_sat;
}

/* ---------- Timp: SysTick la 1 ms ---------- */
volatile uint32_t ms_tick;

void SysTick_Handler(void)
{
    ms_tick++;
}

/* ---------- Starea buclei ---------- */
float    T_sim;      /* temperatura simulata a NTC dupa efectul ventilatorului [degC] */
static float    u;          /* ultima comanda, 0..100 [%] (folosita de planta la pasul urmator) */
static uint32_t t_next;     /* momentul urmatorului pas de regulator [ms] */
static uint32_t t_last;     /* momentul pasului anterior [ms], pentru Ts real */
static uint8_t  started;    /* 0 = prima rulare */

void Regulator_Init(void)
{
    UART_Init();
    SysTick_Config(F_CPU / 1000U);          /* intrerupere SysTick la fiecare 1 ms */

    /* antet pentru log (coloanele ies in serial monitor  */
    const char *h = "ms,T_ntc,T_bmp,pert,T_sim,dT,u\r\n";
    while (*h) {
        UART_PutC(*h++);
    }
}

/*  ruleaza efectiv doar la fiecare TS_MS */
void Regulator_Run(void)
{
    /* 0. Verificare timp: iesi daca nu a trecut perioada */
    if ((int32_t)(ms_tick - t_next) < 0) {
        return;
    }
    t_next += TS_MS;

    /* 1. Citirea perturbatiei: NTC masurat  */
    float T_load = Temperature_Read();
    if (T_load < -273.0f) {
        return;                              /* ADC in saturatie */
    }

    /* 2. Perioada de esantionare reala (bucla dureaza mai mult de 250 ms) */
    if (!started) {
        T_sim  = T_load;                     /* pornesti din echilibru, fara tranzitoriu */
        t_last = ms_tick;
        started = 1;
    } else {
        reg.Ts = (ms_tick - t_last) * 0.001f;
        t_last = ms_tick;
    }

    /* 3. Planta simulata (ordin 1):
          temperatura de echilibru = T_load - efectul ventilatorului (u anterior)
          T_sim tinde spre ea cu constanta de timp PLANT_TAU */
    float T_eq = T_load - PLANT_KF * u / 100.0f;
    T_sim += (reg.Ts / PLANT_TAU) * (T_eq - T_sim);

    /* 4. Marimea reglata: diferenta fata de ambient (BMP180) */
    float dT = T_sim - g_temp_bmp;
    float pert = T_load - g_temp_bmp;

    // pentru afisaj
    g_T_sim = T_sim;
    g_dT    = dT;

    /* 5. Regulatorul calculeaza noua comanda */
    u = PI_Step(&reg, DT_SET, dT);

    /* 6. Aplica comanda pe servo (0..100 % -> 500..2500 us) */
    Servo_SetPercent(u);

/* 7. Log UART: ms,T_ntc,T_bmp,T_sim,dT,u */
UART_PutU32(ms_tick);                           UART_PutC(',');   /* 1. timp */
UART_PutFixed2((int32_t)(T_load * 100.0f));     UART_PutC(',');   /* 2. NTC masurat (ADC) */
UART_PutFixed2((int32_t)(g_temp_bmp * 100.0f)); UART_PutC(',');   /* 3. ambient (BMP180, I2C) */
UART_PutFixed2((int32_t)(pert * 100.0f));       UART_PutC(',');   /* 3. perturbatie */
UART_PutFixed2((int32_t)(T_sim * 100.0f));      UART_PutC(',');   /* 4. cum se schimba sistemul cu comanda */
UART_PutFixed2((int32_t)(dT * 100.0f));         UART_PutC(',');   /* 5. diferenta, tinde spre 0 */
UART_PutFixed2((int32_t)(u * 100.0f));          UART_PutC('\r');  /* 6. comanda aplicata */
UART_PutC('\n');
}
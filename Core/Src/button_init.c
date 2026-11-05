#include "main.h"

#define BTN_DEBOUNCE_MS 50U

static volatile uint8_t  btn_events = BTN_EV_NONE;  /* biți BTN_EV_UP / BTN_EV_DN */
static volatile uint32_t t_last_up  = 0;
static volatile uint32_t t_last_dn  = 0;

/* ---------- Butoane UP (PA10 = EXTI10) / DOWN (PB3 = EXTI3), front căzător ---------- */
void Buttons_Init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;
    (void)RCC->IOPENR;

    /* input (00) + pull-up (01) */
    BTN_UP_GPIO_Port->MODER &= ~(3U << (BTN_UP_Pin * 2));
    BTN_UP_GPIO_Port->PUPDR &= ~(3U << (BTN_UP_Pin * 2));
    BTN_UP_GPIO_Port->PUPDR |=  (1U << (BTN_UP_Pin * 2));

    BTN_DN_GPIO_Port->MODER &= ~(3U << (BTN_DN_Pin * 2));
    BTN_DN_GPIO_Port->PUPDR &= ~(3U << (BTN_DN_Pin * 2));
    BTN_DN_GPIO_Port->PUPDR |=  (1U << (BTN_DN_Pin * 2));

    /* Rutare linie EXTI -> port (EXTICR: câte un octet per linie; 0 = PA, 1 = PB)
       EXTI10: EXTICR[2] biții 23:16 -> PA */
    EXTI->EXTICR[2] &= ~(0xFFU << 16);
    EXTI->EXTICR[2] |=  (0U    << 16);
    /* EXTI3: EXTICR[0] biții 31:24 -> PB */
    EXTI->EXTICR[0] &= ~(0xFFU << 24);
    EXTI->EXTICR[0] |=  (1U    << 24);

    /* Front: doar căzător (buton la GND), fără front crescător */
    EXTI->RTSR1 &= ~((1U << BTN_UP_Pin) | (1U << BTN_DN_Pin));
    EXTI->FTSR1 |=  ((1U << BTN_UP_Pin) | (1U << BTN_DN_Pin));

    /* Șterge orice pending vechi (scrii 1 ca să ștergi), apoi demascheaza liniile */
    EXTI->FPR1 = (1U << BTN_UP_Pin) | (1U << BTN_DN_Pin);
    EXTI->IMR1 |= (1U << BTN_UP_Pin) | (1U << BTN_DN_Pin);

    /* NVIC */
    NVIC_SetPriority(BTN_UP_EXTI_IRQn, 1);
    NVIC_SetPriority(BTN_DN_EXTI_IRQn, 1);
    NVIC_EnableIRQ(BTN_UP_EXTI_IRQn);
    NVIC_EnableIRQ(BTN_DN_EXTI_IRQn);
}

/* ---------- ISR-uri: scurte, fără I2C/OLED/delay ---------- */
void EXTI4_15_IRQHandler(void)
{
    if (EXTI->FPR1 & (1U << BTN_UP_Pin))
    {
        EXTI->FPR1 = (1U << BTN_UP_Pin);                      /* clear pending */
        if ((uint32_t)(ms_tick - t_last_up) >= BTN_DEBOUNCE_MS)
        {
            t_last_up   = ms_tick;
            btn_events |= BTN_EV_UP;
        }
    }
}

void EXTI2_3_IRQHandler(void)
{
    if (EXTI->FPR1 & (1U << BTN_DN_Pin))
    {
        EXTI->FPR1 = (1U << BTN_DN_Pin);
        if ((uint32_t)(ms_tick - t_last_dn) >= BTN_DEBOUNCE_MS)
        {
            t_last_dn   = ms_tick;
            btn_events |= BTN_EV_DN;
        }
    }
}

/* ---------- Citește și șterge evenimentele (atomic) ---------- */
uint8_t Buttons_GetEvents(void)
{
    __disable_irq();
    uint8_t ev = btn_events;
    btn_events = BTN_EV_NONE;
    __enable_irq();
    return ev;
}
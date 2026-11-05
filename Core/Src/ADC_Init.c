#include "main.h"
#include <math.h>

/* ---------- ADC ---------- */
void ADC_Init(void)
{
    /* 1. Activare ceas ADC
       RCC_APBENR2_ADCEN = bit 20  */
    RCC->APBENR2 |= RCC_APBENR2_ADCEN;

    /* 2. PA0 analog */
    GPIOA->MODER |= (3U << (0 * 2));

    /* 3. Pornire ADC */
    /* ADC_ISR_ADRDY = bit 0  Flag-ul se sterge scriind 1 in el */
    ADC1->ISR |= ADC_ISR_ADRDY;

    /* ADC_CR_ADEN = bit 0  Activeaza ADC-ul */
    ADC1->CR |= ADC_CR_ADEN;

    /* Asteapta ADRDY = 1 (bit 0 din ISR): ADC-ul e gata de conversii */
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0)
        ;

    // 4. Selectare canal
    ADC1->CHSELR |= (1U << 0);
}

uint16_t ADC_Read(void)
{
    /* ADC_CR_ADSTART = bit 2 din ADC1->CR
       Porneste o conversie */
    ADC1->CR |= ADC_CR_ADSTART;

    while ((ADC1->ISR & ADC_ISR_EOC) == 0)
        ;

    //  DR = registrul de date
    return ADC1->DR;
}
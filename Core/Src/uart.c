#include "main.h"

void UART_Init(void)
{
    /* Ceasuri: GPIOA = bit 0 din IOPENR, USART2 = bit 17 din APBENR1 */
    RCC->IOPENR  |= (1U << 0);
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;
    (void)RCC->APBENR1;

    /* PA2 = TX, PA3 = RX: MODER 10 = alternate function, AFR = AF1 (USART2) */
    GPIOA->MODER  &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOA->MODER  |=  ((2U << (2 * 2)) | (2U << (3 * 2)));
    GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
    GPIOA->AFR[0] |=  ((1U << (2 * 4)) | (1U << (3 * 4)));

    /* BRR = f_clk / baud (OVER8 = 0, oversampling 16) */
    USART2->BRR = F_CPU / 115200U;

    /* CR1: bit 3 = TE (emitator), bit 0 = UE (USART enable) */
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

// scrie caracter
void UART_PutC(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE_TXFNF));   /* bit 7: TDR gol, poti scrie */
    USART2->TDR = (uint8_t)c;
}

// scrie cifra 
void UART_PutU32(uint32_t v)
{
    char buf[10];
    int n = 0;
    do { 
        buf[n++] = '0' + (v % 10U); 
        v /= 10U; 
    }
    while (v);
    while (n) UART_PutC(buf[--n]);
}

void UART_PutI32(int32_t v)
{
    if (v < 0) 
    { UART_PutC('-'); UART_PutU32((uint32_t)(-(v + 1)) + 1U); }
    else       
    { UART_PutU32((uint32_t)v); }
}

/* v = valoare x100  ->  "-12.34"  fara printf avantaj memorie*/
void UART_PutFixed2(int32_t v)
{
    if (v < 0) 
    { UART_PutC('-'); v = -v; }
    UART_PutU32((uint32_t)(v / 100)); 
    UART_PutC('.');
    UART_PutC('0' + (v % 100) / 10);
    UART_PutC('0' + (v % 10));
}
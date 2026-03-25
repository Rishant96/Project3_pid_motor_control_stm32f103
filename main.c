#include "MCU_STM32.h"

static void clock_init(void)
{
	FLASH_R->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTBE;
	
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY));
	
	RCC->CR |= RCC_CR_CSSON;
	
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL9;

	/* Turn on PLL and wait */
	RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));
	
	/* Switch system clock to PLL */
	RCC->CFGR = (RCC->CFGR & ~(3U << 0)) | RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL);
}

static void gpio_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN
				  |  RCC_APB2ENR_IOPCEN
				  |  RCC_APB2ENR_AFIOEN
				  |  RCC_APB2ENR_USART1EN;
	
	/* Setup Pin 13 */
	GPIOC->CRH &= ~(0xFU << 20);
	GPIOC->CRH |=  (0x2U << 20);
	
	/* Setup Pin 0 */
	GPIOA->CRL &= ~(0xFU << 0);
	GPIOA->CRL |=  (0x8U << 0);
	GPIOA->BSRR = (1U << 0);
	
	/* Setup Pin 9 */
	GPIOA->CRH &= ~(0xFU << 4);
	GPIOA->CRH |=  (0xBU << 4);
	
	/* Setup Pin 11 */
	GPIOA->CRH &= ~(0xFU << 12);
	GPIOA->CRH |=  (0x8U << 12);
	
	/* Setup Pin 12 */
	GPIOA->CRH &= ~(0xFU << 14);
	GPIOA->CRH |=  (0xBU << 14);
}

static void uart_putc(char c)
{
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = (uint32_t)c;
}

static void uart_write(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

static void uart_init(void)
{
	USART1->BRR = 0x271;
	USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}

static void tim2_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	
	TIM2->PSC = 7199;
	TIM2->ARR = 9999;
	TIM2->DIER = TIM_DIER_UIE;
	TIM2->CR1  = TIM_CR1_CEN;
	
	NVIC_ISER0 = (1U << IRQ_TIM2);
}

static void exti0_init(void)
{
	EXTI->FTSR |= (1U << 0);
	EXTI->IMR  |= (1U << 0);
	
	NVIC_ISER0 = (1U << IRQ_EXTI0);
}

void NMI_Handler(void)
{
	if (RCC->CIR & RCC_CIR_CSSF) {
		RCC->CIR |= RCC_CIR_CSSC;
		uart_write("HSE Failure\r\n");
	}
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;   /* Clear interrupt flag */
        GPIOC->ODR ^= (1U << 13);  /* Toggle LED */
        uart_write("tick\r\n");
    }
}

void EXTI0_IRQHandler(void)
{
	if (EXTI->PR & (1U << 0)) {
		EXTI->PR = (1U << 0);
		uart_write("button!\r\n");
	}
}

static void can1_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
}

int main(void)
{
	clock_init();
	gpio_init();
	uart_init();
	tim2_init();
	exti0_init();
	can1_init();
	
	uart_write("Project 1 - Bare Metal STM32 Programming\r\n");

	for (;;) { } /* Using interrupts for flow control */

}
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
	GPIOA->CRL |=  (0xBU << 0);
	
	/* Setup Pin 9 */
	GPIOA->CRH &= ~(0xFU << 4);
	GPIOA->CRH |=  (0xBU << 4);
	
	/* Setup Pin 1 */
	GPIOA->CRL &= ~(0xFU << 4);
}

static void adc1_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	
    ADC1->SQR3 = 1;

    ADC1->CR2 |= ADC_CR2_ADON;

    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL);
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL);
}

static uint16_t adc1_read(void)
{
    ADC1->CR2 |= ADC_CR2_ADON;
    while (!(ADC1->SR & ADC_SR_EOC));
    return (uint16_t)ADC1->DR;
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

static void uart_print_num(uint16_t val)
{
    char buf[6];
    int i = 0;

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        uart_putc(buf[--i]);
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
	TIM2->PSC = 71;
	TIM2->ARR = 49;
	TIM2->CCR1 = 25;
	
	TIM2->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
	TIM2->CCER  = TIM_CCER_CC1E;
	
	TIM2->CR1 = TIM_CR1_CEN;
}

void NMI_Handler(void)
{
	if (RCC->CIR & RCC_CIR_CSSF) {
		RCC->CIR |= RCC_CIR_CSSC;
		uart_write("HSE Failure\r\n");
	}
}

typedef struct {
  int32_t kp, ki, kd;
  int32_t integral, integral_max;
  int32_t prev_error;
  int32_t output_min, output_max;
} PID_State;

int32_t pid_update(PID_State *pid, int32_t setpoint, int32_t measurement)
{
    int32_t error;
    int32_t derivative;
    int32_t output;

    error = setpoint - measurement;

    pid->integral += error;
    if (pid->integral > pid->integral_max)  pid->integral = pid->integral_max;
    if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;

    derivative = error - pid->prev_error;

    output = (pid->kp * error
            + pid->ki * pid->integral
            + pid->kd * derivative) >> 8;

    pid->prev_error = error;

    if (output > pid->output_max) output = pid->output_max;
    if (output < pid->output_min) output = pid->output_min;

    return output;
}

static void delay(volatile uint32_t count)
{
    while (count--);
}

int main(void)
{
    PID_State pid;
	pid.kp = 256;
	pid.ki = 0;
	pid.kd = 0;
	pid.integral = 0;
	pid.prev_error = 0;
	pid.integral_max = 500;
	pid.output_min = 0;
	pid.output_max = 49;
	
    uint16_t adc_val;
    int32_t output;
	
	clock_init();
	gpio_init();
	adc1_init();
	uart_init();
	tim2_init();
	
    uart_write("Project 3 - PID Motor Control\r\n");

    for (;;) {
        adc_val = adc1_read();
        output = pid_update(&pid, 2048, (int32_t)adc_val);
        TIM2->CCR1 = (uint32_t)output;

        uart_write("ADC: ");
        uart_print_num(adc_val);
        uart_write(" PWM: ");
        uart_print_num((uint16_t)output);
        uart_write("\r\n");

        delay(720000);
    } /* Using interrupts for flow control */

}
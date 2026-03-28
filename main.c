#include "MCU_STM32.h"

static void delay(volatile uint32_t count)
{
    while (count--);
}

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

static adc_count_t adc1_read(void)
{
	adc_count_t result;
    ADC1->CR2 |= ADC_CR2_ADON;
    while (!(ADC1->SR & ADC_SR_EOC)) { };
	result.raw = (uint16_t)ADC1->DR;
	Assert(result.raw <= 4095);
    return result;
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

static void uart_print_unum(uint32_t val)
{
    char buf[11];
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

static void uart_print_num(int32_t val)
{
	if (val < 0) { uart_putc('-'); uart_print_unum(-val); }
    else uart_print_unum(val);
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

pid_update_result pid_update(pid_state_t *pid, adc_count_t setpoint, adc_count_t measurement)
{
	pid_update_result result;
	
    int32_t error;
    int32_t derivative;
    int32_t output;
	
	Assert(pid != 0);
	Assert(pid->output_max > pid->output_min);  /* catches ZII trap */

    error = setpoint.raw - measurement.raw;

    pid->integral += error;
    if (pid->integral > pid->integral_max)  pid->integral = pid->integral_max;
    if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;

    derivative = error - pid->prev_error;

    output = fixed16_to_int(pid->kp.raw * error
            + pid->ki.raw * pid->integral
            + pid->kd.raw * derivative);

    pid->prev_error = error;

    if (output > pid->output_max) output = pid->output_max;
    if (output < pid->output_min) output = pid->output_min;

	result.duty_cycle.raw = output;
	result.error = (int16_t)error;
    return result;
}

int main(void)
{
	static uint32_t tick;
	
    pid_state_t pid;
    pid.kp.raw = fixed16_Kp_from_frac(49, 4095);
    pid.ki.raw = fixed16_Ki(0, 5, 30);
    pid.kd.raw = fixed16_Kd(0, 50);
    pid.integral = 0;
    pid.prev_error = 0;
    pid.integral_max = 500;
    pid.output_min = 0;
    pid.output_max = 49;    /* MUST set — zero clamps all output */
	
	clock_init();
	gpio_init();
	adc1_init();
	uart_init();
	tim2_init();
	
    uart_write("Project 3 - PID Motor Control\r\n");
	tick = 0;
    for (;;) {
		pid_update_result output;
		adc_count_t setpoint, adc_val;
		
        adc_val = adc1_read();
		/* disabling pid for now */
#if 1
		setpoint.raw = 2048;
        output = pid_update(&pid, setpoint, adc_val);
#else
		setpoint.raw = 0;
		output.duty_cycle.raw = (adc_val.raw * pid.output_max) / 4096;
#endif
        TIM2->CCR1 = (uint32_t)output.duty_cycle.raw;

        uart_print_num(tick * 33); /* 33ms is estimated loop time */
		uart_putc(',');
		uart_print_num(setpoint.raw);
		uart_putc(',');
		uart_print_num(adc_val.raw);
		uart_putc(',');
		uart_print_num(output.error);
		uart_putc(',');
		uart_print_num(output.duty_cycle.raw);
        uart_write("\r\n");

        delay(720000); /* Replace with SysTick Sunday */
		tick++;
    } /* Using interrupts for flow control */
}

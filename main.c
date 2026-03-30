#include "MCU_STM32.h"

static uint8_t uart_tx_buf[256];
static ring_buf_t uart_tx_ring;
static volatile int edge_count = 0;

static volatile uint32_t sys_tick_ms = 0;
static volatile uint32_t last_edge_ms = 0;

static void delay(volatile uint32_t count)
{
    while (count--);
}

static void rb_init(ring_buf_t *buffer, uint8_t *array, uint16_t capacity)
{
	Assert(capacity > 0);
	Assert(buffer != 0);	
	Assert(array != 0);
	Assert((capacity & (capacity - 1)) == 0);
	
	buffer->base = array;
	buffer->capacity = capacity;
	buffer->mask = capacity - 1;
	buffer->head = buffer->tail = 0;
}

static int8_t rb_get(ring_buf_t *rb, uint8_t *byte)
{
	int8_t result;
	uint16_t next;
	
	Assert(rb != 0);
	Assert(rb->base != 0);
	next = (rb->tail + 1) & rb->mask;
	
	if (rb->head != rb->tail)
	{
		*byte = rb->base[rb->tail];
		__DMB();
		rb->tail = next;
		result = 0;
	}
	else
	{
		result = -1;
	}
	
	return result;
}

static int8_t rb_put(ring_buf_t *rb, uint8_t byte)
{
	int8_t result;
	uint16_t next;
	
	Assert(rb != 0);
	Assert(rb->base != 0);
	next = (rb->head + 1) & rb->mask;
	
	if (next != rb->tail)
	{
		rb->base[rb->head] = byte;
		__DMB();
		rb->head = next;
		result = 0;
	}
	else
	{
		result = -1;
	}
	
	return result;
}

static int8_t rb_puts(ring_buf_t *rb, const char *msg)
{
	int8_t result;

	Assert(rb != 0);	
	Assert(rb->base != 0);
	Assert(msg != 0);
	
	result = 0;
	
	while(*msg != 0 && result == 0)
	{
		result = rb_put(rb, *msg);
		msg++;
	}
	
	return result;
}

static int8_t rb_put_unum(ring_buf_t *rb, uint32_t val)
{
	int8_t result = -1, i = 0;
	char buf[11];
	
	if (val == 0) {
        result = rb_put(rb, '0');
        return result;
    }

    while (val > 0) 
	{
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) 
	{
        result = rb_put(rb, buf[--i]);
    }
	
	return result;
}

static int8_t rb_put_num(ring_buf_t *rb, int32_t val)
{
	int8_t result = -1;
	
	if (val < 0) { result = rb_put(rb, '-'); result = rb_put_unum(rb, -val); }
    else result = rb_put_unum(rb, val);
	
	return result;
}

static void uart_putc(char c)
{
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = (uint32_t)c;
}

static void uart_write(const char *s)
{
    while (*s) 
	{
        uart_putc(*s++);
    }
}

/*
static void uart_print_unum(uint32_t val)
{
    char buf[11];
    int i = 0;

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0) 
	{
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) 
	{
        uart_putc(buf[--i]);
    }
}

static void uart_print_num(int32_t val)
{
	if (val < 0) { uart_putc('-'); uart_print_unum(-val); }
    else uart_print_unum(val);
}
*/

static void uart_init(void)
{
	USART1->BRR = 0x271;
	USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
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
				  |  RCC_APB2ENR_IOPBEN
				  |  RCC_APB2ENR_IOPCEN
				  |  RCC_APB2ENR_AFIOEN
				  |  RCC_APB2ENR_USART1EN;
	
	/* Setup Pin A0 */
	GPIOA->CRL &= ~(0xFU << 0);
	GPIOA->CRL |=  (0xBU << 0);
	
	/* Setup Pin A1 */
	GPIOA->CRL &= ~(0xFU << 4);
	
	/* Setup Pin A9 */
	GPIOA->CRH &= ~(0xFU << 4);
	GPIOA->CRH |=  (0xBU << 4);
	
	/* Setup Pin A10 */
	GPIOA->CRH &= ~(0xFU << 8);
	GPIOA->CRH |=  (0x4U << 8);
	
	/* Setup Pin B10 */
	GPIOB->CRH &= ~(0xFU << 8);
	GPIOB->CRH |=  (0x4U << 8);
		
	/* Setup Pin C13 */
	GPIOC->CRH &= ~(0xFU << 20);
	GPIOC->CRH |=  (0x2U << 20);
}

static void exti10_init(void)
{
	AFIO->EXTICR[2] = (0x1 << 8);

	EXTI->FTSR |= (1U << 10);
	EXTI->IMR  |= (1U << 10);
	
	NVIC_ISER1 = (1U << 8);
}

void EXTI15_10_IRQHandler(void)
{
	uint32_t now;

	if (EXTI->PR & (1U << 10)) {
		EXTI->PR = (1U << 10);
		
		now = sys_tick_ms;
		if ((now - last_edge_ms) >= 10) {
			edge_count++;
			last_edge_ms = now;
		}
	}
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

void NMI_Handler(void)
{
	if (RCC->CIR & RCC_CIR_CSSF) {
		rb_puts(&uart_tx_ring, "HSE Failure\r\n");
		RCC->CIR |= RCC_CIR_CSSC;
	}
}

void SysTick_Handler(void)
{
	sys_tick_ms++;
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

static uint8_t usart_pid_cmd_type_match_64(char *buffer, uint8_t len, const char *token)
{
	uint8_t result, pos;
	
	if (len == 0 || *token == '\0')
	{
		result = 0;
	}
	else
	{
		pos = 0;
		result = 1;
		
		while (pos < len)
		{
			if (token[pos] == '\0' || token[pos] != buffer[pos])
			{
				result = 0;
				break;
			}
			pos++;
		}
		
		if (token[len] != '\0') result = 0;
	}
	
	return result;
}

static int16_t parse_int(char *int_str, uint8_t len)
{
	int16_t result;
	uint8_t pos, base;
	
	result = 0; 
	base = 1;
	pos = len;
	
	Assert(len > 0);
	
	while (pos-- > 0)
	{
		char c = int_str[pos];
		Assert( c >= '0' && c <= '9');
		
		result += (c - '0') * base;
		
		base *= 10;
	}
	
	return result;
}

static usart_pid_cmd usart_pid_cmd_parse_64(cmd_buffer_64 *buffer)
{
	usart_pid_cmd result;
	uint8_t pos, name_len;
	char *c;
	
	result.type = PID_CMD_COUNT;
	result.param.raw = 0;
	
	pos = 0;
	c = buffer->line;
	name_len = buffer->count;
		
	Assert(*c != ' ' && buffer->count > 0);	
		
	while (pos++ < buffer->count && *c++ != ' ' )
	{
		name_len = pos;
	}
	
	/* get param */
	if (name_len != buffer->count) result.param.raw = fixed16_from_int(parse_int(c, buffer->count - pos));
	
	/* get type */
	if (usart_pid_cmd_type_match_64(buffer->line, name_len, "KP"))
	{
		result.type = PID_CMD_KP;
	}
	else if (usart_pid_cmd_type_match_64(buffer->line, name_len, "KI"))
	{
		result.type = PID_CMD_KI;
	}
	else if (usart_pid_cmd_type_match_64(buffer->line, name_len, "KD"))
	{
		result.type = PID_CMD_KD;
	}
	else if (usart_pid_cmd_type_match_64(buffer->line, name_len, "SET"))
	{
		result.type = PID_CMD_SET;
	}	
	else if (usart_pid_cmd_type_match_64(buffer->line, name_len, "INSPECT"))
	{
		result.type = PID_CMD_INSPECT;
	}
	
	return result;
}

int main(void)
{
	static uint32_t tick, current_rpm = 0;
	cmd_buffer_64 usart_cmd_64;
	int i;
    pid_state_t pid;
	fixed16_t tmp_kp, tmp_ki, tmp_kd;
	
    pid.kp.raw = tmp_kp.raw = fixed16_Kp_from_frac(49, 4095);
    pid.ki.raw = tmp_ki.raw = fixed16_Ki(0, 5, 30);
    pid.kd.raw = tmp_kd.raw = fixed16_Kd(0, 50);
    pid.integral = 0;
    pid.prev_error = 0;
    pid.integral_max = 500;
    pid.output_min = 0;
    pid.output_max = 49;    /* MUST set — zero clamps all output */
		
	for (i = 0; i < PID_CMD_BUFFER_MAX; i++)
	{
		usart_cmd_64.line[i] = 0;
	}
	usart_cmd_64.count = 0;
	
	rb_init(&uart_tx_ring, uart_tx_buf, sizeof(uart_tx_buf));
	
	clock_init();
	SYSTICK_RVR = 71999;
	SYSTICK_CVR = 0;
	SYSTICK_CSR = 0X7;
	gpio_init();
	uart_init();
	adc1_init();
	tim2_init();
	exti10_init();
	
    uart_write("Project 3 - PID Motor Control\r\n");
	tick = 0;
    for (;;) {
		{
			static uint32_t last_rpm_ms = 0;
			static uint32_t last_edge_snapshot = 0;

			if ((sys_tick_ms - last_rpm_ms) >= 1000) {
				uint32_t edges = edge_count - last_edge_snapshot;
				current_rpm = edges * 60;
				last_edge_snapshot = edge_count;
				last_rpm_ms = sys_tick_ms;
			}
		}
		{
			uint8_t c;
			while (rb_get(&uart_tx_ring, &c) == 0)
			{
				while (!(USART1->SR & USART_SR_TXE));
				USART1->DR = c;
			}
		}
		{
			usart_pid_cmd req;
			if (USART1->SR & USART_SR_RXNE)
			{
				char c = USART1->DR & 0xFF;
				if (c == '\r' || c == '\n')
				{
					if (usart_cmd_64.count > 0)
					{
						req = usart_pid_cmd_parse_64(&usart_cmd_64);
						switch (req.type)
						{
							case PID_CMD_KP:
							{
								tmp_kp = req.param;
							} break;
							
							case PID_CMD_KI:
							{
								tmp_ki = req.param;
							} break;
							
							case PID_CMD_KD:
							{
								tmp_kd = req.param;
							} break;
							
							case PID_CMD_SET:
							{
								pid.kp = tmp_kp;
								pid.ki = tmp_ki;
								pid.kd = tmp_kd;
								rb_puts(&uart_tx_ring, "Setting... \r\n");
							} break;
							
							case PID_CMD_INSPECT:
							{
								rb_put_num(&uart_tx_ring, pid.kp.raw);
								rb_put(&uart_tx_ring, ',');
								rb_put_num(&uart_tx_ring, pid.ki.raw);
								rb_put(&uart_tx_ring, ',');
								rb_put_num(&uart_tx_ring, pid.kd.raw);
								rb_puts(&uart_tx_ring, "\r\n");
							} break;
							
							case PID_CMD_COUNT:
							{
								rb_puts(&uart_tx_ring, "ERR: unknown\r\n");
							} break;
						}
					}
					for (i = 0; i < PID_CMD_BUFFER_MAX; i++)
					{
						usart_cmd_64.line[i] = 0;
					}
					usart_cmd_64.count = 0;
				}
				else if (usart_cmd_64.count < PID_CMD_BUFFER_MAX)
				{
					usart_cmd_64.line[usart_cmd_64.count++] = c;
				}
				
			}
		}
		{
			pid_update_result output;
			adc_count_t setpoint, adc_val;
			
			adc_val = adc1_read();
			/* disabling pid for now */
#if 0
			setpoint.raw = 2048;
			output = pid_update(&pid, setpoint, adc_val);
#else
			setpoint.raw = 0;
			output.duty_cycle.raw = (adc_val.raw * pid.output_max) / 4096;
			output.error = 0;
#endif
			TIM2->CCR1 = (uint32_t)output.duty_cycle.raw;

			if (tick % 5 == 0)
			{
				rb_put_num(&uart_tx_ring, tick * 33); /* 33ms is estimated loop time */
				rb_put(&uart_tx_ring, ',');
				rb_put_num(&uart_tx_ring, setpoint.raw);
				rb_put(&uart_tx_ring, ',');
				rb_put_num(&uart_tx_ring, adc_val.raw);
				rb_put(&uart_tx_ring, ',');
				rb_put_num(&uart_tx_ring, output.error);
				rb_put(&uart_tx_ring, ',');
				rb_put_num(&uart_tx_ring, output.duty_cycle.raw);
				rb_put(&uart_tx_ring, ',');
				rb_put_num(&uart_tx_ring, edge_count);
				rb_put(&uart_tx_ring, ',');
				rb_put_num(&uart_tx_ring, sys_tick_ms);
				rb_put(&uart_tx_ring, ',');
				rb_put_num(&uart_tx_ring, current_rpm);
				rb_puts(&uart_tx_ring, "\r\n");
			}
		}
        delay(720000); /* Replace with SysTick Sunday */
		tick++;
    } /* Using interrupts for flow control */
}

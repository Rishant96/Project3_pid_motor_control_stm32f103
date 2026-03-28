#ifndef MCU_STM32_h
#define MCU_STM32_h

#ifdef DEBUG
	#define Assert(expr) do { if (!(expr)) { __asm volatile("bkpt #0"); } } while(0)
#else
	#define Assert(expr) ((void)0)
#endif

#include <stdint.h>

#define PERIPH_BASE 	((uint32_t)0x40000000)
#define APB1_BASE       (PERIPH_BASE + 0x00000000)
#define APB2_BASE       (PERIPH_BASE + 0x00010000)
#define AHB_BASE    	(PERIPH_BASE + 0x00020000)
#define CAN1_BASE       (PERIPH_BASE + 0x00006400)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_t;

#define RCC               ((RCC_t *)(AHB_BASE + 0x1000))


#define RCC_CR_HSION      (1U << 0)
#define RCC_CR_HSIRDY     (1U << 1)
#define RCC_CR_HSEON      (1U << 16)
#define RCC_CR_HSERDY     (1U << 17)
#define RCC_CR_CSSON	  (1U << 19)
#define RCC_CR_PLLON      (1U << 24)
#define RCC_CR_PLLRDY     (1U << 25)


#define RCC_CFGR_SW_HSI   (0U << 0)
#define RCC_CFGR_SW_HSE   (1U << 0)
#define RCC_CFGR_SW_PLL   (2U << 0)
#define RCC_CFGR_SWS_MASK (3U << 2)
#define RCC_CFGR_SWS_PLL  (2U << 2)
#define RCC_CFGR_PPRE1_Pos 8U
#define RCC_CFGR_PPRE1_DIV2  (4U << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PLLSRC   (1U << 16)
#define RCC_CFGR_PLLMUL9  (7U << 18)

#define RCC_CIR_CSSF 	  (1U << 7)
#define RCC_CIR_CSSC 	  (1U << 23)

#define RCC_APB2ENR_AFIOEN   (1U << 0)
#define RCC_APB2ENR_IOPAEN   (1U << 2)
#define RCC_APB2ENR_IOPBEN   (1U << 3)
#define RCC_APB2ENR_IOPCEN   (1U << 4)
#define RCC_APB2ENR_USART1EN (1U << 14)

#define RCC_APB1ENR_TIM2EN   (1U << 0)
#define RCC_APB1ENR_CAN1EN   (1U << 25)

#define RCC_APB2ENR_ADC1EN (1U << 9)

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_t;

#define GPIOA             ((GPIO_t *)(APB2_BASE + 0x0800))
#define GPIOB             ((GPIO_t *)(APB2_BASE + 0x0C00))
#define GPIOC             ((GPIO_t *)(APB2_BASE + 0x1000))

typedef struct {
    volatile uint32_t CR1;          /* 0x00 */
    volatile uint32_t CR2;          /* 0x04 */
    volatile uint32_t SMCR;         /* 0x08 */
    volatile uint32_t DIER;         /* 0x0C */
    volatile uint32_t SR;           /* 0x10 */
    volatile uint32_t EGR;          /* 0x14 */
    volatile uint32_t CCMR1;        /* 0x18 */
    volatile uint32_t CCMR2;        /* 0x1C */
    volatile uint32_t CCER;         /* 0x20 */
    volatile uint32_t CNT;          /* 0x24 */
    volatile uint32_t PSC;          /* 0x28 */
    volatile uint32_t ARR;          /* 0x2C */
    volatile uint32_t _reserved;    /* 0x30 */
    volatile uint32_t CCR1;         /* 0x34 */
    volatile uint32_t CCR2;         /* 0x38 */
    volatile uint32_t CCR3;         /* 0x3C */
    volatile uint32_t CCR4;         /* 0x40 */
    volatile uint32_t _reserved2;   /* 0x44 */
    volatile uint32_t DCR;          /* 0x48 */
    volatile uint32_t DMAR;         /* 0x4C */
} TIM_t;

#define TIM2              ((TIM_t *)(APB1_BASE + 0x0000))

#define TIM_CR1_CEN       (1U << 0) 
#define TIM_DIER_UIE      (1U << 0) 
#define TIM_SR_UIF        (1U << 0)   

#define TIM_CCMR1_OC1PE   (1U << 3)
#define TIM_CCMR1_OC1M_PWM1  (0x6U << 4)
#define TIM_CCER_CC1E     (1U << 0)

typedef struct {
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} EXTI_t;

#define EXTI              ((EXTI_t *)(APB2_BASE + 0x0400))

#define NVIC_ISER0        (*(volatile uint32_t *)0xE000E100)
#define NVIC_ISER1        (*(volatile uint32_t *)0xE000E104)

#define IRQ_EXTI0         6
#define IRQ_TIM2          28

typedef struct {
    volatile uint32_t ACR;          /* 0x00 */
    volatile uint32_t KEYR;         /* 0x04 */
    volatile uint32_t OPTKEYR;      /* 0x08 */
    volatile uint32_t SR;           /* 0x0C */
    volatile uint32_t CR;           /* 0x10 */
    volatile uint32_t AR;           /* 0x14 */
    volatile uint32_t _reserved;    /* 0x18 */
    volatile uint32_t OBR;          /* 0x1C */
    volatile uint32_t WRPR;         /* 0x20 */
} FLASH_t;

#define FLASH_R					 ((FLASH_t *)0x40022000)
#define FLASH_ACR_LATENCY_2WS    (2U << 0U) /* 2 wait states for frequencies 48-72 MHz */
#define FLASH_ACR_PRFTBE  		 (1U << 4U) /* enabling Prefetch buffer improves performance */

typedef struct {
    volatile uint32_t SR; 
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_t;

#define USART1            ((USART_t *)(APB2_BASE + 0x3800))

#define USART_SR_TXE      (1U << 7)

#define USART_CR1_UE      (1U << 13)
#define USART_CR1_TE      (1U << 3)

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR1;
    volatile uint32_t JOFR2;
    volatile uint32_t JOFR3;
    volatile uint32_t JOFR4;
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;
    volatile uint32_t JDR1;
    volatile uint32_t JDR2;
    volatile uint32_t JDR3;
    volatile uint32_t JDR4;
    volatile uint32_t DR;
} ADC_t;

#define ADC1              ((ADC_t *)(APB2_BASE + 0x2400))

#define ADC_SR_EOC        (1U << 1)
#define ADC_CR2_ADON      (1U << 0)
#define ADC_CR2_CAL       (1U << 2)
#define ADC_CR2_RSTCAL    (1U << 3)

typedef struct { int16_t raw; } fixed16_t;

#define FIXED16_SHIFT 8
#define fixed16_from_int(x) ((int16_t)((x) << FIXED16_SHIFT))
#define fixed16_to_int(x)   ((int)(x) >> FIXED16_SHIFT)

#define fixed16_Kp_from_frac(output_max, error_at_saturation) \
		  ((int16_t)(((output_max) << FIXED16_SHIFT) / (error_at_saturation)))

#define fixed16_Ki(extra_output, error, loops_per_sec) \
		  ((int16_t)(((extra_output) * 256) / ((error) * (loops_per_sec))))
		  
#define fixed16_Kd(output_adj, error_change) \
		  ((int16_t)(((output_adj) << FIXED16_SHIFT) / (error_change)))

typedef struct { uint16_t raw; } adc_count_t;
typedef struct { uint8_t  raw; } duty_t;

typedef struct {
	fixed16_t kp, ki, kd;
	int16_t prev_error;
	int32_t integral;
	int32_t integral_max;    /* MUST set before use — zero clamps integral */
	int16_t output_min;
	int16_t output_max;      /* MUST set before use — zero clamps all output */
} pid_state_t;

typedef struct {
	duty_t duty_cycle;
	uint8_t _;
	int16_t error;
} pid_update_result;

#endif

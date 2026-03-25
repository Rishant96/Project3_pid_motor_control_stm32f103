#include <stdint.h>

extern int main(void);

extern uint32_t _estack;
extern uint32_t _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern uint32_t _etext;
extern uint32_t _sidata;


/* Handlers */
void Reset_Handler(void);
void Default_Handler(void);
/* Handlers the user can override (weak linkage) */
void NMI_Handler(void)             __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)             __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)         __attribute__((weak, alias("Default_Handler")));

/* IRQ handlers - only the ones we use get real implementations in main.c */
void WWDG_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void TAMPER_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));  /* IRQ6 */
void EXTI1_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC1_2_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void USB_HP_CAN_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USB_LP_CAN_RX0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN_RX1_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void CAN_SCE_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));  /* IRQ28 */
void TIM3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void RTCAlarm_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void USBWakeup_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector")))
void (* const vector_table[])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMPER_IRQHandler,
    RTC_IRQHandler,
    FLASH_IRQHandler, 
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Channel1_IRQHandler,
    DMA1_Channel2_IRQHandler,
    DMA1_Channel3_IRQHandler,
    DMA1_Channel4_IRQHandler,
    DMA1_Channel5_IRQHandler,
    DMA1_Channel6_IRQHandler,
    DMA1_Channel7_IRQHandler,
    ADC1_2_IRQHandler,
    USB_HP_CAN_TX_IRQHandler,
    USB_LP_CAN_RX0_IRQHandler,
    CAN_RX1_IRQHandler,
    CAN_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_IRQHandler,
    TIM1_UP_IRQHandler,
    TIM1_TRG_COM_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler, 
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler, 
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTCAlarm_IRQHandler,
    USBWakeup_IRQHandler,
};

void Reset_Handler(void)
{
    /* Copy data */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata)
        *dst++ = *src++;

    /* Zero BSS */
    dst = &_sbss;
    while (dst < &_ebss)
        *dst++ = 0;

    main();

    for (;;);
}

void Default_Handler(void)
{
    for (;;);
}

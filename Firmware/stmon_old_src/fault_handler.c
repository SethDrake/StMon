#include "stm32f10x.h"

void hard_fault_handler_c (uint32_t * hardfault_args)
{
    volatile uint32_t stacked_r0 __attribute__((unused));
    volatile uint32_t stacked_r1 __attribute__((unused));
    volatile uint32_t stacked_r2 __attribute__((unused));
    volatile uint32_t stacked_r3 __attribute__((unused));
    volatile uint32_t stacked_r12 __attribute__((unused));
    volatile uint32_t stacked_lr __attribute__((unused));
    volatile uint32_t stacked_pc __attribute__((unused));
    volatile uint32_t stacked_psr __attribute__((unused));

    // Bus Fault Address Register
    volatile uint32_t BFAR __attribute__((unused));
    // Configurable Fault Status Register Consists of MMSR, BFSR and UFSR
    volatile uint32_t CFSR __attribute__((unused));
    // Hard Fault Status Register
    volatile uint32_t HFSR __attribute__((unused));
    // Debug Fault Status Register
    volatile uint32_t DFSR __attribute__((unused));
    // Auxiliary Fault Status Register
    volatile uint32_t AFSR __attribute__((unused));
    // MemManage Fault Address Register
    volatile uint32_t MMAR __attribute__((unused));

    volatile uint32_t SCB_SHCSR __attribute__((unused));

    stacked_r0 = hardfault_args[0];
    stacked_r1 = hardfault_args[1];
    stacked_r2 = hardfault_args[2];
    stacked_r3 = hardfault_args[3];
    stacked_r12 = hardfault_args[4];
    stacked_lr = hardfault_args[5];
    stacked_pc = hardfault_args[6];
    stacked_psr = hardfault_args[7];

    BFAR = (*((volatile unsigned long *)(0xE000ED38)));
    CFSR = (*((volatile unsigned long *)(0xE000ED28)));
    HFSR = (*((volatile unsigned long *)(0xE000ED2C)));
    DFSR = (*((volatile unsigned long *)(0xE000ED30)));
    AFSR = (*((volatile unsigned long *)(0xE000ED3C)));
    MMAR = (*((volatile unsigned long *)(0xE000ED34)));
    SCB_SHCSR = SCB->SHCSR;

//    DBG_HALT(0);
    log_message("Hard Fault.\r\nSystem Halted...");
    while (1);
}

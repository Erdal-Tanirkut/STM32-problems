#include "main.h"

// Soft Timer Struct Definition
typedef struct {
    uint32_t duration;
    uint32_t remaining;
    uint8_t active;
} SoftTimer_t;

#define MAX_SOFT_TIMERS 10
SoftTimer_t SoftTimers[MAX_SOFT_TIMERS];

void SystemClock_Config(void);
void TIM2_Config(void);
void AddSoftTimer(uint32_t duration, uint8_t autoStart);
void StartSoftTimer(uint8_t timerID);
void StopSoftTimer(uint8_t timerID);
void UpdateSoftTimer(uint8_t timerID, uint32_t newDuration);

int main(void)
{
    // System Initialization
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    SystemClock_Config();
    TIM2_Config();

    // Initialize Soft Timers
    for (int i = 0; i < MAX_SOFT_TIMERS; i++) {
        SoftTimers[i].active = 0;
    }

    // Add a soft timer with 100 ms duration and auto start
    AddSoftTimer(100, 1);

    while (1) {
        // Main loop
    }
}

void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
    while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3) {}
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSE_Enable();
    while(LL_RCC_HSE_IsReady() != 1) {}
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();
    while(LL_RCC_PLL_IsReady() != 1) {}
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}
    LL_Init1msTick(168000000);
    LL_SetSystemCoreClock(168000000);
}

void TIM2_Config(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
    LL_TIM_SetPrescaler(TIM2, __LL_TIM_CALC_PSC(SystemCoreClock, 10000)); // 10 kHz timer clock
    LL_TIM_SetAutoReload(TIM2, 10 - 1); // 10 ms interrupt
    LL_TIM_EnableIT_UPDATE(TIM2);
    LL_TIM_EnableCounter(TIM2);
    NVIC_SetPriority(TIM2_IRQn, 0);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM2)) {
        LL_TIM_ClearFlag_UPDATE(TIM2);
        for (int i = 0; i < MAX_SOFT_TIMERS; i++) {
            if (SoftTimers[i].active && SoftTimers[i].remaining > 0) {
                SoftTimers[i].remaining--;
                if (SoftTimers[i].remaining == 0) {
                    SoftTimers[i].active = 0;
                    // Timer has expired, handle timer expiration here
                }
            }
        }
    }
}

void AddSoftTimer(uint32_t duration, uint8_t autoStart)
{
    for (int i = 0; i < MAX_SOFT_TIMERS; i++) {
        if (!SoftTimers[i].active) {
            SoftTimers[i].duration = duration;
            SoftTimers[i].remaining = duration;
            SoftTimers[i].active = autoStart;
            break;
        }
    }
}

void StartSoftTimer(uint8_t timerID)
{
    if (timerID < MAX_SOFT_TIMERS) {
        SoftTimers[timerID].active = 1;
        SoftTimers[timerID].remaining = SoftTimers[timerID].duration;
    }
}

void StopSoftTimer(uint8_t timerID)
{
    if (timerID < MAX_SOFT_TIMERS) {
        SoftTimers[timerID].active = 0;
    }
}

void UpdateSoftTimer(uint8_t timerID, uint32_t newDuration)
{
    if (timerID < MAX_SOFT_TIMERS) {
        SoftTimers[timerID].duration = newDuration;
        if (SoftTimers[timerID].active) {
            SoftTimers[timerID].remaining = newDuration;
        }
    }
}

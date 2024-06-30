#include "main.h"
#include <string.h>
#include <stdio.h>

// USART1 Configuration
void USART1_Config(void);
void USART1_Transmit(char *string);
void USART1_IRQHandler(void);

// Timer Configuration
void TIM2_Config(void);
void TIM2_IRQHandler(void);
void SystemClock_Config(void);

// Command Processing
void ProcessCommand(char *cmd);

// Buffer for incoming USART data
#define RX_BUFFER_SIZE 64
char rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t cmd_received = 0;

// LED Toggle variables
uint32_t led_toggle_period = 1000; // Default 1 second
uint8_t led_running = 0;

int main(void)
{
    // System Initialization
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    SystemClock_Config();
    TIM2_Config();
    USART1_Config();

    // Initialize LED (Assuming LED is connected to PA5)
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_OUTPUT);

    while (1)
    {
        if (cmd_received)
        {
            cmd_received = 0;
            ProcessCommand(rx_buffer);
            rx_index = 0;
            memset(rx_buffer, 0, RX_BUFFER_SIZE);
        }
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
    LL_TIM_SetAutoReload(TIM2, 1000 - 1); // Default 1 second interrupt
    LL_TIM_EnableIT_UPDATE(TIM2);
    LL_TIM_EnableCounter(TIM2);
    NVIC_SetPriority(TIM2_IRQn, 0);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM2))
    {
        LL_TIM_ClearFlag_UPDATE(TIM2);
        if (led_running)
        {
            LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_5);
        }
    }
}

void USART1_Config(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    // Configure PA9 (TX) and PA10 (RX)
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF_7);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_10, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF_7);

    // Configure USART1
    LL_USART_SetTransferDirection(USART1, LL_USART_DIRECTION_TX_RX);
    LL_USART_SetParity(USART1, LL_USART_PARITY_NONE);
    LL_USART_SetDataWidth(USART1, LL_USART_DATAWIDTH_8B);
    LL_USART_SetStopBitsLength(USART1, LL_USART_STOPBITS_1);
    LL_USART_SetBaudRate(USART1, SystemCoreClock / 4, LL_USART_OVERSAMPLING_16, 9600);
    LL_USART_EnableIT_RXNE(USART1);
    LL_USART_Enable(USART1);

    // Enable USART1 interrupt
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART1))
    {
        char received_char = LL_USART_ReceiveData8(USART1);
        if (received_char == '\n' || rx_index >= RX_BUFFER_SIZE - 1)
        {
            cmd_received = 1;
            rx_buffer[rx_index] = '\0';
        }
        else
        {
            rx_buffer[rx_index++] = received_char;
        }
    }
}

void USART1_Transmit(char *string)
{
    for (uint32_t i = 0; i < strlen(string); i++)
    {
        while (!LL_USART_IsActiveFlag_TXE(USART1));
        LL_USART_TransmitData8(USART1, string[i]);
    }
    while (!LL_USART_IsActiveFlag_TC(USART1));
}

void ProcessCommand(char *cmd)
{
    if (strncmp(cmd, "Led", 3) == 0)
    {
        if (strncmp(cmd, "LedStop", 7) == 0)
        {
            led_running = 0;
            LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_5);
            USART1_Transmit("OK\n");
        }
        else if (strncmp(cmd, "LedStart", 8) == 0)
        {
            led_running = 1;
            USART1_Transmit("OK\n");
        }
        else if (strncmp(cmd, "Led", 3) == 0)
        {
            char *end;
            uint32_t duration = strtoul(cmd + 3, &end, 10);
            if (*end == 'm' && *(end + 1) == 's' && duration >= 1 && duration <= 1000)
            {
                led_toggle_period = duration;
                LL_TIM_SetAutoReload(TIM2, led_toggle_period * 10 - 1);
                USART1_Transmit("OK\n");
            }
            else if (*end == 's' && duration >= 1 && duration <= 10)
            {
                led_toggle_period = duration * 1000;
                LL_TIM_SetAutoReload(TIM2, led_toggle_period * 10 - 1);
                USART1_Transmit("OK\n");
            }
            else
            {
                USART1_Transmit("Invalid value\n");
            }
        }
        else
        {
            USART1_Transmit("CmdError\n");
        }
    }
    else
    {
        USART1_Transmit("CmdError\n");
    }
}

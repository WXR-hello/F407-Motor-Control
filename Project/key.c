
#include "stm32f4xx.h"                  // Device header
void Key_Init(){   
GPIO_InitTypeDef GPIO_InitStruct = {0};

// 1. 开启GPIO时钟，例如GPIOE
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_GPIOG_CLK_ENABLE();
__HAL_RCC_GPIOC_CLK_ENABLE();
// 2. 配置引脚参数
GPIO_InitStruct.Pin = GPIO_PIN_0;                // 引脚号
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;          // 输入模式
GPIO_InitStruct.Pull = GPIO_NOPULL;              // 上拉；GPIO_PULLDOWN下拉；GPIO_NOPULL浮空
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

GPIO_InitStruct.Pin=GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;
GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
GPIO_InitStruct.Pull=GPIO_NOPULL;
HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);


GPIO_InitStruct.Pin=GPIO_PIN_13;
GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
GPIO_InitStruct.Pull=GPIO_NOPULL;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}




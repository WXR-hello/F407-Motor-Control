/**
  ******************************************************************************
  * @file    bsp_lcd_display.c
  * @author  embedfire
  * @brief   
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

#include "bsp_lcd_display.h"

SRAM_HandleTypeDef hlcd_8080;

void LCD_Display_FSMC_Init(void)
{
    /* Peripheral clock enable */
    __HAL_RCC_FSMC_CLK_ENABLE();    
    
    FSMC_NORSRAM_TimingTypeDef Timing = {0};

    /** Perform the SRAM1 memory initialization sequence
    */
    hlcd_8080.Instance = FSMC_NORSRAM_DEVICE;
    hlcd_8080.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    /* hsram1.Init */
    hlcd_8080.Init.NSBank = FSMC_Bank1_NORSRAMx;
    hlcd_8080.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hlcd_8080.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    hlcd_8080.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    hlcd_8080.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hlcd_8080.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hlcd_8080.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hlcd_8080.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hlcd_8080.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hlcd_8080.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hlcd_8080.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
    hlcd_8080.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hlcd_8080.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    hlcd_8080.Init.PageSize = FSMC_PAGE_SIZE_NONE;
    /* Timing */
    Timing.AddressSetupTime = 0x04;
    Timing.AddressHoldTime = 0x00;
    Timing.DataSetupTime = 0x04;
    Timing.BusTurnAroundDuration = 0x00;
    Timing.CLKDivision = 0x00;
    Timing.DataLatency = 0x00;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;
    /* ExtTiming */

    if (HAL_SRAM_Init(&hlcd_8080, &Timing, &Timing) != HAL_OK)
    {
        while(1);
    }         
}

/**
  * @brief  ILI9341（3.2 / 2.8 寸电阻屏）专用的 FSMC 时序
  *
  *   野火官方 ILI9341 例程用的是"模式 B + 数据保持 15 个 HCLK"，
  *   和 NT35510 / ILI9806G 用的"模式 A + 4 个 HCLK"不一样。
  *   认屏时先按 NT35510/ILI9806G 的时序读 ID，读不到再切到这套时序，
  *   两套参数各自都是官方验证过的值。
  */
void LCD_Display_FSMC_Init_ILI9341(void)
{
    FSMC_NORSRAM_TimingTypeDef Timing = {0};

    hlcd_8080.Instance = FSMC_NORSRAM_DEVICE;
    hlcd_8080.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    hlcd_8080.Init.NSBank = FSMC_Bank1_NORSRAMx;
    hlcd_8080.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hlcd_8080.Init.MemoryType = FSMC_MEMORY_TYPE_NOR;
    hlcd_8080.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    hlcd_8080.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hlcd_8080.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hlcd_8080.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hlcd_8080.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hlcd_8080.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hlcd_8080.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hlcd_8080.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
    hlcd_8080.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hlcd_8080.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    hlcd_8080.Init.PageSize = FSMC_PAGE_SIZE_NONE;
    /* 时序：地址建立 4 个 HCLK，数据保持 15 个 HCLK，模式 B */
    Timing.AddressSetupTime = 0x04;
    Timing.AddressHoldTime = 0x00;
    Timing.DataSetupTime = 0x0F;
    Timing.BusTurnAroundDuration = 0x00;
    Timing.CLKDivision = 0x00;
    Timing.DataLatency = 0x00;
    Timing.AccessMode = FSMC_ACCESS_MODE_B;

    if (HAL_SRAM_Init(&hlcd_8080, &Timing, &Timing) != HAL_OK)
    {
        while(1);
    }
}



void LCD_Display_Pin_Init(void)
{
    /* FSMC LCD 8080 GPIO
   
    PD0   ------>  FSMC_D2  
    PD1   ------>  FSMC_D3
    PD4   ------>  FSMC_NOE  / LCD_RD
    PD5   ------>  FSMC_NWE  / LCD_WR
    PD8   ------>  FSMC_D13
    PD9   ------>  FSMC_D14
    PD10  ------>  FSMC_D15
    PD14  ------>  FSMC_D0
    PD15  ------>  FSMC_D1
    
    PE7   ------>  FSMC_D4  
    PE8   ------>  FSMC_D5
    PE9   ------>  FSMC_D6
    PE10  ------>  FSMC_D7
    PE11  ------>  FSMC_D8
    PE12  ------>  FSMC_D9
    PE13  ------>  FSMC_D10
    PE14  ------>  FSMC_D11
    PE15  ------>  FSMC_D12

    PG10  ------>  FSMC_NE3  / LCD_CS

    PF0   ------>  FSMC_A0   / LCD_RS
        
    */
   
    GPIO_InitTypeDef GPIO_InitStruct ={0};    

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14
                          |GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);    
    
       
    /*  
        PG9   ------>   Not used         
        PE5   ------>   LCD_BL
        PF11  ------>   LCD_RST    
    */      
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
     
    /* LCD复位低电平有效，可将其上拉 */
    GPIO_InitStruct.Pin = LCD_RST_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_RST_GPIO_PORT, &GPIO_InitStruct);   
    
    
    /* LCD背光高电平使能，可将其下拉 */
    GPIO_InitStruct.Pin = LCD_BL_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_BL_GPIO_PORT, &GPIO_InitStruct);   
}


void LCD_Display_Rest(void)
{
	HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN ,GPIO_PIN_RESET);
	HAL_Delay ( 50 ); 					   

	HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN ,GPIO_PIN_SET);		 
	HAL_Delay ( 50 ); 	 
}

void LCD_Display_BL_ENABLE(uint8_t state)
{
	if ( state == 1 )
    {
		 HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN ,GPIO_PIN_SET);
    }        
	else if ( state == 0 )
    {
        HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN ,GPIO_PIN_RESET);
    } 

}

/*********************************************END OF FILE**********************/

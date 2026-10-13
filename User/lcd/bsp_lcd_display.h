#ifndef __BSP_LCD_DISPLAY_H
#define	__BSP_LCD_DISPLAY_H

#include "stm32f4xx.h"

/*NE脚对应BANK3 控制AO脚 命令操作地址  */
#define     LCD_8080_ADDR_CMD             0x68000000
/*NE脚对应BANK3 控制AO脚 数据操作地址  */
#define     LCD_8080_ADDR_DATA            0x68000002

//由片选引脚决定的NOR/SRAM块
#define     FSMC_Bank1_NORSRAMx           FSMC_NORSRAM_BANK3

#define     LCD_WRITE_CMD(CMD)            *(uint16_t*)LCD_8080_ADDR_CMD = CMD
#define     LCD_WRITE_DATA(DATA)          *(uint16_t*)LCD_8080_ADDR_DATA = DATA
#define     LCD_READ_DATA()               (*(volatile uint16_t*)LCD_8080_ADDR_DATA)


/*LCD背光控制引脚*/
#define     LCD_BL_GPIO_PIN               GPIO_PIN_5
#define     LCD_BL_GPIO_PORT              GPIOE
/*LCD复位引脚*/
#define     LCD_RST_GPIO_PIN              GPIO_PIN_11
#define     LCD_RST_GPIO_PORT             GPIOF



void LCD_Display_FSMC_Init(void);
void LCD_Display_FSMC_Init_ILI9341(void);
void LCD_Display_Pin_Init(void);
void LCD_Display_Rest(void);
void LCD_Display_BL_ENABLE(uint8_t state);

#endif /* __BSP_LCD_DISPLAY_H */

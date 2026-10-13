#ifndef __LCD_DRAW_H
#define	__LCD_DRAW_H

#include <stdint.h>
#include <stdio.h>
#include "fonts.h"

/* 定义 LCD 驱动芯片 ID */
#define     LCDID_UNKNOWN             0

#define WHITE                 0xFFFF //白色
#define BLACK                 0x0000 //黑色
#define BLUE                  0x001F //蓝色
#define BRED                  0xF81F //红色
#define GRED                  0xFFE0 //红色
#define GBLUE                 0x07FF //蓝色
#define RED                   0xF800 //红色
#define MAGENTA               0xF81F //品红
#define GREEN                 0x07E0 //绿色
#define CYAN                  0x7FFF //青色
#define YELLOW                0xFFE0 //黄色
#define BROWN                 0xBC40 //棕色
#define BRRED                 0xFC07 //棕红色
#define GRAY                  0x8430 //灰色

extern uint16_t LCD_X_LENGTH;
extern uint16_t LCD_Y_LENGTH;
extern uint8_t LCD_SCAN_MODE;
/*当前识别到的面板 ID（0 = 面板没回应）；用来给板上的 LED 报信*/
extern uint16_t LCD_PANEL_ID;

extern void (*lcd_gram_scan)(uint8_t ucOption);    
extern void (*lcd_open_window)(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
extern void (*lcd_set_pixel)(uint16_t sx, uint16_t sy,uint16_t color);
extern void (*lcd_fill_color)(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,uint16_t color);  

uint8_t LCD_Draw_Config(void);
void LCD_Draw_Line ( uint16_t usX1, uint16_t usY1, uint16_t usX2, uint16_t usY2, uint16_t color );
void LCD_DispChar_EN ( uint16_t usX, uint16_t usY, const char cChar, uint16_t TextColor, uint16_t BackColor, sFONT *LCD_Currentfonts);
void LCD_DispString_EN ( uint16_t usX ,uint16_t usY, char * pStr, uint16_t TextColor, uint16_t BackColor, sFONT *LCD_Currentfonts);
void LCD_DispChar_Bitmap(uint16_t start_x, uint16_t start_y, 
                              uint16_t index, uint16_t fg_color, 
                              uint16_t bg_color, FontBitmap_t *font);
void LCD_DispString_Bitmap(uint16_t start_x, uint16_t start_y, 
                              const uint16_t *indices, uint8_t count,
                              uint16_t TextColor, uint16_t BackColor, 
                              FontBitmap_t *font);
#endif /* __LCD_DRAW_H */

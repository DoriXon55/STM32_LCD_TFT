#pragma once

#include <stdint.h>

//=======MACRO POMAGAJĄCE W DETEKCJI CZY JEST TO KOMENDA CZY DANE==========
#define CMD(x)			((x) | 0x100)


//===================KOMENDY DO WYŚWIETLACZA, JEDNOBAJTOWE=================
#define ST7735S_SLPOUT			0x11 	//sleep out
#define ST7735S_DISPOFF			0x28 	//display off
#define ST7735S_DISPON			0x29 	// display on
#define ST7735S_CASET			0x2a	// ustawia początkową i końcową kolumnę rysowanego obszaru
#define ST7735S_RASET			0x2b	// ustawia początkowy i końcowy wiersz rysowanego obrazu
#define ST7735S_RAMWR			0x2c	// rozpoczyna przesyłanie danych do zdefiniowanego obszaru
#define ST7735S_MADCTL			0x36	// read display MADCTL
#define ST7735S_COLMOD			0x3a	// read display pixel format
//===================FRAME RATE CONTROL=================
#define ST7735S_FRMCTR1			0xb1	// normal mode
#define ST7735S_FRMCTR2			0xb2	// in idle mode
#define ST7735S_FRMCTR3			0xb3	//in partial mode
//===================POWER CONTROL=================
#define ST7735S_PWCTR1			0xc0	// power control 1
#define ST7735S_PWCTR2			0xc1	// power control 2
#define ST7735S_PWCTR3			0xc2	// power control 3 (normal mode)
#define ST7735S_PWCTR4			0xc3	// power control 4 (idle mode)
#define ST7735S_PWCTR5			0xc4	// power control 5 (partial mode)
//======================================================
#define ST7735S_INVCTR			0xb4	// display invertion control
#define ST7735S_VMCTR1			0xc5	//VCOM Control 1
#define ST7735S_GAMCTRP1		0xe0	//gamma adjustment (+ Polarity)
#define ST7735S_GAMCTRN1		0xe1	//gamma adjustment (- Polarity)

#define LCD_WIDTH	160
#define LCD_HEIGHT	128

//====================DEFINICJA PRZYKŁADOWYCH KOLORÓW=============
typedef enum{
	BLACK = 0x0000,
	RED = 0x00f8,
	GREEN = 0xE007,
	BLUE = 0x1F00,
	YELLOW = 0xE0FF,
	MAGENTA = 0x1FF8,
	CYAN = 0xFF07,
	WHITE = 0xFFFF
}Color_t;

typedef struct {
    const char *name;
    Color_t value;
} ColorMap;

static const ColorMap color_map[] = {
    {"RED", RED},
    {"GREEN", GREEN},
    {"BLUE", BLUE},
    {"YELLOW", YELLOW},
    {"MAGENTA", MAGENTA},
    {"CYAN", CYAN},
    {"WHITE", WHITE},
    {"BLACK", BLACK}
};

//=================PRZESUNIĘCIE PONIEWAŻ STEROWNIK 162x132px OBSŁUGUJE===========
#define LCD_OFFSET_X  1
#define LCD_OFFSET_Y  2

//================FUNCKJE OD WYSYŁANIA KOMEND/DANYCH/SPRAWDZANIA=================

void lcdCopy(void);
void lcdClear(void);
void lcdPutPixel(int x, int y, uint16_t color);


//==========================INICJALIZACJA WYŚWIETLACZA============================

void lcdInit(void);


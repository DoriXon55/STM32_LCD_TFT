#pragma once

#include <stdint.h>
#include <stdbool.h>
//=======MACRO POMAGAJĄCE W DETEKCJI CZY JEST TO KOMENDA CZY DANE==========
#define CMD(x)			((x) | 0x100)


//===================KOMENDY DO WYŚWIETLACZA, JEDNOBAJTOWE=================
#define ST7735S_SLPOUT          0x11    // SLEEP OUT
#define ST7735S_DISPOFF         0x28    // DISPLAY OFF
#define ST7735S_DISPON          0x29    // DISPLAY ON
#define ST7735S_CASET           0x2A    // USTAWIA POCZĄTKOWĄ I KOŃCOWĄ KOLUMNĘ RYSOWANEGO OBSZARU
#define ST7735S_RASET           0x2B    // USTAWIA POCZĄTKOWY I KOŃCOWY WIERSZ RYSOWANEGO OBRAZU
#define ST7735S_RAMWR           0x2C    // ROZPOCZYNA PRZESYŁANIE DANYCH DO ZDEFINIOWANEGO OBSZARU
#define ST7735S_MADCTL          0x36    // READ DISPLAY MADCTL
#define ST7735S_COLMOD          0x3A    // READ DISPLAY PIXEL FORMAT
//===================FRAME RATE CONTROL=================
#define ST7735S_FRMCTR1         0xB1    // NORMAL MODE
#define ST7735S_FRMCTR2         0xB2    // IN IDLE MODE
#define ST7735S_FRMCTR3         0xB3    // IN PARTIAL MODE
//===================POWER CONTROL=================
#define ST7735S_PWCTR1          0xC0    // POWER CONTROL 1
#define ST7735S_PWCTR2          0xC1    // POWER CONTROL 2
#define ST7735S_PWCTR3          0xC2    // POWER CONTROL 3 (NORMAL MODE)
#define ST7735S_PWCTR4          0xC3    // POWER CONTROL 4 (IDLE MODE)
#define ST7735S_PWCTR5          0xC4    // POWER CONTROL 5 (PARTIAL MODE)
//======================================================
#define ST7735S_INVCTR          0xB4    // DISPLAY INVERSION CONTROL
#define ST7735S_VMCTR1          0xC5    // VCOM CONTROL 1
#define ST7735S_GAMCTRP1        0xE0    // GAMMA ADJUSTMENT (+ POLARITY)
#define ST7735S_GAMCTRN1        0xE1    // GAMMA ADJUSTMENT (- POLARITY)

#define LCD_WIDTH	160
#define LCD_HEIGHT	128

//====================DEFINICJA PRZYKŁADOWYCH KOLORÓW=============
/*
 * układ pracuje w trybie little-endian, więc najpierw w pamięci jest
 * zapisany młodzszy bajt a następnie starszy. Dlatego wysyłane dane
 * koloru są ustawione odwrotnie.
 * Można użyć funkcji __REV16(), jest to instrukcja z asemblera, ale tutaj
 * przestawilem bajty.
 */

typedef enum{
	BLACK = 0x0000,
	RED = 0x00F8,
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
bool lcdIsBusy();
//==========================INICJALIZACJA WYŚWIETLACZA============================

void lcdInit(void);


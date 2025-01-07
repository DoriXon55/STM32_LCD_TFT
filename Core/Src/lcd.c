/*
 * lcd.c
 *
 *  Created on: Nov 23, 2024
 *      Author: doria
 */



#include "lcd.h"
#include "gpio.h"
#include "usart.h"
#include "main.h"
#include "spi.h"


/************************************************************************
* Zmienne:
* frame_buffer - Bufor ramki przechowujący piksele wyświetlacza
* init_table - Tablica inicjalizacyjna z konfiguracją wyświetlacza
*
* frame_buffer - Bufor o rozmiarze LCD_WIDTH * LCD_HEIGHT * 2 bajtów
* przechowujący aktualny stan wyświetlacza w formacie RGB565
*
* init_table - Zawiera sekwencję komend i danych inicjalizujących wyświetlacz
* Każda komenda jest oznaczona makrem CMD()
************************************************************************/
static uint16_t frame_buffer[LCD_WIDTH * LCD_HEIGHT] = {0};
static const uint16_t init_table[] = {
    CMD(ST7735S_FRMCTR1), 0x01, 0x2c, 0x2d,
    CMD(ST7735S_FRMCTR2), 0x01, 0x2c, 0x2d,
    CMD(ST7735S_FRMCTR3), 0x01, 0x2c, 0x2d, 0x01, 0x2c, 0x2d,
    CMD(ST7735S_INVCTR), 0x07,
    CMD(ST7735S_PWCTR1), 0xa2, 0x02, 0x84,
    CMD(ST7735S_PWCTR2), 0xc5,
    CMD(ST7735S_PWCTR3), 0x0a, 0x00,
    CMD(ST7735S_PWCTR4), 0x8a, 0x2a,
    CMD(ST7735S_PWCTR5), 0x8a, 0xee,
    CMD(ST7735S_VMCTR1), 0x0e,
    CMD(ST7735S_GAMCTRP1), 0x0f, 0x1a, 0x0f, 0x18, 0x2f, 0x28, 0x20, 0x22,
    0x1f, 0x1b, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,
    CMD(ST7735S_GAMCTRN1), 0x0f, 0x1b, 0x0f, 0x17, 0x33, 0x2c, 0x29, 0x2e,
    0x30, 0x30, 0x39, 0x3f, 0x00, 0x07, 0x03, 0x10,
    CMD(0xf0), 0x01,
    CMD(0xf6), 0x00,
    CMD(ST7735S_COLMOD), 0x05,
    CMD(ST7735S_MADCTL), 0xa0,
};





/************************************************************************
* Funkcja: lcdCmd()
*
* Cel: Wysyła komendę do wyświetlacza
* Parametry:
*   - cmd: 8-bitowa komenda do wysłania
* Korzysta z:
*   - HAL_GPIO_WritePin: ustawienie pinów CS i DC
*   - HAL_SPI_Transmit: transmisja przez SPI
************************************************************************/
static void lcdCmd(uint8_t cmd)
{
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

/************************************************************************
* Funkcja: lcdData()
*
* Cel: Wysyła dane do wyświetlacza
* Parametry:
*   - data: 8-bitowa wartość do wysłania
* Korzysta z:
*   - HAL_GPIO_WritePin: ustawienie pinów CS i DC
*   - HAL_SPI_Transmit: transmisja przez SPI
************************************************************************/
static void lcdData(uint8_t data)
{
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

/************************************************************************
* Funkcja: lcdSend()
*
* Cel: Wysyła komendę lub dane w zależności od bitu 0x100
* Parametry:
*   - value: 16-bitowa wartość (bit 0x100 określa czy to komenda)
* Korzysta z:
*   - lcd_cmd: wysyłanie komendy
*   - lcd_data: wysyłanie danych
************************************************************************/
static void lcdSend(uint16_t value)
{
	if (value & 0x100) {
		lcdCmd(value);
	} else {
		lcdData(value);
	}
}

/************************************************************************
* Funkcja: lcdData16()
*
* Cel: Wysyła 16-bitową wartość jako dwa bajty
* Parametry:
*   - value: 16-bitowa wartość do wysłania
* Korzysta z:
*   - lcd_data: wysyłanie pojedynczych bajtów
************************************************************************/
static void lcdData16(uint16_t value)
{
	lcdData(value >> 8);
	lcdData(value);
}

/************************************************************************
* Funkcja: lcdSetWindow()
*
* Cel: Ustawia okno do zapisu na wyświetlaczu
* Parammetry:
*   - x, y: Współrzędne początkowe okna
*   - width, height: Szerokość i wysokość okna
* Korzysta z:
*   - lcd_cmd: wysyłanie komend CASET i RASET
*   - lcd_data16: wysyłanie współrzędnych
************************************************************************/
static void lcdSetWindow(int x, int y, int width, int height)
{
  lcdCmd(ST7735S_CASET);
  lcdData16(LCD_OFFSET_X + x);
  lcdData16(LCD_OFFSET_X + x + width - 1);

  lcdCmd(ST7735S_RASET);
  lcdData16(LCD_OFFSET_Y + y);
  lcdData16(LCD_OFFSET_Y + y + height- 1);
}

/************************************************************************
* Funkcja: lcdCopy()
*
* Cel: Kopiuje zawartość bufora ramki do wyświetlacza
*   1. Ustawia okno na cały wyświetlacz
*   2. Rozpoczyna zapis do pamięci RAM wyświetlacza
*   3. Przesyła cały bufor ramki przez SPI
* Korzysta z:
*   - lcd_set_window: ustawienie obszaru zapisu
*   - HAL_SPI_Transmit: przesłanie danych
************************************************************************/
void lcdCopy(void)
{
	lcdSetWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);
	lcdCmd(ST7735S_RAMWR);
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit_DMA(&hspi2, (uint8_t*)frame_buffer, sizeof(frame_buffer));
}
/************************************************************************
* Funkcja: lcdClear()
*
* Cel: Czyści wyświetlacz (ustawia wszystkie piksele na czarny)
*   1. Zeruje bufor ramki
*   2. Przesyła wyzerowany bufor do wyświetlacza
* Korzysta z:
*   - lcd_set_window: ustawienie obszaru zapisu
*   - HAL_SPI_Transmit: przesłanie danych
************************************************************************/
void lcdClear(void) {
	lcdSetWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);
    lcdCmd(ST7735S_RAMWR);
    HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        frame_buffer[i] = 0x0000; // Czarny kolor
    }
    HAL_SPI_Transmit(&hspi2, (uint8_t*)frame_buffer, sizeof(frame_buffer), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}


/************************************************************************
* Funkcja: lcdInit()
*
* Cel: Inicjalizuje wyświetlacz
*   1. Wykonuje reset sprzętowy wyświetlacza
*   2. Wysyła sekwencję inicjalizacyjną z init_table
*   3. Wyłącza tryb uśpienia
*   4. Włącza wyświetlacz i podświetlenie
* Korzysta z:
*   - lcd_send: wysyłanie komend inicjalizacyjnych
*   - HAL_GPIO_WritePin: sterowanie pinami RST i BL
************************************************************************/
void lcdInit(void) {
    int i;
    HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
    delay(100);
    HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
    delay(100);
    for (i = 0; i < sizeof(init_table) / sizeof(uint16_t); i++) {
        lcdSend(init_table[i]);
    }
    delay(200);
    lcdCmd(ST7735S_SLPOUT);
    delay(120);
    lcdCmd(ST7735S_DISPON);
    HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_SET);
}


/************************************************************************
* Funkcja: lcdPutPixel()
*
* Cel: Ustawia kolor pojedynczego piksela w buforze ramki
*   Zapisuje kolor do bufora ramki. Zmiana będzie widoczna
*   dopiero po wywołaniu lcd_copy()
* Parametry:
*   - x, y: Współrzędne piksela
*   - color: Kolor w formacie RGB565
************************************************************************/
void lcdPutPixel(int x, int y, uint16_t color)
{
	frame_buffer[x + y * LCD_WIDTH] = color;
}



void lcdTransferDone(void)
{
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}
bool lcdIsBusy(void)
{
	if(HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_BUSY)
	{
		return true;
	} else {
		return false;
	}
}

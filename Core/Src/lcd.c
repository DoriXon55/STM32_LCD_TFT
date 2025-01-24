/*
 * lcd.c
 *
 *  Created on: Nov 23, 2024
 *      Author: doria
 */


//============================INCLUDES====================================
#include "lcd.h"
#include "gpio.h"
#include "usart.h"
#include "main.h"
#include "spi.h"
#include "frame.h"
#include <string.h>
//========================================================================



//=========================ZMIENNE POMOCNICZE=============================
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
uint16_t frameBuffer[LCD_WIDTH * LCD_HEIGHT] = {0};
static volatile bool transferInProgress = false;
//========================================================================


//================================STATIC===================================
static void lcdCmd(uint8_t cmd)
{
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

static void lcdData(uint8_t data)
{
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

static void lcdSend(uint16_t value)
{
	if (value & 0x100) {
		lcdCmd(value);
	} else {
		lcdData(value);
	}
}

static void lcdData16(uint16_t value)
{
	lcdData(value >> 8);
	lcdData(value);
}

static void lcdSetWindow(int x, int y, int width, int height)
{
  lcdCmd(ST7735S_CASET);
  lcdData16(LCD_OFFSET_X + x);
  lcdData16(LCD_OFFSET_X + x + width - 1);

  lcdCmd(ST7735S_RASET);
  lcdData16(LCD_OFFSET_Y + y);
  lcdData16(LCD_OFFSET_Y + y + height- 1);
}
static bool lcdIsBusy(void) {
    return transferInProgress;
}
//=======================================================================

void lcdCopy(void)
{
    if (lcdIsBusy()) return;
    lcdSetWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);
    lcdCmd(ST7735S_RAMWR);
    HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit_DMA(&hspi2, (uint8_t*)frameBuffer, sizeof(frameBuffer));
}

void lcdClear(void)
{
    memset(frameBuffer, 0, sizeof(frameBuffer));
}

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
    memset(frameBuffer, 0, sizeof(frameBuffer));
}

void lcdPutPixel(int x, int y, uint16_t color)
{
        frameBuffer[y * LCD_WIDTH + x] = color;
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &hspi2) {
        HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
        transferInProgress = false;
    }
}

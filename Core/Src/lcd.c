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
static uint16_t frame_buffer[LCD_WIDTH * LCD_HEIGHT];
static const uint16_t init_table[] =
{
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

static void lcd_cmd(uint8_t cmd)
{
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}
static void lcd_data(uint8_t data)
{
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}
static void lcd_send(uint16_t value)
{
	if (value & 0x100) {
		lcd_cmd(value);
	} else {
		lcd_data(value);
	}
}
static void lcd_data16(uint16_t value)
{
	lcd_data(value >> 8);
	lcd_data(value);
}
static void lcd_set_window(int x, int y, int width, int height)
{
  lcd_cmd(ST7735S_CASET);
  lcd_data16(LCD_OFFSET_X + x);
  lcd_data16(LCD_OFFSET_X + x + width - 1);

  lcd_cmd(ST7735S_RASET);
  lcd_data16(LCD_OFFSET_Y + y);
  lcd_data16(LCD_OFFSET_Y + y + height- 1);
}
void lcd_copy(void)
{
	lcd_set_window(0, 0, LCD_WIDTH, LCD_HEIGHT);
	lcd_cmd(ST7735S_RAMWR);
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, (uint8_t*)frame_buffer, sizeof(frame_buffer), HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

//TODO zrobic zarządzanie podświetleniem
void lcd_init(void)
{
	  int i;

	  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
	  delay(100);
	  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
	  delay(100);
	  for (i = 0; i < sizeof(init_table) / sizeof(uint16_t); i++) {
	    lcd_send(init_table[i]);
	  }
	  delay(200);

	  lcd_cmd(ST7735S_SLPOUT);
	  delay(120);
	  lcd_cmd(ST7735S_DISPON);
}

void lcd_fill_box(int x, int y, int width, int height, uint16_t color)
{
	lcd_set_window(x, y, width, height);

	lcd_cmd(ST7735S_RAMWR);
	for (int i = 0; i < width * height; i++)
		lcd_data16(color);
}
void lcd_put_pixel(int x, int y, uint16_t color)
{
	frame_buffer[x + y * LCD_WIDTH] = color;
}

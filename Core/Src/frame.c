/*
 * frame.c
 *
 *  Created on: Nov 6, 2024
 *      Author: doria
 */
//=========================INCLUDES=====================================
#include "frame.h"
#include "crc.h"
#include "USART_ringbuffer.h"
#include "usart.h"
#include "lcd.h"
#include "font5x7.h"
#include "font5x8.h"
#include "font6x9.h"
#include "hagl.h"
#include "rgb565.h"
#include <ctype.h>
#include <math.h>
#include "crc.h"
//======================================================================

//=========================ZMIENNE DO RAMKI=============================
uint8_t bx[MAX_FRAME_LEN];
bool escapeDetected = false;
int bxIndex = 0;
bool inFrame = false;
uint8_t receivedChar;
Frame frame;
ScrollingTextState text = { 0 };
//======================================================================

//======================FUNKCJE POMOCNICZE==============================
static void resetFrameState() {
	inFrame = false;
	escapeDetected = false;
	bxIndex = 0;
}

static void stopAnimation(void) {
	text.isScrolling = false;
}

static bool isWithinBounds(int x, int y) {
	return (x >= 0 && x < LCD_WIDTH) && (y >= 0 && y < LCD_HEIGHT);
}

static bool safeCompare(const char *str1, const char *str2, size_t len) {
	if (str1 == NULL || str2 == NULL) {
		return false;
	}
	return memcmp(str1, str2, len) == 0;
}

static bool isValidTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
		int16_t x3, int16_t y3) {
	int32_t a2 = (int32_t) (x2 - x1) * (x2 - x1)
			+ (int32_t) (y2 - y1) * (y2 - y1);
	int32_t b2 = (int32_t) (x3 - x2) * (x3 - x2)
			+ (int32_t) (y3 - y2) * (y3 - y2);
	int32_t c2 = (int32_t) (x1 - x3) * (x1 - x3)
			+ (int32_t) (y1 - y3) * (y1 - y3);

	if (a2 + b2 <= c2 || b2 + c2 <= a2 || c2 + a2 <= b2) {
		return false;
	}

	int32_t cross = (int32_t) (x2 - x1) * (y3 - y1)
			- (int32_t) (y2 - y1) * (x3 - x1);
	if (cross == 0) {
		return false;
	}

	return true;
}

static void sendStatus(StatusCode_t status) {
	if (status < STATUS_COUNT) {
		prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "%s", STATUS_MESSAGES[status]);
	}
}

static void clearFrame(Frame *frame) {
	if (frame) {
		memset(frame->data, 0, sizeof(frame->data));
		memset(frame->command, 0, sizeof(frame->command));
	}
}

static bool parseCoordinates(const uint8_t *data, int *x, int *y) {
	*x = data[0];  // Pierwszy bajt to x
	*y = data[2];  // Drugi bajt to y
	return true;
}

static bool parseParameters(const uint8_t *data, const char *format, ...) {
	if (!data || !format) {
		return false;
	}
	va_list args;
	va_start(args, format);

	const uint8_t *data_ptr = data;
	const char *fmt_ptr = format;

	int u_param_count = 0;
	uint8_t scrollSpeed = 0;

	while (*fmt_ptr) {
		switch (*fmt_ptr) {
		case 'u': {
			uint8_t *value_ptr = va_arg(args, uint8_t*);
			*value_ptr = *data_ptr++;
			u_param_count++;
			if (u_param_count == 4) {
				scrollSpeed = *value_ptr;
			}
			if (*data_ptr == ',') {
				data_ptr++;
			}
			break;
		}
		case 'C': {
			color_t *color_ptr = va_arg(args, color_t*);

			if ((*data_ptr >= 'A' && *data_ptr <= 'Z')
					|| (*data_ptr >= 'a' && *data_ptr <= 'z')) {
				return false;
			}

			uint8_t lsb = *data_ptr++;
			if (*data_ptr == ',')
				data_ptr++;

			if ((*data_ptr >= 'A' && *data_ptr <= 'Z')
					|| (*data_ptr >= 'a' && *data_ptr <= 'z')) {
				return false;
			}

			uint8_t msb = *data_ptr++;
			if (*data_ptr == ',')
				data_ptr++;
			*color_ptr = ((uint16_t) msb << 8) | lsb; //użycie funkcji asemblera __REV16()
			break;
		}
		case 't': {
			size_t realCharCount = 0;
			size_t maxChars = (scrollSpeed == 0) ? 25 : 50;
			wchar_t *text_ptr = va_arg(args, wchar_t*);

			while (*data_ptr && realCharCount < maxChars) {
				if ((*data_ptr & 0x80) == 0) {
					text_ptr[realCharCount++] = (wchar_t) *data_ptr++;
				} else if ((*data_ptr & 0xE0) == 0xC0) {
					if (!data_ptr[1])
						break;
					wchar_t wc = ((data_ptr[0] & 0x1F) << 6)
							| (data_ptr[1] & 0x3F);
					text_ptr[realCharCount++] = wc;
					data_ptr += 2;
				} else {
					data_ptr++;
				}
			}

			if (realCharCount > maxChars) {
				va_end(args);
				sendStatus(ERR_TOO_MUCH_TEXT);
				return false;
			}

			text_ptr[realCharCount] = L'\0';
			break;
		}
		default:
			va_end(args);
			return false;
		}
		fmt_ptr++;
	}

	va_end(args);
	return true;
}

static bool decodeFrame(uint8_t *bx, Frame *frame, uint8_t len) {
	uint8_t ownCrc[2];
	uint8_t incCrc[2];

	if (len >= MIN_DECODED_FRAME_LEN && len <= MAX_FRAME_WITHOUT_STUFFING) {
		uint8_t k = 0;
		frame->sender = bx[k++];
		frame->receiver = bx[k++];
		if (frame->receiver != STM32_ADDR) {
			sendStatus(ERR_WRONG_RECEIVER);
			return false;
		}

		memcpy(frame->command, &bx[k], COMMAND_LENGTH);
		k += COMMAND_LENGTH;

		uint8_t dataLen = len - MIN_DECODED_FRAME_LEN; //to sprawdzic
		memcpy(frame->data, &bx[k], dataLen);
		k += dataLen;

		memcpy(incCrc, &bx[k], 2);
		calculateCrc16((uint8_t*) frame, k, ownCrc);
		if (ownCrc[0] != incCrc[0] || ownCrc[1] != incCrc[1]) {
			sendStatus(ERR_WRONG_CRC);
			return false;
		}
		return true;
	}
	return false;
}

static size_t byteStuffing(uint8_t *input, size_t input_len, uint8_t *output) {
	size_t j = 0;
	for (size_t i = 0; i < input_len; i++) {
		if (input[i] == ESCAPE_CHAR) {
			output[j++] = ESCAPE_CHAR;
			output[j++] = ESCAPE_CHAR_STUFF;
		} else if (input[i] == FRAME_START) {
			output[j++] = ESCAPE_CHAR;
			output[j++] = FRAME_START_STUFF;
		} else if (input[i] == FRAME_END) {
			output[j++] = ESCAPE_CHAR;
			output[j++] = FRAME_END_STUFF;
		} else {
			output[j++] = input[i];
		}
	}
	return j;
}

//======================================================================

//==================FUNCKJE DLA WYŚWIETLACZA===================================
static void executeONK(Frame *frame) {
	uint8_t x = 0, y = 0, r = 0, filling = 0;
	color_t color = 0;
	if (!parseParameters(frame->data, "uuuuC", &x, &y, &r, &filling, &color)) {
		sendStatus(ERR_NOT_RECOGNIZED);
		return;
	}
	switch (filling) {
	case 0:
		hagl_draw_circle(x, y, r, color);
		break;
	case 1:
		hagl_fill_circle(x, y, r, color);
		break;
	}
}

static void executeONP(Frame *frame) {
	uint8_t x = 0, y = 0, width = 0, height = 0, filling = 0;
	color_t color = 0;
	if (!parseParameters(frame->data, "uuuuuC", &x, &y, &width, &height,
			&filling, &color)) {
		sendStatus(ERR_NOT_RECOGNIZED);
		return;
	}

	switch (filling) {
	case 0:
		hagl_draw_rectangle(x, y, width, height, color);
		break;
	case 1:
		hagl_fill_rectangle(x, y, width, height, color);
		break;
	}
}

static void executeONT(Frame *frame) {
	uint8_t x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0, filling = 0;
	color_t color = 0;
	if (!parseParameters(frame->data, "uuuuuuuC", &x1, &y1, &x2, &y2, &x3, &y3,
			&filling, &color)) {
		sendStatus(ERR_NOT_RECOGNIZED);
		return;
	}
	if (!isValidTriangle(x1, y1, x2, y2, x3, y3)) {
		sendStatus(ERR_INVALID_TRIANGLE);
		return;
	}
	switch (filling) {
	case 0:
		hagl_draw_triangle(x1, y1, x2, y2, x3, y3, color);
		break;
	case 1:
		hagl_fill_triangle(x1, y1, x2, y2, x3, y3, color);
		break;
	}
}

static void executeONN(Frame *frame) {
	if (!parseParameters(frame->data, "uuuuCt", &text.x, &text.y,
			&text.fontSize, &text.scrollSpeed, &text.color, text.displayText)) {
		sendStatus(ERR_NOT_RECOGNIZED);
		return;
	}

	text.startX = text.x;
	text.startY = text.y;
	text.textLength = 0;
	while (text.displayText[text.textLength] != L'\0') {
		text.textLength++;
	}
	text.firstIteration = true;

	text.isScrolling = (text.scrollSpeed > 0);
	text.lastUpdate = HAL_GetTick();

	const uint8_t *font;
	switch (text.fontSize) {
	case 1:
		font = font5x7;
		break;
	case 2:
		font = font5x8;
		break;
	case 3:
		font = font6x9;
		break;
	default:
		font = font5x7;
	}

	if (!text.scrollSpeed) {
		hagl_put_text((wchar_t*) text.displayText, text.x, text.y, text.color,
				font);
	}
}

static void executeOFF(Frame *frame) {

	switch (frame->data[0]) {
	case 0:
		HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_RESET);
		break;
	case 1:
		lcdClear();
		break;
	default:
		sendStatus(ERR_WRONG_OFF_DATA);
	}
}

//=====================================================================

void prepareFrame(uint8_t sender, uint8_t receiver, const char *command,
		const char *format, ...) {
	Frame frame = { 0 };
	frame.sender = sender;
	frame.receiver = receiver;
	strncpy((char*) frame.command, command, COMMAND_LENGTH);

	uint8_t formatted_data[MAX_DATA_SIZE] = { 0 };  // Zainicjalizowane zerami
	va_list args;
	va_start(args, format);
	int data_len = vsnprintf((char*) formatted_data, MAX_DATA_SIZE, format,
			args);
	va_end(args);

	uint8_t raw_payload[MAX_FRAME_WITHOUT_STUFFING] = { 0 };
	size_t raw_payload_len = 0;

	raw_payload[raw_payload_len++] = frame.sender;
	raw_payload[raw_payload_len++] = frame.receiver;
	memcpy(raw_payload + raw_payload_len, frame.command, COMMAND_LENGTH);
	raw_payload_len += COMMAND_LENGTH;

	if (data_len > 0) {
		memcpy(raw_payload + raw_payload_len, formatted_data, data_len);
		raw_payload_len += data_len;
	}

	uint8_t crc_output[2];
	calculateCrc16(raw_payload, raw_payload_len, crc_output);
	raw_payload[raw_payload_len++] = crc_output[0];
	raw_payload[raw_payload_len++] = crc_output[1];

	uint8_t stuffed_payload[MAX_FRAME_LEN];
	size_t stuffed_len = byteStuffing(raw_payload, raw_payload_len,
			stuffed_payload);

	uint8_t final_frame[MAX_FRAME_LEN + 2];
	size_t final_len = 0;

	final_frame[final_len++] = FRAME_START;
	memcpy(final_frame + final_len, stuffed_payload, stuffed_len);
	final_len += stuffed_len;
	final_frame[final_len++] = FRAME_END;

	USART_sendFrame(final_frame, final_len);
}

void processReceivedChar(uint8_t receivedChar) {
	if (receivedChar == FRAME_START) {
		resetFrameState();
		inFrame = true;
	} else if (receivedChar == FRAME_END && escapeDetected == false) {
		if (inFrame) {
			if (decodeFrame(bx, &frame, bxIndex)) {
				stopAnimation();
				handleCommand(&frame);
			}
			resetFrameState();
			return;
		}
	} else if (inFrame) {
			if (escapeDetected) {
				if (receivedChar == FRAME_START_STUFF) {
					bx[bxIndex++] = FRAME_START;
				} else if (receivedChar == ESCAPE_CHAR_STUFF) {
					bx[bxIndex++] = ESCAPE_CHAR;
				} else if (receivedChar == FRAME_END_STUFF) {
					bx[bxIndex++] = FRAME_END;
				} else {
					resetFrameState();
				}
				escapeDetected = false;
			} else if (receivedChar == ESCAPE_CHAR) {
				escapeDetected = true;
			} else {
				bx[bxIndex++] = receivedChar;
			}
			if(bxIndex > MAX_FRAME_WITHOUT_STUFFING)
			{
				resetFrameState();
			}
	}
}

void handleCommand(Frame *frame) {
	if (frame == NULL) {
		return;
	}
	CommandEntry commandTable[COMMAND_COUNT] = {
			{ "ONK", executeONK },
			{ "ONP", executeONP },
			{ "ONT", executeONT },
			{ "ONN", executeONN },
			{ "OFF", executeOFF } };
	HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_SET);
	for (int i = 0; i < COMMAND_COUNT; i++) {
		if (safeCompare(frame->command, commandTable[i].command,
				COMMAND_LENGTH)) {

			sendStatus(ERR_GOOD);

			if (safeCompare(commandTable[i].command, "OFF", COMMAND_LENGTH)) {
				commandTable[i].function(frame);
				lcdCopy();
				clearFrame(frame);
				return;
			}

			int x, y;
			if (parseCoordinates(frame->data, &x, &y)) {
				if (isWithinBounds(x, y)) {
					lcdClear();
					commandTable[i].function(frame);
					lcdCopy();
					clearFrame(frame);
					return;
				} else {
					sendStatus(ERR_DISPLAY_AREA);
					return;
				}
			} else {
				lcdClear();
				lcdCopy();
				sendStatus(ERR_NOT_RECOGNIZED);
				return;
			}
		}
	}
}

void updateScrollingText(void) {
	if (!text.isScrolling || text.scrollSpeed == 0) {
		return;
	}

	uint32_t currentTime = HAL_GetTick();
	if ((currentTime - text.lastUpdate) >= (text.scrollSpeed >> 1)) {
		text.lastUpdate = currentTime;

		uint8_t charWidth;
		uint8_t charHeight;
		const uint8_t *font;
		switch (text.fontSize) {
		case 1:
			charWidth = 5;
			charHeight = 7;
			font = font5x7;
			break;
		case 2:
			charWidth = 5;
			charHeight = 8;
			font = font5x8;
			break;
		case 3:
			charWidth = 6;
			charHeight = 9;
			font = font6x9;
			break;
		default:
			charWidth = 5;
			charHeight = 7;
			font = font5x7;
			break;
		}

		int16_t textWidth = text.textLength * charWidth;

		if (!text.firstIteration) {
		    text.x += text.textLength;


		    if (text.x > LCD_WIDTH) {
		        text.x = -textWidth;
		        text.y += charHeight;


		        if (text.y >= LCD_HEIGHT - charHeight) {
		            text.y = text.startY;
		        }
		    }
		} else {

		    text.x = -textWidth;
		    text.y = text.startY;
		    text.firstIteration = false;
		}

		lcdClear();
		hagl_put_text(text.displayText, text.x, text.y, text.color, font);
		lcdCopy();
	}
}

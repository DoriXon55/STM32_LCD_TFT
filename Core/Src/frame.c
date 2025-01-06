/*
 * frame.c
 *
 *  Created on: Nov 6, 2024
 *      Author: doria
 */
//=========================INCLUDES=============================
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
#include "crc.h"

//=========================ZMIENNE DO RAMKI=============================
extern uint8_t USART_RxBuf[];
extern uint8_t USART_TxBuf[];
extern volatile int USART_TX_Empty;
extern volatile int USART_TX_Busy;
extern volatile int USART_RX_Empty;
extern volatile int USART_RX_Busy;
uint8_t bx[270];
bool escape_detected = false;
int bx_index = 0;
bool in_frame = false;
uint8_t received_char;
Receive_Frame ramka;

//=========================FUNKCJE POMOCNICZE=============================
//TODO do sprawdzenia
bool parse_color(const char* color_name, Color_t* color) {
    for (int i = 0; i < sizeof(color_map) / sizeof(ColorMap); i++) {
        if (strcmp(color_name, color_map[i].name) == 0) {
            *color = color_map[i].value;
            return true;
        }
    }
    return false;
}

static void reset_frame_state() {
    in_frame = false;
    escape_detected = false;
    bx_index = 0;
}
/*
static void parseCommandSwtich(char command)
{
	switch(command)
	{
	case 'K':
		break;
	case 'P':
		break;
	case 'T':
		break;
	case 'N':
		break;
	case 'F':
		break;
	}
}
*/
bool parse_parameters(const char* data, const char* format, ...) {
    if (!data || !format) return false;

    va_list args;
    va_start(args, format);

    const char* ptr = data;
    int value = 0;
    bool reading_number = false;
    int param_index = 0;
    char color_name[32] = {0};
    int color_index = 0;

    while (*ptr != '\0') {
        if (*ptr >= '0' && *ptr <= '9') {
            value = (value * 10) + (*ptr - '0');
            reading_number = true;
        }
        else if (*ptr == ',' || *(ptr + 1) == '\0') {
            if (reading_number) {
                if (format[param_index] == 'u') {
                    uint8_t* param = va_arg(args, uint8_t*);
                    *param = (uint8_t)value;
                }
                value = 0;
                reading_number = false;
                param_index++;
            }
            else if (color_index > 0) {
                color_name[color_index] = '\0';
                if (format[param_index] == 's') {
                    uint16_t* color_value = va_arg(args, uint16_t*);
                    if (!parse_color(color_name, color_value)) {
                        va_end(args);
                        return false;
                    }
                }
                color_index = 0;
                param_index++;
            }
        }
        else if (*ptr >= 'A' && *ptr <= 'Z') {
            color_name[color_index++] = *ptr;
            if (color_index >= sizeof(color_name) - 1) {
                va_end(args);
                return false;
            }
        }
        ptr++;
    }

    va_end(args);
    return true;
}

//==========================OBSŁUGA KOMEND================================

//TODO nie dzialla
static void executeONK(Receive_Frame *frame)
{
	uint8_t x = 0, y = 0, r = 0, filling = 0;
	Color_t color = 0;
    if (!parse_parameters(frame->data, "uuuus", &x, &y, &r, &filling, &color))
    {
		prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "NOT_RECOGNIZED%s", frame->data);
        return;
    }
	switch(filling)
	{
	case 0:
		hagl_draw_circle(x, y, r, color);
		break;
	case 1:
		hagl_fill_circle(x, y, r, color);
		break;
	}
}


static void executeONP(Receive_Frame *frame)
{
	uint8_t x = 0, y = 0, width = 0, height = 0, filling = 0;
	Color_t color = 0;
	if (!parse_parameters(frame->data, "uuuuus", &x, &y, &width, &height, &filling, &color)) {
		prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "NOT_RECOGNIZED%s", frame->data);
		return;
	}

	switch(filling)
	{
	case 0:
		hagl_draw_rectangle(x, y, width, height, color);
		break;
	case 1:
		hagl_fill_rectangle(x, y, width, height, color);
		break;
	}
}


//TODO nie dziala
static void executeONT(Receive_Frame *frame)
{
    uint8_t x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0, filling = 0;
    Color_t color = 0;
    if (!parse_parameters(frame->data, "uuuuuuus", &x1, &y1, &x2, &y2, &x3, &y3, &filling, &color))
    {
		prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "NOT_RECOGNIZED%s", frame->data);
        return;
    }
    switch(filling)
    {
        case 0:
            hagl_draw_triangle(x1, y1, x2, y2, x3, y3, color);
            break;
        case 1:
            hagl_fill_triangle(x1, y1, x2, y2, x3, y3, color);
            break;
    }
}

//TODO nie dziala, dodac obsluge przewijania tekstu
static void executeONN(Receive_Frame *frame)
{
    char text[50] ={0};
    uint8_t x = 0, y = 0, fontSize = 0;
    uint16_t color = 0;
    if (!parse_parameters(frame->data, "uuust", &x, &y, &fontSize, &color, text)) {
		prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "NOT_RECOGNIZED%s", frame->data);
        return;
    }

    switch(fontSize)
    {
        case 1:
            hagl_put_text((wchar_t*)text, x, y, color, font5x7); //fontSize zmien
            break;
        case 2:
            hagl_put_text((wchar_t*)text, x, y, color, font5x8); //fontSize zmien
            break;
        case 3:
            hagl_put_text((wchar_t*)text, x, x, color, font6x9); //fontSize zmien
            break;
    }
}



//TODO bład parsowania danych
static void executeOFF(Receive_Frame *frame)
{
	uint8_t state = 0;
	if (!parse_parameters(frame->data, "u", &state)) {
		prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "NOT_RECOGNIZED%s", frame->data);
		return;
	}
	switch(state)
	{
	case 0:
		HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_SET);
		break;
	case 1:
		hagl_fill_rectangle(0,0, LCD_WIDTH, LCD_HEIGHT, WHITE);
		break;
	}
}


//=======================SPRAWDZANIE KOORDYNATÓW=========================
bool is_within_bounds(int x, int y)
{
	return (x >= 0 && x < LCD_WIDTH)&&(y >= 0 && y < LCD_HEIGHT);
}
bool parse_coordinates(const char *data, int *x, int *y)
{
	char *token;
	    char data_copy[MAX_DATA_SIZE];
	    strncpy(data_copy, data, MAX_DATA_SIZE); // Kopiujemy dane wejściowe, bo strtok modyfikuje ciąg

	    token = strtok(data_copy, ","); // Pierwsza współrzędna (jest to funkcja służąca do oddzielania stringów z separatorem)
	    if (token == NULL) {
	        return false;
	    }
	    *x = atoi(token);

	    token = strtok(NULL, ","); // Druga współrzędna
	    if (token == NULL) {
	        return false;
	    }
	    *y = atoi(token);

	    return true;
}
//=======================BYTE STUFFING=========================
size_t byteStuffing(uint8_t *input, size_t input_len, uint8_t *output) {
    size_t j = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == ESCAPE_CHAR) {
            output[j++] = ESCAPE_CHAR;
            output[j++] = ']';
        } else if (input[i] == '~') {
            output[j++] = ESCAPE_CHAR;
            output[j++] = '^';
        } else if (input[i] == '`') {
            output[j++] = ESCAPE_CHAR;
            output[j++] = '&';
        } else {
            output[j++] = input[i];
        }
    }
    return j;
}

//=======================OBSŁUGA RAMKI ZWROTNEJ=========================
void prepareFrame(uint8_t sender, uint8_t receiver, const char *command, const char *format, ...) {
    Frame frame = {0};
    frame.sender = sender;
    frame.receiver = receiver;
    strncpy((char *)frame.command, command, COMMAND_LENGTH);

    // Formatowanie danych
    va_list args;
    va_start(args, format);
    vsnprintf((char *)frame.data, MAX_DATA_SIZE, format, args);
    va_end(args);

    // Oblicz długość danych
    size_t data_len = strlen((const char *)frame.data);

    // Przygotowanie danych do obliczenia CRC
    size_t crc_input_len = 2 + COMMAND_LENGTH + data_len;
    uint8_t crc_input[crc_input_len];
    crc_input[0] = frame.sender;
    crc_input[1] = frame.receiver;
    memcpy(crc_input + 2, frame.command, COMMAND_LENGTH);
    memcpy(crc_input + 2 + COMMAND_LENGTH, frame.data, data_len);

    // Obliczanie CRC
    char crc_output[2]; // Tablica na wynik CRC
    calculate_crc16(crc_input, crc_input_len, crc_output);

    // Konwersja CRC na heksadecymalne ciągi znaków
    char crc_hex[5]; // 4 znaki na heksadecymalną reprezentację + 1 na znak null
    snprintf(crc_hex, sizeof(crc_hex), "%02X%02X", (uint8_t)crc_output[0], (uint8_t)crc_output[1]);

    // Przygotowanie do byte-stuffingu
    uint8_t raw_payload[2 + COMMAND_LENGTH + data_len + 4]; // 4 dodatkowe bajty na heksadecymalne CRC
    raw_payload[0] = frame.sender;
    raw_payload[1] = frame.receiver;
    memcpy(raw_payload + 2, frame.command, COMMAND_LENGTH);
    memcpy(raw_payload + 2 + COMMAND_LENGTH, frame.data, data_len);
    memcpy(raw_payload + 2 + COMMAND_LENGTH + data_len, crc_hex, 4); // Dodanie heksadecymalnego CRC

    uint8_t stuffed_payload[512];
    size_t stuffed_len = byteStuffing(raw_payload, 2 + COMMAND_LENGTH + data_len + 4, stuffed_payload);

    // Wysyłanie ramki
    USART_fsend("%c", FRAME_START); // Wyślij początek ramki
    for (size_t i = 0; i < stuffed_len; i++) {
        USART_fsend("%c", stuffed_payload[i]); // Wyślij dane po byte-stuffingu
        delay(10);
    }
    USART_fsend("%c", FRAME_END); // Wyślij koniec ramki
    USART_fsend("\r\n");

}

//=======================DEKODOWANIE RAMKI=========================
bool decodeFrame(uint8_t *bx, Receive_Frame *frame, uint8_t len) {
    char ownCrc[2];
    char incCrc[2];
        if(len >= MIN_DECODED_FRAME_LEN && len <= MAX_FRAME_LEN) {
            uint8_t k = 0;
            frame->receiver = bx[k++];
            frame->sender = bx[k++];
            memcpy(frame->command, &bx[k],COMMAND_LENGTH);
            k += COMMAND_LENGTH;
            uint8_t data_len = len - MIN_DECODED_FRAME_LEN;
            memcpy(frame->data, &bx[k],data_len);
            k += data_len;
            memcpy(incCrc, &bx[k], 2);
            calculate_crc16((uint8_t *)frame, k, ownCrc);
            if(ownCrc[0] != incCrc[0] || ownCrc[1] != incCrc[1]) {
            	return false;
            }
            return true;
        }
        return false;
}



//=======================DETEKCJA RAMKI=========================
void process_received_char(uint8_t received_char) {
    if (received_char == '~') {
        if (!in_frame) {
            in_frame = true;
            bx_index = 0;
            escape_detected = false;
        } else {
            reset_frame_state();
        }
    } else if (received_char == '`') {
        if (in_frame) {
            if (decodeFrame(bx, &ramka, bx_index)) {
                prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "GOOD");
                handleCommand(&ramka);
            } else {
                prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "FAIL");
            }
            reset_frame_state();
        } else {
            prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "FAIL");
            reset_frame_state();
        }
    } else if (in_frame) {
        if (escape_detected) {
            if (received_char == '^') {
                bx[bx_index++] = '~';
            } else if (received_char == ']') {
                bx[bx_index++] = '}';
            } else if (received_char == '&') {
                bx[bx_index++] = '`';
            } else {
                prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "FAIL");
                reset_frame_state();
            }
            escape_detected = false;
        } else if (received_char == '}') {
            escape_detected = true;
        } else {
            if (bx_index < sizeof(bx)) {
                bx[bx_index++] = received_char;
            } else {
                reset_frame_state();
            }
        }
    } else {
        reset_frame_state();
    }
}





//=======================ROZPOZNANIE I WYKONANIE KOMENDY=========================
void handleCommand(Receive_Frame *frame)
{
	CommandEntry commandTable[COMMAND_COUNT] = {
			{"ONK", executeONK},
			{"ONP", executeONP},
			{"ONT", executeONT},
			{"ONN", executeONN},
			{"OFF", executeOFF}
	};
	for (int i = 0; i < COMMAND_COUNT; i++) {
	        if (strncmp(frame->command, commandTable[i].command, COMMAND_LENGTH) == 0) {
	            // Parsowanie współrzędnych z `data`
	            int x, y;
	            if (parse_coordinates(frame->data, &x, &y)) {
	                // Sprawdzenie zakresu współrzędnych
	                if (is_within_bounds(x, y)) {
	                    lcd_clear();
	                    commandTable[i].function(frame); // Wywołaj przypisaną funkcję
	                    lcd_copy();
	                    return;
	                } else {
	                    prepareFrame(STM32_ADDR, PC_ADDR, "BCK", " DISPLAY_AREA");
	                    return;
	                }
	            } else {
	                prepareFrame(STM32_ADDR, PC_ADDR, "BCK", " NOT_RECOGNIZED%s", frame->data);
	                return;
	            }
	        }
	    }
	    prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "NOT_RECOGNIZED%s", frame->command);
}


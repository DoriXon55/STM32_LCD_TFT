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
#include <ctype.h>
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

bool parse_parameters(const char* data, const char* format, ...) {
    if (data == NULL || format == NULL) {
        return false;
    }

    // Debug input data
    USART_fsend("Received data: %s\r\n", data);
    USART_fsend("Format string: %s\r\n", format);

    va_list args;
    va_start(args, format);

    // Alokuj pamięć z dodatkowym marginesem bezpieczeństwa
    size_t data_len = strlen(data) + 1;
    char* data_copy = (char*)calloc(data_len + 1, sizeof(char)); // Użyj calloc zamiast malloc
    if (data_copy == NULL) {
        va_end(args);
        return false;
    }

    // Kopiuj dane z ograniczeniem długości
    strncpy(data_copy, data, data_len);
    data_copy[data_len] = '\0'; // Upewnij się, że string jest zakończony

    char* token = strtok(data_copy, ",");
    const char* fmt_ptr = format;
    int param_count = 0;

    while (*fmt_ptr != '\0' && token != NULL) {
        // Oczyść token ze zbędnych znaków
        char cleaned_token[50] = {0}; // Bufor na wyczyszczony token
        size_t clean_idx = 0;
        size_t token_len = strlen(token);

        // Pomiń początkowe spacje
        size_t start_idx = 0;
        while (start_idx < token_len && isspace(token[start_idx])) {
            start_idx++;
        }

        // Kopiuj tylko znaki do pierwszej spacji dla parametrów innych niż tekst
        for (size_t i = start_idx; i < token_len && clean_idx < sizeof(cleaned_token) - 1; i++) {
            if (*fmt_ptr != 't' && isspace(token[i])) {
                break;
            }
            cleaned_token[clean_idx++] = token[i];
        }
        cleaned_token[clean_idx] = '\0';

        switch (*fmt_ptr) {
            case 'u': {
                char* endptr;
                unsigned long val = strtoul(cleaned_token, &endptr, 10);
                if (*endptr != '\0' || val > 255) {
                    free(data_copy);
                    va_end(args);
                    return false;
                }
                uint8_t* ptr = va_arg(args, uint8_t*);
                *ptr = (uint8_t)val;
                USART_fsend("Parsed uint8_t: %u\r\n", (uint8_t)val);
                break;
            }
            case 's': {
                Color_t* color_ptr = va_arg(args, Color_t*);
                if (!parse_color(cleaned_token, color_ptr)) {
                    free(data_copy);
                    va_end(args);
                    return false;
                }
                break;
            }
            case 't': {
                char* ptr = va_arg(args, char*);
                // Dla tekstu, kopiujemy całą pozostałą część danych
                strncpy(ptr, token, 49);
                ptr[49] = '\0';
                break;
            }
            default:
                free(data_copy);
                va_end(args);
                return false;
        }

        token = strtok(NULL, ",");
        fmt_ptr++;
        param_count++;
    }

    bool success = (*fmt_ptr == '\0' && token == NULL);
    if (!success) {
        USART_fsend("Parameter count mismatch\r\n");
    }

    free(data_copy);
    va_end(args);
    return success;
}

// Funkcja do czyszczenia ramki
void clear_frame(Receive_Frame* frame) {
    if (frame) {
        memset(frame->data, 0, sizeof(frame->data));
        memset(frame->command, 0, sizeof(frame->command));
        // Wyczyść wszystkie inne pola ramki, jeśli istnieją
    }
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
    char text[50] = {0};
    wchar_t wtext[50] = {0};
    uint8_t x = 0, y = 0, fontSize = 0;
    Color_t color = BLACK;

    USART_fsend("Executing ONN with data: %s\r\n", frame->data);

    // Zmieniliśmy format z "uuust" na "uuuss" - ostatni parametr koloru jako string
    if (!parse_parameters(frame->data, "uuust", &x, &y, &fontSize, &color, text)) {
        prepareFrame(STM32_ADDR, PC_ADDR, "BCK", "NOT_RECOGNIZED%s", frame->data);
        return;
    }
    size_t textLen = strlen(text);
    for(size_t i = 0; i < textLen && i < 49; i++) {
        wtext[i] = (wchar_t)text[i];
    }
    wtext[textLen] = L'\0';

    switch(fontSize)
    {
        case 1:
            hagl_put_text(wtext, x, y, color, font5x7);
            break;
        case 2:
            hagl_put_text(wtext, x, y, color, font5x8);
            break;
        case 3:
            hagl_put_text(wtext, x, y, color, font6x9);
            break;
    }
}


/************************************************************************
* Function: main()
* (some details about what main does here...)
************************************************************************/
//TODO bład parsowania danych
static void executeOFF(Receive_Frame *frame)
{

	switch(frame->data[0])
	{
	case 0:
		HAL_GPIO_WritePin(BL_GPIO_Port, BL_Pin, GPIO_PIN_RESET); //TODO sprawdzić dlaczego nie działa
		break;
	case 1:
		hagl_fill_rectangle(0,0, LCD_WIDTH, LCD_HEIGHT, BLACK);
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
	        	if (strcmp(commandTable[i].command, "OFF") == 0 && strlen(frame->data) == 1) {
	        	                lcd_clear();
	        	                commandTable[i].function(frame);
	        	                lcd_copy();
	        	                clear_frame(frame);
	        	                return;
	        	}
	            int x, y;
	            if (parse_coordinates(frame->data, &x, &y)) {
	            	USART_fsend("%s", frame->data);
	            	USART_fsend("\r\n");
	                // Sprawdzenie zakresu współrzędnych
	                if (is_within_bounds(x, y)) {
	                    lcd_clear();
	                    commandTable[i].function(frame); // Wywołaj przypisaną funkcję
	                    lcd_copy();
	                    clear_frame(frame);
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


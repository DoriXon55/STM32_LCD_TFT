/*
 * frame.c
 *
 *  Created on: Nov 6, 2024
 *      Author: doria
 */
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


extern uint8_t USART_RxBuf[];
extern uint8_t USART_TxBuf[];
extern volatile int USART_TX_Empty;
extern volatile int USART_TX_Busy;
extern volatile int USART_RX_Empty;
extern volatile int USART_RX_Busy;

#include <string.h>
#include "crc.h"  // Zakładamy, że crc16_ccitt jest zaimplementowane
//==========================OBSŁUGA KOMEND================================

static void executeONK(Receive_Frame *frame)
{

	USART_fsend("Wykonanie ONK z danymi: %s\n", frame->data);
	uint8_t x = 0, y = 0, r = 0, filling = 0;
	uint16_t color = 0;
	lcd_init();
	switch(filling)
	{
	case 0:
		hagl_draw_circle(x, y, r, color);
		break;
	case 1:
		hagl_fill_circle(x, y, r, color);
		break;
	}
	lcd_copy();
}
static void executeONP(Receive_Frame *frame)
{
	USART_fsend("Wykonanie ONP z danymi: %s\n", frame->data);
	uint8_t x = 0, y = 0, width = 0, height = 0, filling = 0;
	uint16_t color = 0;
	lcd_init();
	switch(filling)
	{
	case 0:
		hagl_draw_rectangle(x, y, width, height, color);
		break;
	case 1:
		hagl_fill_rectangle(x, y, width, height, color);
		break;
	}
	lcd_copy();


}
static void executeONT(Receive_Frame *frame)
{
	USART_fsend("Wykonanie ONT z danymi: %s\n", frame->data);
	uint8_t x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0, filling = 0;
	uint16_t color = 0;
	lcd_init();
	switch(filling)
	{
	case 0:
		hagl_draw_triangle(x1, y1, x2, y2, x3, y3, color);
		break;
	case 1:
		hagl_fill_triangle(x1, y1, x2, y2, x3, y3, color);
		break;
	}
	lcd_copy();

}
static void executeONN(Receive_Frame *frame)
{
	USART_fsend("Wykonanie ONN z danymi: %s\n", frame->data);

	const char *text[512];
	uint8_t x1 = 0, x2 = 0, fontSize = 0;
	uint16_t color = 0;
	lcd_init();
	switch(fontSize)
	{
	case 1:
		hagl_put_text(text, x1, x2, color, font5x7); //fontSize zmien
		break;
	case 2:
		hagl_put_text(text, x1, x2, color, font5x8); //fontSize zmien
		break;
	case 3:
		hagl_put_text(text, x1, x2, color, font6x9); //fontSize zmien
		break;
	}
	lcd_copy();



}
static void executeOFF(Receive_Frame *frame)
{
	USART_fsend("Wykonanie OFF z danymi: %s\n", frame->data);
	uint8_t state = 0;
	switch(state)
	{
	case 0:
		//off
		break;
	case 1:
		//reset
		break;
	}
	lcd_copy();

}

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
//=======================OBSŁUGA RAMKI=========================
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

void prepareFrame(uint8_t sender, uint8_t receiver, const char *command, const char *format, ...) {
	Frame frame = {0};
	    frame.frame_start = FRAME_START;
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

	    // Przygotowanie do byte-stuffingu
	    uint8_t raw_payload[2 + COMMAND_LENGTH + data_len + 2];
	    raw_payload[0] = frame.sender;
	    raw_payload[1] = frame.receiver;
	    memcpy(raw_payload + 2, frame.command, COMMAND_LENGTH);
	    memcpy(raw_payload + 2 + COMMAND_LENGTH, frame.data, data_len);
	    raw_payload[2 + COMMAND_LENGTH + data_len] = (uint8_t)crc_output[0]; // Pierwszy bajt CRC
	    raw_payload[2 + COMMAND_LENGTH + data_len + 1] = (uint8_t)crc_output[1]; // Drugi bajt CRC

	    uint8_t stuffed_payload[512];
	    size_t stuffed_len = byteStuffing(raw_payload, 2 + COMMAND_LENGTH + data_len + 2, stuffed_payload);

	    // Wysyłanie ramki
	    USART_fsend("%c", FRAME_START); // Wyślij początek ramki
	    for (size_t i = 0; i < stuffed_len; i++) {
	        USART_fsend("%c", stuffed_payload[i]); // Wyślij dane po byte-stuffingu
	    }
	    USART_fsend("%c", FRAME_END); // Wyślij koniec ramki

}


bool decodeFrame(char *bx, Receive_Frame *frame, uint8_t len) {
    char ownCrc[2];
    char incCrc[2];
    //USART_fsend("\r\ndlugosc: %d\r\n",len);
        if(len >= MIN_DECODED_FRAME_LEN && len <= MAX_FRAME_LEN) {
            //USART_fsend("bede zapisywac ramke w strukturze\r\n");
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
            for (int l = 0; l < 2; l++) {
                if(ownCrc[l] != incCrc[l]) {
                    return false;
                }
            }
            return true; // crc zosta?o pomy?lnie por?wnanie
        }
    return false; // ramka niepoprawna
}


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
	                    USART_fsend("Współrzędne poprawne: x = %d, y = %d\r\n", x, y);
	                    commandTable[i].function(frame); // Wywołaj przypisaną funkcję
	                    return;
	                } else {
	                    USART_fsend("Współrzędne poza zakresem: x = %d, y = %d\r\n", x, y);
	                    return;
	                }
	            } else {
	                USART_fsend("Błąd parsowania współrzędnych w danych: %s\r\n", frame->data);
	                return;
	            }
	        }
	    }
	    USART_fsend("Nieznana komenda: %s\r\n", frame->command);
}

/*
void sendBack(Frame *frame, Receive_Frame *receive_frame, char command){

	//TODO nadpisać rameczkę z komunikatem powrotnym
	frame->command = "BCK";
	frame->data = command + " " + receive_frame->data; //-> z wywołanego błędu

	USART_fsend("~skibidi~", frame->command, frame->data);
}
*/

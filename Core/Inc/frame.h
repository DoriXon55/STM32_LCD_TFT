/*
 * frame.h
 *
 *  Created on: Nov 6, 2024
 *      Author: doria
 */

#ifndef INC_FRAME_H_
#define INC_FRAME_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

//==============================DEFINICJE FLAG=======================================

#define FRAME_START '~'
#define FRAME_END '`'
#define ESCAPE_CHAR '}'


//==============================DEFINICJE ADRESÓW=======================================
#define STM32_ADDR 'h'
#define PC_ADDR	'g'

//==============================DEFINICJE WIELKOŚCI=======================================
#define MAX_DATA_SIZE 128
#define MIN_FRAME_LEN 10
#define MAX_FRAME_LEN 270
#define COMMAND_LENGTH 3
#define MIN_DECODED_FRAME_LEN 7


//====================KOMENDY KTÓRE UŻYTKOWNIK MOŻE WYKONAĆ=========================
#define COMMAND_COUNT 5
#define COMMAND_ONK "ONK"//wyświetlanie koła
#define COMMAND_ONP "ONP"//wyświetlanie prostokąta
#define COMMAND_ONT "ONT"//wyświetlanie trójkąta
#define COMMAND_ONN "ONN"//wyświetlanie napisu
#define COMMAND_OFF "OFF"//wyłączenie wyświetlacza

//===================STRUKTURA DO TWORZENIA ORAZ WYSYŁANIA RAMKI====================
typedef struct{
	uint8_t frame_start;
	uint8_t sender;
	uint8_t receiver;
	uint8_t command[COMMAND_LENGTH];
	uint8_t data[MAX_DATA_SIZE];
	uint16_t crc;
	uint8_t frame_end;
}Frame;


//=====================STRUKTURA DLA ODBIORU I DEKODOWANIA RAMKI=================
typedef struct{
	char receiver;
	char sender;
	char command[COMMAND_LENGTH];
	char data[MAX_DATA_SIZE];
} Receive_Frame;


//====================STRUKTURA DLA ROZPOZNAWANIA KOMENDY======================
typedef struct {
    char command[COMMAND_LENGTH];
    void (*function)(Receive_Frame *frame);
} CommandEntry;


//===================FUNCKJE DLA RAMEK======================================
void prepareFrame(uint8_t sender, uint8_t receiver, const char *command, const char *format, ...);
size_t byteStuffing(uint8_t *input, size_t input_len, uint8_t *output);
size_t byteUnstuffing(uint8_t *input, size_t input_len, uint8_t *output);
bool decodeFrame(uint8_t *bx, Receive_Frame *frame, uint8_t len);
void handleCommand(Receive_Frame *frame);
void process_received_char(uint8_t received_char);





#endif /* INC_FRAME_H_ */

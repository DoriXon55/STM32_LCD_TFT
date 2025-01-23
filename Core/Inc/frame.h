/*
 * frame.h
 *
 *  Created on: Nov 6, 2024
 *      Author: doria
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>

//==============================DEFINICJE FLAG=======================================

#define FRAME_START '~'
#define FRAME_END '`'
#define ESCAPE_CHAR '}'
#define ESCAPE_CHAR_STUFF ']'
#define FRAME_END_STUFF '&'
#define FRAME_START_STUFF '^'


//==============================DEFINICJE ADRESÓW=======================================
#define STM32_ADDR 'h'
#define PC_ADDR	'g'

//==============================DEFINICJE WIELKOŚCI=======================================
#define MIN_FRAME_LEN 7
#define MAX_FRAME_LEN 270
#define COMMAND_LENGTH 3
#define MIN_DECODED_FRAME_LEN 7
#define MAX_FRAME_WITHOUT_STUFFING 135
#define MAX_DATA_SIZE 128

//====================KOMENDY KTÓRE UŻYTKOWNIK MOŻE WYKONAĆ=========================
#define COMMAND_COUNT 5
#define COMMAND_ONK "ONK" //wyświetlanie koła
#define COMMAND_ONP "ONP" //wyświetlanie prostokąta
#define COMMAND_ONT "ONT" //wyświetlanie trójkąta
#define COMMAND_ONN "ONN" //wyświetlanie napisu
#define COMMAND_OFF "OFF" //wyłączenie wyświetlacza


//=====================STRUKTURA DLA ODBIORU I DEKODOWANIA RAMKI=================
typedef struct{
	uint8_t sender;
	uint8_t receiver;
	char command[COMMAND_LENGTH];
	uint8_t data[MAX_DATA_SIZE];
} Frame;

//====================STRUKTURA DLA ROZPOZNAWANIA KOMENDY======================

/************************************************************************
* Struktura: CommandEntry
* Cel: Definiuje mapowanie między komendą a funkcją ją obsługującą
*
*   1. command[COMMAND_LENGTH]:
*      - Tablica znaków przechowująca nazwę komendy
*      - Długość określona przez COMMAND_LENGTH (zazwyczaj 3)
*      - Przykład: "ONK", "ONP", "ONN"
*
*   2. void (*function)(Receive_Frame *frame):
*      - Wskaźnik na funkcję obsługującą komendę
*      - Przyjmuje parametr typu Receive_Frame*
*      - Zwraca void
************************************************************************/
typedef struct {
    char command[COMMAND_LENGTH];
    void (*function)(Frame *frame);
} CommandEntry;

typedef struct {
    wchar_t displayText[50];
    int16_t x;              // Aktualna pozycja X
    int16_t y;              // Aktualna pozycja Y
    int16_t startX;         // Początkowa pozycja X
    int16_t startY;         // Początkowa pozycja Y
    uint8_t fontSize;
    uint8_t scrollSpeed;
    uint16_t color;
    uint8_t textLength;
    bool isScrolling;
    bool firstIteration;
    uint32_t lastUpdate;
} ScrollingTextState;



typedef enum {
    ERR_GOOD = 0,           // ramka poprawnie odebrana
    ERR_FAIL,               // ramka niepoprawnie odebrana
    ERR_WRONG_SENDER,       // nieprawidłowy nadawca
    ERR_WRONG_CRC,         // niezgodność sumy kontrolnej

    // Błędy wyświetlacza
    ERR_DISPLAY_AREA,         // współrzędne poza obszarem wyświetlacza
    ERR_WRONG_OFF_DATA,       // niepoprawna wartość dla komendy OFF
    ERR_WRONG_DATA,           // niepoprawne dane figury
    ERR_INVALID_TRIANGLE,     // niepoprawne parametry trójkąta
    ERR_TOO_MUCH_TEXT,        // przekroczony limit znaków
    ERR_NOT_RECOGNIZED,       // błąd detekcji wartości

    STATUS_COUNT
} StatusCode_t;

static const char* const STATUS_MESSAGES[] = {
    "GOOD",              // STATUS_GOOD
    "FAIL",              // STATUS_FAIL
    "WRONG_SENDER",      // STATUS_WRONG_SENDER
    "WRONG_CRC",         // STATUS_WRONG_CRC

    "DISPLAY_AREA",      // ERR_DISPLAY_AREA
    "WRONG_OFF_DATA",    // ERR_WRONG_OFF_DATA
    "WRONG_DATA",        // ERR_WRONG_DATA
    "INVALID_TRIANGLE",  // ERR_INVALID_TRIANGLE
    "TOO_MUCH_TEXT",     // ERR_TOO_MUCH_TEXT
    "NOT_RECOGNIZED"     // ERR_NOT_RECOGNIZED
};


//===================FUNCKJE DLA RAMEK======================================
void prepareFrame(uint8_t sender, uint8_t receiver, const char *command, const char *format, ...);
size_t byteStuffing(uint8_t *input, size_t input_len, uint8_t *output);
size_t byteUnstuffing(uint8_t *input, size_t input_len, uint8_t *output);
bool decodeFrame(uint8_t *bx, Frame *frame, uint8_t len);
void handleCommand(Frame *frame);
void processReceivedChar(uint8_t received_char);
void updateScrollingText(void);


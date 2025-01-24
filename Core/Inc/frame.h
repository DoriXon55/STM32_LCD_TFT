/*
 * frame.h
 *
 *  Created on: Nov 6, 2024
 *      Author: doria
 */
#pragma once


//==============================INCLUDES============================================
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
//==================================================================================


//==============================DEFINICJE FLAG======================================
#define FRAME_START '~'
#define FRAME_END '`'
#define ESCAPE_CHAR '}'
#define ESCAPE_CHAR_STUFF ']'
#define FRAME_END_STUFF '&'
#define FRAME_START_STUFF '^'
//==================================================================================


//====================================ADRESY========================================
#define STM32_ADDR 'h'
#define PC_ADDR	'g'
//==================================================================================


//==============================DEFINICJE WIELKOŚCI=================================
#define MIN_FRAME_LEN 7
#define MAX_FRAME_LEN 270
#define COMMAND_LENGTH 3
#define MIN_DECODED_FRAME_LEN 7
#define MAX_FRAME_WITHOUT_STUFFING 135
#define MAX_DATA_SIZE 128
//==================================================================================


//====================KOMENDY KTÓRE UŻYTKOWNIK MOŻE WYKONAĆ=========================
#define COMMAND_COUNT 5
#define COMMAND_ONK "ONK"
#define COMMAND_ONP "ONP"
#define COMMAND_ONT "ONT"
#define COMMAND_ONN "ONN"
#define COMMAND_OFF "OFF"
//==================================================================================


//=====================STRUKTURY==========================
typedef struct{
	uint8_t sender;
	uint8_t receiver;
	char command[COMMAND_LENGTH];
	uint8_t data[MAX_DATA_SIZE];
} Frame;

typedef struct {
    char command[COMMAND_LENGTH];
    void (*function)(Frame *frame);
} CommandEntry;

typedef struct {
    wchar_t displayText[50];
    int16_t x;
    int16_t y;
    int16_t startX;
    int16_t startY;
    uint8_t fontSize;
    uint8_t scrollSpeed;
    uint16_t color;
    uint8_t textLength;
    bool isScrolling;
    bool firstIteration;
    uint32_t lastUpdate;
} ScrollingTextState;

typedef enum {
    ERR_GOOD = 0,
    ERR_FAIL,
    ERR_WRONG_SENDER,
    ERR_WRONG_CRC,

    ERR_DISPLAY_AREA,
    ERR_WRONG_OFF_DATA,
    ERR_WRONG_DATA,
    ERR_INVALID_TRIANGLE,
    ERR_TOO_MUCH_TEXT,
    ERR_NOT_RECOGNIZED,

    STATUS_COUNT
} StatusCode_t;

static const char* const STATUS_MESSAGES[] = {
    "GOOD",
    "FAIL",
    "WRONG_SENDER",
    "WRONG_CRC",

    "DISPLAY_AREA",
    "WRONG_OFF_DATA",
    "WRONG_DATA",
    "INVALID_TRIANGLE",
    "TOO_MUCH_TEXT",
    "NOT_RECOGNIZED"
};
//=======================================================


//===================FUNCKJE DLA RAMEK======================================
void prepareFrame(uint8_t sender, uint8_t receiver, const char *command, const char *format, ...);
void handleCommand(Frame *frame);
void processReceivedChar(uint8_t received_char);
void updateScrollingText(void);
//==========================================================================

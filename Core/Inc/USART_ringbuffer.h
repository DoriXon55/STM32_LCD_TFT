/*
 * USART_ringbuffer.h
 *
 *  Created on: Nov 2, 2024
 *      Author: doria
 */
#pragma once
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include "main.h"
#include "usart.h"
#include <stdint.h>

//==============================DEFINICJE WIELKOŚCI BUFORA KOŁOWEGO=============================
#define TX_BUFFER_SIZE 2048
#define RX_BUFFER_SIZE 256

typedef struct {
	uint8_t* buffer;
	uint32_t readIndex;
	uint32_t writeIndex;
	uint32_t mask;
}ring_buffer;

uint8_t USART_kbhit(void);
int16_t USART_getchar(void);
uint8_t USART_getline(char *buf);
void USART_fsend(char* format, ...);
void ringBufferSetup(ring_buffer* rb, uint8_t* buffer, uint32_t size);


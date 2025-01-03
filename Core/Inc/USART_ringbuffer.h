/*
 * USART_ringbuffer.h
 *
 *  Created on: Nov 2, 2024
 *      Author: doria
 */

#ifndef INC_USART_RINGBUFFER_H_
#define INC_USART_RINGBUFFER_H_

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
/*
 * Sprawdza czy w buforze odbiorczym znajdują się dane
 * Zwraca 1 jeśli bufor zawiera dane do odczytu
 */
uint8_t USART_kbhit(void);

/*
 * Funkcja pobierająca jeden bajt z bufora odbiorczego.
 * Jeśli bufor pusty = -1
 * Jeśli jest dostępny bajt to funkcja zwraca jesgo wartość
 * oraz aktualizuje index RX_Busy tak aby wskazywał na kolejny
 * bajt do odczytu
 */
int16_t USART_getchar(void);

/*
 * Pobiera linię tekstu z bufora odbiorczego aż do napotkania
 * znaku końca linii 10 lub 13.
 * Kopiuje otrzymane znaki do buffer. Jeśli linia jest długa to
 * znaki zawija w ramach bufora o maks. długośći 128 znaków (na razie)
 * 0x0A = 10 LF - Line Feed a 0x0D = 13 CR - Carriage Return znaki końca linii
 */
uint8_t USART_getline(char *buf);


/*
 * Formatuje teskt i zapisuje go do bufora nadawczego. Jeśli
 * rejest jest gotowy, funkcja inicjuje przerwanie transmisji
 * pierwszego bajru. __disable_irq() oraz __enable_irq() to
 * nic innego jak zabezpieczenie kodu przed sekcją krytyczną
 */
void USART_fsend(char* format, ...);

/*
 * Utworzenie instancji bufora
 */
void ring_buffer_setup(ring_buffer* rb, uint8_t* buffer, uint32_t size);

#endif /* INC_USART_RINGBUFFER_H_ */

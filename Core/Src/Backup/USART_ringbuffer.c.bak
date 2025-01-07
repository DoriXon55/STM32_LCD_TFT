/*
 * USART_Ringbuffer.c
 *
 *  Created on: Nov 2, 2024
 *      Author: doria
 */
#include "USART_ringbuffer.h"
#include "main.h"
#include "frame.h"
ring_buffer rxRingBuffer;
ring_buffer txRingBuffer;
uint8_t USART_TxBuf[TX_BUFFER_SIZE];
uint8_t USART_RxBuf[RX_BUFFER_SIZE];

/************************************************************************
* Funkcja: ringBufferSetup()
* (Utworzenie instancji bufora)
************************************************************************/
void ringBufferSetup(ring_buffer* rb, uint8_t* buffer, uint32_t size)
{
	rb->buffer = buffer;
	rb->readIndex = 0;
	rb->writeIndex = 0;
	rb->mask = size - 1;
}

/************************************************************************
* Funkcja: USART_kbhit()
* (Sprawdza czy w buforze odbiorczym znajdują się dane
* Zwraca 1 jeśli bufor zawiera dane do odczytu)
************************************************************************/
uint8_t USART_kbhit(){
	if(rxRingBuffer.writeIndex == rxRingBuffer.readIndex){
		return 0;
	}else{
		return 1;
	}
}

/************************************************************************
* Funkcja: USART_getchar()
* (Funkcja pobierająca jeden bajt z bufora odbiorczego.
* Jeśli bufor pusty = -1
* Jeśli jest dostępny bajt to funkcja zwraca jesgo wartość
* oraz aktualizuje index RX_Busy tak aby wskazywał na kolejny
* bajt do odczytu)
************************************************************************/
int16_t USART_getchar() {
    if (rxRingBuffer.writeIndex != rxRingBuffer.readIndex) {
        int16_t tmp = USART_RxBuf[rxRingBuffer.readIndex];
        rxRingBuffer.readIndex = (rxRingBuffer.readIndex + 1) % rxRingBuffer.mask;
        return tmp;
    }
    return -1;
}

/************************************************************************
* Funkcja: USART_getline()
* (Pobiera linię tekstu z bufora odbiorczego aż do napotkania
* znaku końca linii 10 lub 13.
* Kopiuje otrzymane znaki do buffer. Jeśli linia jest długa to
* znaki zawija w ramach bufora o maks. długośći 128 znaków (na razie)
* 0x0A = 10 LF - Line Feed a 0x0D = 13 CR - Carriage Return znaki końca linii)
************************************************************************/
uint8_t USART_getline(char *buf){
	static uint8_t buffer[128];
	static uint8_t index = 0;
	int i;
	uint8_t ret;

	// dopóki są dane w buforze
	while(USART_kbhit()){
		buffer[index] = USART_getchar();
		if(((buffer[index] == 0x0A)||(buffer[index] == 0x0D)))
		{
			buffer[index] = 0;
			for(i = 0;i <= index; i++){
				buf[i] = buffer[i];
			}
			ret = index;
			index = 0;
			return ret;
		}else{
			index++;
			if(index >= sizeof(buffer)) index = 0;
		}
	}
	return 0;
}

/************************************************************************
* Funkcja: USART_fsend()
* (Formatuje teskt i zapisuje go do bufora nadawczego. Jeśli
* rejest jest gotowy, funkcja inicjuje przerwanie transmisji
* pierwszego bajru. __disable_irq() oraz __enable_irq() to
* nic innego jak zabezpieczenie kodu przed sekcją krytyczną)
************************************************************************/
void USART_fsend(char* format,...){
	char tmp_rs[258];
	int i;
	volatile int idx;
	va_list arglist;
	  va_start(arglist,format);
	  vsprintf(tmp_rs,format,arglist);
	  va_end(arglist);
	  idx=txRingBuffer.writeIndex;
	  for(i=0;i<strlen(tmp_rs);i++){
		  USART_TxBuf[idx]=tmp_rs[i];
		  idx++;
		  if(idx >= TX_BUFFER_SIZE)idx=0;
	  }
	  __disable_irq();//wyłączamy przerwania
	  if((txRingBuffer.writeIndex==txRingBuffer.readIndex)&&(__HAL_UART_GET_FLAG(&huart2,UART_FLAG_TXE)==SET)){//sprawdzic dodatkowo zajetosc bufora nadajnika
		  txRingBuffer.writeIndex=idx;
		  uint8_t tmp=USART_TxBuf[txRingBuffer.readIndex];
		  txRingBuffer.readIndex++;
		  if(txRingBuffer.readIndex >= TX_BUFFER_SIZE)txRingBuffer.readIndex=0;
		  HAL_UART_Transmit_IT(&huart2, &tmp, 1);
	  }else{
		  txRingBuffer.writeIndex=idx;
	  }
	  __enable_irq();
}




/************************************************************************
* Funkcja: USART_sendFrame()
* (Funkcja wysyłająca ramkę danych przez UART z wykorzystaniem bufora
* kołowego. Dodaje znaki początku i końca ramki oraz zapisuje dane do
* bufora nadawczego. Transmisja realizowana jest z wykorzystaniem
* przerwań.
*
* Parametry wejściowe:
* - data: wskaźnik na tablicę bajtów zawierającą dane do wysłania
* - length: długość danych w bajtach
*
* Działanie:
* 1. Wyłącza przerwania aby zabezpieczyć sekcję krytyczną
* 2. Dodaje znak początku ramki (FRAME_START) do bufora
* 3. Kopiuje dane wejściowe do bufora nadawczego z kontrolą przepełnienia
* 4. Dodaje znak końca ramki (FRAME_END) do bufora
* 5. Inicjuje transmisję pierwszego bajtu przez przerwanie, jeśli:
*    - bufor był pusty (writeIndex == readIndex)
*    - rejestr nadawczy jest gotowy (TXE = 1)
* 6. Aktualizuje wskaźnik zapisu w buforze kołowym
* 7. Włącza przerwania
************************************************************************/
void USART_sendFrame(const uint8_t* data, size_t length) {
    int idx = txRingBuffer.writeIndex;

    __disable_irq();

    // Dodaj początek ramki
    USART_TxBuf[idx] = FRAME_START;
    idx++;
    if(idx >= TX_BUFFER_SIZE) idx = 0;

    // Kopiuj dane do bufora nadawczego
    for(size_t i = 0; i < length; i++) {
        USART_TxBuf[idx] = data[i];
        idx++;
        if(idx >= TX_BUFFER_SIZE) idx = 0;
    }

    // Dodaj koniec ramki
    USART_TxBuf[idx] = FRAME_END;
    idx++;
    if(idx >= TX_BUFFER_SIZE) idx = 0;

    // Rozpocznij transmisję jeśli bufor był pusty
    if((txRingBuffer.writeIndex == txRingBuffer.readIndex) &&
       (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE) == SET)) {
        txRingBuffer.writeIndex = idx;
        uint8_t tmp = USART_TxBuf[txRingBuffer.readIndex];
        txRingBuffer.readIndex++;
        if(txRingBuffer.readIndex >= TX_BUFFER_SIZE) txRingBuffer.readIndex = 0;
        HAL_UART_Transmit_IT(&huart2, &tmp, 1);
    } else {
        txRingBuffer.writeIndex = idx;
    }

    __enable_irq();
}

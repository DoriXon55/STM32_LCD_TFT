/*
 * USART_Ringbuffer.c
 *
 *  Created on: Nov 2, 2024
 *      Author: doria
 */

//=======================INCLUDES==========================
#include "USART_ringbuffer.h"
#include "main.h"
#include "frame.h"
//==========================================================

//=======================ZMIENNE==========================
ring_buffer rxRingBuffer;
ring_buffer txRingBuffer;
uint8_t USART_TxBuf[TX_BUFFER_SIZE];
uint8_t USART_RxBuf[RX_BUFFER_SIZE];
//=========================================================

void ringBufferSetup(ring_buffer* rb, uint8_t* buffer, uint32_t size)
{
	rb->buffer = buffer;
	rb->readIndex = 0;
	rb->writeIndex = 0;
	rb->mask = size;
}

uint8_t USART_kbhit(){
	if(rxRingBuffer.writeIndex == rxRingBuffer.readIndex){
		return 0;
	}else{
		return 1;
	}
}

int16_t USART_getchar() {
    if (rxRingBuffer.writeIndex != rxRingBuffer.readIndex) {
        int16_t tmp = USART_RxBuf[rxRingBuffer.readIndex];
        rxRingBuffer.readIndex = (rxRingBuffer.readIndex + 1) % rxRingBuffer.mask;
        return tmp;
    }
    return -1;
}

void USART_sendFrame(const uint8_t* data, size_t length) {
    int idx = txRingBuffer.writeIndex;

    __disable_irq();
    // Kopiuj dane do bufora nadawczego
    for(size_t i = 0; i < length; i++) {
        USART_TxBuf[idx] = data[i];
        idx = (idx + 1) % txRingBuffer.mask;
    }

    // Rozpocznij transmisję jeśli bufor był pusty
    if((txRingBuffer.writeIndex == txRingBuffer.readIndex) &&
       (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE) == SET)) {
        txRingBuffer.writeIndex = idx;
        uint8_t tmp = USART_TxBuf[txRingBuffer.readIndex];
        txRingBuffer.readIndex = (txRingBuffer.readIndex + 1) % txRingBuffer.mask;
        HAL_UART_Transmit_IT(&huart2, &tmp, 1);
    } else {
        txRingBuffer.writeIndex = idx;
    }

    __enable_irq();
}

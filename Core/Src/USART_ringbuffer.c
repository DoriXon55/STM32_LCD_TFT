/*
 * USART_Ringbuffer.c
 *
 *  Created on: Nov 2, 2024
 *      Author: doria
 */
#include "USART_ringbuffer.h"
#include "main.h"
uint8_t USART_TxBuf[USART_TXBUF_LEN];
uint8_t USART_RxBuf[USART_RXBUF_LEN];

volatile int USART_TX_Empty = 0; // wskaźnik do zapisania TX
volatile int USART_TX_Busy = 0; // wskaźnik do odczytu TX
volatile int USART_RX_Empty = 0; // wskaźnik do zapisania RX
volatile int USART_RX_Busy = 0; // wskaźnik do odczytu RX

uint8_t USART_kbhit(){
	if(USART_RX_Empty == USART_RX_Busy){
		return 0;
	}else{
		return 1;
	}
}

int16_t USART_getchar(){
	if(USART_RX_Empty != USART_RX_Busy){
		int16_t tmp = USART_RxBuf[USART_RX_Busy]; // odczyt znaku
		 USART_RX_Busy++;
		 if(USART_RX_Busy >= USART_RXBUF_LEN)
		 {
			 if(USART_RXBUF_LEN % 2 == 0)
			 {
				 USART_RX_Busy = (USART_RX_Busy + 1) & USART_RXBUF_MASK;
			 } else {
				 USART_RX_Busy = 0;
			 }
		 }
		 return tmp;
	} else {
		return -1; //bufor pusty
	}
}

uint8_t USART_getline(char *buf){
	static uint8_t buffer[128];
	static uint8_t index = 0;
	int i;
	uint8_t ret;

	// dopóki są dane w buforze
	while(USART_kbhit()){
		buffer[index] = USART_getchar(); //odczyt znaku
		if(((buffer[index] == 0x0A)||(buffer[index] == 0x0D))) //napotyka koniec wiersza
		{
			buffer[index] = 0;
			for(i = 0;i <= index; i++){
				buf[i] = buffer[i]; //kopiowanie zawartości buffera do buf
			}
			ret = index;
			index = 0;
			return ret; //zwracanie długości linii
		}else{
			index++;
			if(index >= sizeof(buffer)) index = 0; //jeżeli nie ma końca znaku to zawijamy
		}
	}
	return 0;
}

void USART_fsend(char* format,...){
	char tmp_rs[256];
	int i;
	volatile int index;
	//wstawianie dodatkowych argumentów do formatu.
	va_list arglist;
	va_start(arglist, format);
	vsprintf(tmp_rs, format, arglist); //tworzenie ciągu danych do wysłania. Formatuje tekst z argumentów.
	va_end(arglist);
	index = USART_TX_Empty;

	// przechodzi przez każdy element tmp_rs i zapisuje go do TxBuf
	for(i = 0;i < strlen(tmp_rs); i++){
		USART_TxBuf[index] = tmp_rs[i];
		index++;
		if(index >= USART_TXBUF_LEN)index=0;
	}

	__disable_irq(); //zapobieganie sekcji krytycznej

	if((USART_TX_Empty == USART_TX_Busy)&&(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE) == SET)) //sprawdza czy bufor nie pusty i czy transmisja ready
	{
		USART_TX_Empty = index;
		uint8_t tmp = USART_TxBuf[USART_TX_Busy]; //zapisanie bajtu
		USART_TX_Busy++;
		if(USART_TX_Busy >= USART_TXBUF_LEN)
		{
			 if(USART_TXBUF_LEN % 2 == 0)
			 {
				 USART_TX_Busy = (USART_TX_Busy + 1) & USART_TXBUF_MASK;
			 } else {
				 USART_TX_Busy = 0;
			 }
		}
		HAL_UART_Transmit_IT(&huart2, &tmp, 1); //wysłąnie bajtu z bufora
	}else{
		USART_TX_Empty = index;
	}

	__enable_irq(); //włączenie przerwań
}

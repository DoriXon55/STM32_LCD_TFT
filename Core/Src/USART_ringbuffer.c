/*
 * USART_Ringbuffer.c
 *
 *  Created on: Nov 2, 2024
 *      Author: doria
 */
#include "USART_ringbuffer.h"
#include "main.h"
ring_buffer rxRingBuffer;
ring_buffer txRingBuffer;
uint8_t USART_TxBuf[TX_BUFFER_SIZE];
uint8_t USART_RxBuf[RX_BUFFER_SIZE];

void ring_buffer_setup(ring_buffer* rb, uint8_t* buffer, uint32_t size)
{
	rb->buffer = buffer;
	rb->readIndex = 0;
	rb->writeIndex = 0;
	rb->mask = size - 1; // zakładając, że zmienna size jest potęgą 2
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
    return -1; // Buffer empty
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
void USART_send(uint8_t message[]){

    uint16_t i,idx = txRingBuffer.writeIndex;


    for(i=0; message[i] != '\0'; i++){ // przenosimy tekst z wywołania funkcji USART_send do tablicy BUF_TX[]

            USART_TxBuf[idx] = message[i];
            idx++;
            if(idx >= TX_BUFFER_SIZE) idx = 0;
            if(idx == txRingBuffer.readIndex) txRingBuffer.readIndex++;
            //if(busy_TX >= sizeof(BUF_TX)) busy_TX=0;
            if(txRingBuffer.readIndex >= TX_BUFFER_SIZE) txRingBuffer.readIndex=0;

    } // cały tekst ze zmienne message[] znajduje się już teraz w BUF_TX[]

        __disable_irq(); //wyłączamy przerwania, bo poniżej kilka linii do zrobienia

        if (txRingBuffer.readIndex == txRingBuffer.writeIndex)
        {
            txRingBuffer.writeIndex = idx;
            uint8_t tmp = USART_TxBuf[txRingBuffer.readIndex];
            txRingBuffer.readIndex++;
            if(txRingBuffer.readIndex >= TX_BUFFER_SIZE) txRingBuffer.readIndex = 0;
            HAL_UART_Transmit_IT(&huart2, &tmp, 1);

            //HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        }
        else
        {
        	txRingBuffer.readIndex = idx;
        }

        __enable_irq(); //ponownie aktywujemy przerwania
}
void USART_fsend(char* format,...){
	char tmp_rs[128];
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

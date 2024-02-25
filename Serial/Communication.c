/*
 * Comunication.c
 *
 *  Created on: May 26, 2020
 *      Author: gert
 */
#include "Communication.h"
#include "Pins.h"
#include "stdbool.h"
#include "BeerCount.h"
/*-----------------------------------------------------------------------------------------------------------
 * Komunikation module lavet til L011.
 * Denne unit er skrevet til Uart1
 * HUSK
 * DMA skal være cirkulær
 * PA12 er Direction
 -----------------------------------------------------------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;


volatile uint8_t USART_txBuff[TxBuffSize];
uint8_t Uart_RxBuffer[RxBuffSize];

uint8_t DMA_RX_Tail = 0;
uint8_t DMA_RX_Head = 0;
int NRxReceived;
uint8_t xchar;

void initCom1() {
	NVIC_EnableIRQ(USART1_IRQn);
	HAL_UART_Receive_IT(&huart1, &xchar, 1); //vi venter på en header
	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
}

uint8_t CRCXor() {
	uint8_t val = 0;
	for (int i = 0; i < USART_txBuff[1]; i++) {
		val ^= USART_txBuff[i];
	}
	return val;
}

void TxRawframe(volatile uint8_t *data, uint16_t frametype, int Datasize) {
	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
	USART_txBuff[0] = 0x02;
	USART_txBuff[1] = Datasize + HeaderSize;   //Data length
	USART_txBuff[2] = frametype;
	USART_txBuff[3] = BeerRec.NetworkAdr;
	memcpy(&USART_txBuff[4], data, Datasize);
	USART_txBuff[USART_txBuff[1]] = CRCXor();
	HAL_UART_Transmit_IT(&huart1, &USART_txBuff[0], USART_txBuff[1] + 1);
//	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
}

void TxACK() {
	uint8_t ch = 0x06;
	TxRawframe(&ch, 0xff, 1);
}

void TxNACK() {
	uint8_t ch = 0x15;
	TxRawframe(&ch, 0xff, 1);
}

//-----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//RX
//----------------------------------------------------------------------------------------------
static bool crc() {
	uint8_t val = 0;
	for (int i = 0; i < uart.Rxbuffer[1]; i++) {
		val ^= uart.Rxbuffer[i];
	}
	return val==uart.Rxbuffer[uart.Rxbuffer[1]];
}

static void Uart_Handler(uint8_t *DataBuf, int Ndata) {
	for (int i = 0; i < Ndata; i++) {
		switch (uart.RxState) {
		case 0: {
			if (*DataBuf == 0x02) {
				uart.RxState = 1;
				uart.pRx = 0;
				uart.Rxbuffer[uart.pRx++] = *DataBuf;
			}
		}
			break;
		case 1: { //venter p� header
			uart.Rxbuffer[uart.pRx++] = *DataBuf;
			if (uart.pRx == HeaderSize) {
				uart.RxState = 2;
			}
		}
			break;
		case 2: { //venter p� resten af frame
			uart.Rxbuffer[uart.pRx++] = *DataBuf;
			if (uart.pRx >= uart.Rxbuffer[1]+1) { //==length
				if (crc())	uart.RxReady = 1; else uart.RxReady = 0;
				uart.RxState = 0;
			}
		}
			break;
		}
		DataBuf++;
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	  if (huart->Instance == USART1)
		Uart_Handler(&xchar,1);
	    HAL_UART_Receive_IT(&huart1, &xchar, 1);
}



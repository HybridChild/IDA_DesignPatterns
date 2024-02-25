/*
 * Comunication.h
 *
 *  Created on: May 26, 2020
 *      Author: gert
 */

#ifndef SRC_COMMUNICATION_H_
#define SRC_COMMUNICATION_H_
//--------------------------------------------------------------------------------------
#include "stdint.h"
#include "stm32l0xx_hal.h"
#include <string.h>

//--------------------------------------------------------------------------------------
#define HeaderSize 4
#define TxBuffSize 250
#define RxBuffSize          30

typedef struct {
  uint8_t RxReady; //indikation af data der er klar til behandling
  uint8_t Rxbuffer[RxBuffSize];
  uint8_t pRx;
  uint8_t RxState;

} Rxbuff;
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
Rxbuff uart;
void TxRawframe(volatile uint8_t *data, uint16_t frametype, int Datasize);
void initCom1();

#endif /* SRC_COMUNICATION_H_ */

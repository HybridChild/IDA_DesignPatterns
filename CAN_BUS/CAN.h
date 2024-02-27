/*
 * @file           : CAN.h
 * @brief          : basic function of the CAN controller with priority Q
 *  Created on     : Jul 1, 2021
 * @authors : Gert Lauritsen
 */

#ifndef CAN_BUS
#define CAN_BUS
#include "msg_type.h"
#include "stm32f3xx_hal.h"

#include "stdbool.h"

#define LOCAL_DIGITAL_INPUT_STATE_TRANSMIT_ON_CAN_BUS 0x00  // Input 18 til 1 (bit 0 = input 1)  bx0000 0000 0000 0000 0000 0000 00000 0000
#define LT_MESSAGE_TO_ALL_PROCESS_CONTROLLERS         0xAA  // MUST ALWAYS BE SECOND LAST!!!!

/* Structure defining a CAN-message */
typedef struct
{
  CAN_RxHeaderTypeDef RxHeader;
  CAN_TxHeaderTypeDef TxHeader;
  uint32_t LTCanPriority; /// Set periority beyond std CAN periority (has to do with the software Q)
  uint8_t data[8];
} can_msg_t;

typedef bool (*CALLBACK_FUNCTION_CAN_COM_RX_IN_APP)(can_msg_t* msg);          /// Callback type for data
typedef void (*CAN_TxMailbox1CompleteCallback)(CAN_HandleTypeDef* CANHandle); /// Callback for when a transmission has completed

uint32_t send_in_count;  /// Number of msg in
uint32_t send_out_count; /// Number of msg out
bool CanIsInitialized;   /// Flag that CAN has beeen Initialized
uint32_t CanErrorCode;   /// Give the reason for an CAN Error

/* Exported functions prototypes */
void can_driver_init(CALLBACK_FUNCTION_CAN_COM_RX_IN_APP cb);
bool Is_CAN_Ready(void);
uint16_t Get_No_on_last_send_telegram_in(void);
uint16_t Get_No_on_last_send_telegram_out(void);

bool CAN1_Send(can_msg_t msg);
bool CAN1_Receive(can_msg_t* ret);

uint16_t Get_CAN_Rx_Buffer_Cnt(void);
uint16_t Get_CAN_Tx_Free_Buffer_Size(void);

void SetCanTxComplete(CAN_TxMailbox1CompleteCallback cb);

#endif /* HWWRAPPER_CAN_H_ */

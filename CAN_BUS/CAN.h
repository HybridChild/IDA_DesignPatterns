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

#define LOCAL_DIGITAL_INPUT_STATE_TRANSMIT_ON_CAN_BUS  0x00        // Input 18 til 1 (bit 0 = input 1)  bx0000 0000 0000 0000 0000 0000 00000 0000
#define LT_MESSAGE_TO_ALL_PROCESS_CONTROLLERS          0xAA // MUST ALWAYS BE SECOND LAST!!!!

  /* Structure defining a CAN-message */
typedef struct {
    CAN_RxHeaderTypeDef Rxheader;
	CAN_TxHeaderTypeDef header;
    uint32_t LTCanPriority;         ///Set periority beyong std CAN periority (has to do with the software Q)
    uint8_t data[8];
} can_msg_t;

typedef	bool (* CALLBACK_FUNCTION_CAN_COM_RX_IN_APP )(can_msg_t *msg);   /// Callback type for data
typedef void (* CAN_TxMailbox1CompleteCallback) (CAN_HandleTypeDef *CANHandle); /// Callback for when a transmission has completed

uint32_t send_in_count,send_out_count; ///Number of msg in and out
bool      CanIsInitialized;  ///Flag that CAN has beeen Initialized
uint32_t  CanErrorCode;      ///Give the reason for an CAN Error

/* Exported functions prototypes ---------------------------------------------*/


void can_driver_init(CALLBACK_FUNCTION_CAN_COM_RX_IN_APP cb);


//can driver
/******************************************************************************
* Name    :    Is_CAN_Ready
* Input   :    -
* Output  :    true/false can_bus_aktiv
* Function:
* OLd name: extern bool Er_com_bus_aktiv (void);
******************************************************************************/
bool Is_CAN_Ready (void);

/*************************************************
* Name    :	Get_No_on_last_send_telegram_in
* Input   :    -
* Function:    Get_No_on_last_send_telegram_in
* Output  :    No. on last send telegram in
************************************************/
uint16_t Get_No_on_last_send_telegram_in(void);

/*************************************************
* Name    :	Get_No_on_last_send_telegram_out
* Input   :    -
* Function:    Get_No_on_last_send_telegram_out
* Output  :    No. on last send telegram in
************************************************/
uint16_t Get_No_on_last_send_telegram_out(void);

/*!
  @fn 		: 	Can1_Send
    Set data data in Q, if a mailbox is ready it is put in
    normal mod msg is put in mailboks 1, where as high priority msg is send with mailbox 0
  @param 	:	msg holds both the header, data and priority
*/
bool Can1_Send(can_msg_t msg);

bool CAN1_Receive(can_msg_t *ret);

uint16_t Get_CAN_Rx_Buffer_Cnt (void);
uint16_t Get_CAN_Tx_Free_Buffer_Size(void);

/*!
 *  @fn		: SetCanTxComplet
 *  @param	: pointer to callboack
 *  @brief	: Set a signal pointer that is called, when a transmition is completted
 */
void SetCanTxComplet(CAN_TxMailbox1CompleteCallback cb);




#endif /* HWWRAPPER_CAN_H_ */

/*
 * @file           : CAN.c
 *
 *  Created on: Jul 1, 2021
 * @authors       : Gert Lauritsen
 */

#include "CAN.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "string.h"
#include "Errors.h"

/**** Definitions ****/
#define CAN_DRIVER_TX_QUEUE_SIZE 100 /* Max number of CAN messages in the queue from the application to the CAN driver */
#define CAN_DRIVER_RX_QUEUE_SIZE 50  /* Max number of CAN messages in the queue from the CAN driver to the application */

static xQueueHandle _can_driver_tx0_queue;
static xQueueHandle _can_driver_tx1_queue;
static xQueueHandle _can_driver_rx_queue;

extern CAN_HandleTypeDef hcan; // deklareret i main.c by Cubemx

// #include "stm32f3xx_hal_can.h"
typedef enum
{
  CAN_OK = 0,
  CAN_ERROR_PASSIVE,
  CAN_ERROR_BUS_OFF
} can_fault;

// forward declaration---------------------------------------------------------------
bool CAN1_Send(can_msg_t msg);
uint32_t TxMailbox;
uint16_t MsgVarId(can_msg_t* msg);

// Var dec---------------------------------------------------------------------------
static can_fault errCAN1 = CAN_OK;
static CALLBACK_FUNCTION_CAN_COM_RX_IN_APP CanRxCB; /// Rx callback function
static CAN_TxMailbox1CompleteCallback TxComplete;
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

//----------------------------------------------------------------------------------

void can_filter_init()
{
  CAN_FilterTypeDef sFilterConfig;
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0000;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14;

  if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
  {
    /* Filter configuration Error */
    LT_Error_Handler();
  }
}

void can_driver_init(CALLBACK_FUNCTION_CAN_COM_RX_IN_APP cb)
{
  _can_driver_tx0_queue = xQueueCreate(CAN_DRIVER_TX_QUEUE_SIZE, sizeof(can_msg_t));
  _can_driver_tx1_queue = xQueueCreate(CAN_DRIVER_TX_QUEUE_SIZE, sizeof(can_msg_t));

  if (cb)
    CanRxCB = cb;
  else
    _can_driver_rx_queue = xQueueCreate(CAN_DRIVER_RX_QUEUE_SIZE, sizeof(can_msg_t));

  can_filter_init();
  if (HAL_CAN_Start(&hcan) != HAL_OK)
  {
    LT_Error_Handler();
  }

  if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
  {
    LT_Error_Handler();
  }
  if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    LT_Error_Handler();
  }
  //  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING);

  send_in_count = 0;
  send_out_count = 0;
  CanIsInitialized = true;
}

void SetCanTxComplete(CAN_TxMailbox1CompleteCallback cb)
{
  TxComplete = cb;
}
//------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------

/*!
  @fn   	CAN1_Send
  @brief  Set data data in Q, if a mailbox is ready it is put in
    normal mod msg is put in mailboks 1, where as high priority msg is send with mailbox 0
  @param 	:	msg holds both the header, data and priority
*/
bool CAN1_Send(can_msg_t msg)
{
  bool retVal = false;
  BaseType_t queueStatus;
  if (CanIsInitialized)
  {
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) > 0)
    { // there is room in the mailboxes so we send it rigth away
      retVal = HAL_CAN_AddTxMessage(&hcan, &msg.TxHeader, &msg.data[0], &TxMailbox) == HAL_OK;
      send_out_count++;
    }
    else
    { // add message to q
      if (msg.LTCanPriority == 0)
      {
        queueStatus = xQueueSend(_can_driver_tx0_queue, &msg, 0); // no delay if queue is full, to prevent slow down rest of the system
      }
      else
      {
        queueStatus = xQueueSend(_can_driver_tx1_queue, &msg, 0); // no delay if queue is full, to prevent slow down rest of the system
      }

      if (queueStatus == errQUEUE_FULL)
      {
        errCAN1 = CAN_ERROR_PASSIVE;
      }
      else
      {
        send_in_count++;
      }
      if (queueStatus == pdTRUE)
      {
        retVal = true;
      }
    } // end set in queue
  }
  return retVal;
}

/*!
 * @fn	CAN1_Receive
 * @params    can_msg_t
 * @brief Get a msg from msgq. Used if CB func is not used
 */
bool CAN1_Receive(can_msg_t* ret)
{
  return (CanIsInitialized && xQueueReceive(_can_driver_rx_queue, ret, 1) == pdTRUE);
#ifdef ENABLE_CAN_RECEIVE_DEBUG
  if (MsgVarId(ret) == filterVarId)
  {
    printf("CAN1 rx-queued %#010x : %d : %02x %02x %02x %02x %02x %02x %02x %02x\n", ret->id, ret->dlc, ret->data[0], ret->data[1], ret->data[2], ret->data[3], ret->data[4], ret->data[5], ret->data[6], ret->data[7]);
  }
#endif
}

/*!
 * @fn    :	Get_CAN_Tx_Free_Buffer_Size
 * @params   :    -
 * @returns  :    Size of free tx buffer
 */
uint16_t Get_CAN_Tx_Free_Buffer_Size(void)
{
  return uxQueueSpacesAvailable(_can_driver_tx1_queue);
}

/*!
 * @fn       :	Get_CAN_Rx_Buffer_Cnt
 *
 * @params   :    -
 * Function:    returns buffer counter value
 * @returns  :    buffer counter value
 */
uint16_t Get_CAN_Rx_Buffer_Cnt(void)
{
  uint16_t buf_val;
  buf_val = uxQueueMessagesWaiting(_can_driver_rx_queue);
  return buf_val;
}
//-----------------------------------------------------------------------------
// Can RxTx
//-----------------------------------------------------------------------------

/*!
 * @fn 	:	Is_CAN_Ready reurn if can is init and there is no errors
 */
bool Is_CAN_Ready(void)
{
  return CanIsInitialized && (errCAN1 == CAN_OK);
}

/*!
 * @fn 		:	HAL_CAN_RxFifo0MsgPendingCallback
 * @params 	:	handle to can
 * @returns :	if cb is not define msg is put in q
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan1)
{
  can_msg_t msg;
  CanErrorCode = HAL_CAN_ERROR_NONE; // reset ErrorCode
  HAL_CAN_GetRxMessage(hcan1, CAN_RX_FIFO0, &msg.RxHeader, &msg.data[0]);
  if (CanRxCB)
    CanRxCB(&msg);
  else
    xQueueSendFromISR(_can_driver_rx_queue, &msg, &xHigherPriorityTaskWoken); // put in Q}
}

/*!
 * @fn 		:	HAL_CAN_RxFifo1MsgPendingCallback
 * @params 	:	handle to can
 * @returns :	if cb is not define msg is put in q
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef* hcan1)
{
  can_msg_t msg;
  CanErrorCode = HAL_CAN_ERROR_NONE; // reset ErrorCode
  HAL_CAN_GetRxMessage(hcan1, CAN_RX_FIFO1, &msg.RxHeader, &msg.data[0]);
  if (CanRxCB)
    CanRxCB(&msg);
  else
    xQueueSendFromISR(_can_driver_rx_queue, &msg, &xHigherPriorityTaskWoken); // put in Q
}

/*!
 * @fn 		:	CheckCANNewMsg
 * @params 	:	none
 * @returns :	none
 * @brief	: 	Checks Q for new msg, and send them. Takes Hi priority msg first
 */
void CheckCANNewMsg()
{
  can_msg_t msg;
  if (xQueueReceive(_can_driver_tx0_queue, (void*)&msg, 0) == pdTRUE)
  { // check if there is something in the buffer
    HAL_CAN_AddTxMessage(&hcan, &msg.TxHeader, &msg.data[0], &TxMailbox);
    send_out_count++;
  }
  else if (xQueueReceive(_can_driver_tx1_queue, (void*)&msg, 0) == pdTRUE)
  { // check if there is something in the buffer
    HAL_CAN_AddTxMessage(&hcan, &msg.TxHeader, &msg.data[0], &TxMailbox);
    send_out_count++;
  }
}

/*!
 * @fn 		:	HAL_CAN_TxMailbox0CompleteCallback
 * @params 	:	handle to can
 * @returns :	none
 */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan1)
{
  CanErrorCode = HAL_CAN_ERROR_NONE; // reset ErrorCode
  if (TxComplete)
    TxComplete(hcan1);
}

/*!
 * @fn 		:	HAL_CAN_TxMailbox1CompleteCallback
 * @params 	:	handle to can
 * @returns :	none
 */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan1)
{
  CanErrorCode = HAL_CAN_ERROR_NONE; // reset ErrorCode
  if (TxComplete)
    TxComplete(hcan1);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan1)
{
  CanErrorCode = HAL_CAN_ERROR_NONE; // reset ErrorCode
  if (TxComplete)
    TxComplete(hcan1);
}

/*!
 * @fn 		:	HAL_CAN_ErrorCallback
 * @params 	:	CAN_HandleTypeDef CAN handle
 * @returns :	None
 * Registers that the interface is in an error state. Accept the error and continue.
 * HAL_CAN_IRQHandler() function : This routine is called when a CAN frame Rx/Tx related interrupt is set.
 * In this function, we see that the CAN FIFO overrun error flag is set (FOV) which happens when the number of CAN frames
 * received exceeds the FIFO capacity (defaults to three frames). When such errors happen we need to enable CAN receive (Rx)
 * interrupt flag, otherwise the reception stops once the error-flag is set. Best way would be to make use of the
 *  HAL_CAN_ErrorCallback() weak-handler function where we can clear the error-flag and enable the CAN Rx interrupt.
 */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* canHandle)
{
  CanErrorCode = canHandle->ErrorCode;
  canHandle->ErrorCode = HAL_CAN_ERROR_NONE; // reset errorscodes
  /*	xQueueReset(_can_driver_tx0_queue);      //clear CAN Q
    xQueueReset(_can_driver_tx1_queue);
    if (_can_driver_rx_queue)
      xQueueReset(_can_driver_rx_queue);*/
}

//----------------------------------------------------------------------------------------------------------------
// Util
//----------------------------------------------------------------------------------------------------------------
/*!
 * @fn 		:	MsgVarId
 * @params 	:	can_msg_t struct with CAN msg
 * @returns :	VarID
 */
uint16_t MsgVarId(can_msg_t* msg)
{
  return (((uint16_t)(msg->data[2] & 0x0F)) << 8) | msg->data[3];
}

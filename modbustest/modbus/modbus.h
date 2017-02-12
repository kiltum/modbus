/**
Modbus implementation for STM32 HAL under FreeRTOS.
**/
#ifndef __modbus_H
#define __modbus_H
#ifdef __cplusplus
 extern "C" {
#endif
#include "FreeRTOS.h"
// 3,5 character timeout or frame compaletion timeout
#define ModBus35 35

     // message queue for incoming bytes
     extern osMessageQId ModBusInHandle;
     // and for outgoing bytes
     extern osMessageQId ModBusOutHandle;
     // just start it before scheduler
	 void ModBus_Init(void);
   // set our address
   void ModBus_SetAddress(uint8_t addr);

#ifdef __cplusplus
}
#endif
#endif

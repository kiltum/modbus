/**
Modbus implementation for STM32 HAL under FreeRTOS.
**/
#ifndef __modbus_H
#define __modbus_H
#ifdef __cplusplus
 extern "C" {
#endif
     // message queue for incoming bytes
     extern osMessageQId ModBusInHandle;
     // and for outgoing bytes
     extern osMessageQId ModBusOutHandle;
     // just start it before scheduler
	 void ModBus_Init(void);

#ifdef __cplusplus
}
#endif
#endif

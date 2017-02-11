#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "modbus.h"

osMessageQId ModBusInHandle;
osMessageQId ModBusOutHandle;
osThreadId ModBusTaskHandle;

void ModBusTask(void const * argument)
{
  for(;;)
  {
    osDelay(200);
		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8); 
  }
}

void ModBus_Init(void)
{
  osMessageQDef(ModBusIn, 256, uint8_t);
  ModBusInHandle = osMessageCreate(osMessageQ(ModBusIn), NULL);
  osMessageQDef(ModBusOut, 256, uint8_t);
  ModBusOutHandle = osMessageCreate(osMessageQ(ModBusOut), NULL);
  osThreadDef(ModBusTask, ModBusTask, osPriorityNormal, 0, 128);
  ModBusTaskHandle = osThreadCreate(osThread(ModBusTask), NULL);
}

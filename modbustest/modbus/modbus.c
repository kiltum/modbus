/**
Modbus slave implementation for STM32 HAL under FreeRTOS.
(c) 2017 Viacheslav Kaloshin, multik@multik.org
Licensed under LGPL. 
**/

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "modbus.h"

// If you want directly send to usb-cdc
// #include "usbd_cdc_if.h"

osMessageQId ModBusInHandle;
osMessageQId ModBusOutHandle;
osThreadId ModBusTaskHandle;

uint16_t mb_reg[ModBusRegisters];

// Here is actual modbus data stores
uint8_t mb_buf_in[256];
uint8_t mb_buf_in_count;
uint8_t mb_addr;
uint8_t mb_buf_out[256];
uint8_t mb_buf_out_count;

void ModBusParse(void);

void ModBusTask(void const * argument)
{
  for(;;)
  {
    osEvent evt = osMessageGet(ModBusInHandle,ModBus35);
    // Frame end?
    if (evt.status == osEventTimeout)
      {
        if(mb_buf_in_count > 0) // ok, something in buffer exist, lets parse it
        {
          ModBusParse();
        }  
      mb_buf_in_count=0;
      }
    // Wow, something come!
    if (evt.status == osEventMessage)
      {
        uint8_t byte = (uint8_t) evt.value.v;
        // buffer has space for incoming?
        if(mb_buf_in_count<254)
        {
          mb_buf_in[mb_buf_in_count]=byte;
          mb_buf_in_count=mb_buf_in_count+1; // prevent opt/war on come compilers
        }
        else // oops, bad frame, by standard we should drop it and no answer
        {
          mb_buf_in_count=0;
        }
      }
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
  mb_buf_in_count=0;
  mb_addr=247; // by default maximum possible adrress
  mb_buf_out_count=0;
  for(int i=0;i<ModBusRegisters;i++) 
  {
    mb_reg[i]=0;
  }
}

void ModBus_SetAddress(uint8_t addr)
{
  mb_addr = addr;
}

void CRC16_OUT(void);
uint8_t CRC16_IN(void);

// parse something in incoming buffer 
void ModBusParse(void)
{
    if(mb_buf_in_count==0) // call as by mistake on empty buffer?
    {
      return;
    }
    
    if(mb_buf_in[0] != mb_addr) // its not our address!
    {
      return;
    }
    // check CRC
    if(CRC16_IN()==0)
    {
      mb_buf_out_count = 0;
      uint16_t st,nu;
      uint8_t func = mb_buf_in[1];
      uint8_t i;
      switch(func)
      {
        case 3:
          // read holding registers. by bytes addr func starth startl totalh totall
          st=mb_buf_in[2]*256+mb_buf_in[3];
          nu=mb_buf_in[4]*256+mb_buf_in[5];
          if( (st+nu) > ModBusRegisters) // dont ask more, that we has!
            {
              mb_buf_out[mb_buf_out_count++]=mb_addr;
              mb_buf_out[mb_buf_out_count++]=func+0x80;
              mb_buf_out[mb_buf_out_count++]=2;
            }
            else
            {
              mb_buf_out[mb_buf_out_count++]=mb_addr;
              mb_buf_out[mb_buf_out_count++]=func;
              mb_buf_out[mb_buf_out_count++]=(st+nu)*2; // how many bytes we will send?
              for(i=st;i<nu;i++)
                {
                  mb_buf_out[mb_buf_out_count++]=( mb_reg[i] >> 8 ) & 0xFF; // hi part
                  mb_buf_out[mb_buf_out_count++]=mb_reg[i] & 0xFF; // lo part
                }
            }
          break;
        case 16: 
          // write holding registers. by bytes addr func starth startl totalh totall num_bytes regh regl ...
          st=mb_buf_in[2]*256+mb_buf_in[3];
          nu=mb_buf_in[4]*256+mb_buf_in[5];
          if( (st+nu) > ModBusRegisters) // dont ask more, that we has!
            {
              mb_buf_out[mb_buf_out_count++]=mb_addr;
              mb_buf_out[mb_buf_out_count++]=func+0x80;
              mb_buf_out[mb_buf_out_count++]=2;
            }
            else
              { // ATTN : skip num_bytes
              for(i=st;i<nu;i++)
                {
                  mb_reg[i]=mb_buf_in[7+i*2]*256+mb_buf_in[8+i*2];
                }
              mb_buf_out[mb_buf_out_count++]=mb_addr;
              mb_buf_out[mb_buf_out_count++]=func;
              mb_buf_out[mb_buf_out_count++]=mb_buf_in[2]; // how many registers ask, so many wrote
              mb_buf_out[mb_buf_out_count++]=mb_buf_in[3];
              mb_buf_out[mb_buf_out_count++]=mb_buf_in[4];
              mb_buf_out[mb_buf_out_count++]=mb_buf_in[5];
            }
          break;
        default:  
          // Exception as we does not provide this function
          mb_buf_out[mb_buf_out_count++]=mb_addr;
          mb_buf_out[mb_buf_out_count++]=func+0x80;
          mb_buf_out[mb_buf_out_count++]=1;
          break;
      }
      
      CRC16_OUT();
      
     // If you want directly to USB-CDC 
     //CDC_Transmit_FS(&mb_buf_out[0], mb_buf_out_count);
     for(int i=0;i<mb_buf_out_count;i++)
        {
          osMessagePut(ModBusOutHandle,mb_buf_out[i],0);
        }
    }
    // Ok, we parsed buffer, clean up
    mb_buf_in_count=0;
    mb_buf_out_count=0;
}

// set value of register
void ModBus_SetRegister(uint8_t reg,uint16_t value)
{
  if(reg<ModBusRegisters)
  {
    mb_reg[reg]=value;
  }
}
// grab value of register
uint16_t ModBus_GetRegister(uint8_t reg)
{
  if(reg<ModBusRegisters)
  {
    return mb_reg[reg];
  }
  return 0;
}


// Calculate CRC for outcoming buffer
// and place it to end.
void CRC16_OUT(void)
{
  uint16_t crc = 0xFFFF;
  uint16_t pos = 0;
  uint8_t i =0;
  uint8_t lo =0;
  uint8_t hi =0;
  
  for (pos = 0; pos < mb_buf_out_count; pos++)
  {
    crc ^= mb_buf_out[pos];

  for (i = 8; i != 0; i--)
    {
    if ((crc & 0x0001) != 0)
      {
      crc >>= 1;
      crc ^= 0xA001;
      }
    else
      crc >>= 1;
    }
  }
  lo = crc & 0xFF;
  hi = ( crc >> 8 ) & 0xFF;
  
  mb_buf_out[mb_buf_out_count++] = lo;
  mb_buf_out[mb_buf_out_count++] = hi;
}

// Calculate CRC fro incoming buffer
// Return 0 - if CRC is correct, overwise return 0 
uint8_t CRC16_IN(void)
{
  uint16_t crc = 0xFFFF;
  uint16_t pos = 0;
  uint8_t i =0;
  uint8_t lo =0;
  uint8_t hi =0;
  
  for (pos = 0; pos < mb_buf_in_count-2; pos++)
  {
    crc ^= mb_buf_in[pos];

  for (i = 8; i != 0; i--)
    {
    if ((crc & 0x0001) != 0)
      {
      crc >>= 1;
      crc ^= 0xA001;
      }
    else
      crc >>= 1;
    }
  }
  lo = crc & 0xFF;
  hi = ( crc >> 8 ) & 0xFF;
  if( (mb_buf_in[mb_buf_in_count-2] == lo) && 
       (mb_buf_in[mb_buf_in_count-1] == hi) )
    {
      return 0;
    }
  return 1;
}

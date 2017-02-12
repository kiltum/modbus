# Simple MODBUS implementation for STM32 HAL & FreeRTOS

Why I write my own implementation? 
- It does not need any timers. 
- It need only one thread on normal (or the same with interface) priority. 
- It can be easy adopted for USB/usart/whatever interfaces.
- It is for fun of course!

# How to USE

* Grab modbus.c and modbus.h to your project
* Fix values in modbus.h for you. 
* Call init functions and set slave address
```
ModBus_Init();
ModBus_SetAddress(1);
```
* Handle incoming bytes to ModBusInHandle queue (example from USB-CDC)
```
USBD_CDC_ReceivePacket(&hUsbDeviceFS);
...
for(int i=0;i<(*Len);i++)
  {
  osMessagePut(ModBusInHandle,Buf[i],0);
  }
...
return (USBD_OK);
```
* Handle outgoing bytes the same way, but from ModBusOutHandle queue
```
uint8_t buf[256]; // buffer, where we collect output data
uint8_t c = 0; // counter for buffer fill
  
for(;;)
{
osEvent evt = osMessageGet(ModBusOutHandle,200); // wait here 200 tick
if (evt.status == osEventMessage)
  {
    buf[c++]=(uint8_t) evt.value.v;
  }
if (evt.status == osEventTimeout)
  {
    if( (c > 0) && (c < 254) ) // ok, something in buffer exist, lets send it
    {
      CDC_Transmit_FS(&buf[0], c); // by USB-CDC         
    }  
  c=0;
  }
```
* In other part code, call ModBus_GetRegister & ModBus_SetRegister for reading & setting register
```
count = ModBus_GetRegister(0);
ModBus_SetRegister(0,count+1); 
```
## How to check?

I prefer to use ModPoll (http://www.modbusdriver.com/modpoll.html)

So in my case all look like this. Here register 0 & 1 overwritten inside programm for test, and modbus slave on COM5
```
C:\Users\multi\Downloads\modpoll.3.4\win32>modpoll.exe -a 1 -r 1 -c 5  COM5 100 200 310 400 500
modpoll 3.4 - FieldTalk(tm) Modbus(R) Master Simulator
Copyright (c) 2002-2013 proconX Pty Ltd
Visit http://www.modbusdriver.com for Modbus libraries and tools.

Protocol configuration: Modbus RTU
Slave configuration...: address = 1, start reference = 1, count = 5
Communication.........: COM5, 19200, 8, 1, even, t/o 1.00 s, poll rate 1000 ms
Data type.............: 16-bit register, output (holding) register table

Written 5 references.

C:\Users\multi\Downloads\modpoll.3.4\win32>modpoll.exe -a 1 -r 1 -c 6 COM5
modpoll 3.4 - FieldTalk(tm) Modbus(R) Master Simulator
Copyright (c) 2002-2013 proconX Pty Ltd
Visit http://www.modbusdriver.com for Modbus libraries and tools.

Protocol configuration: Modbus RTU
Slave configuration...: address = 1, start reference = 1, count = 6
Communication.........: COM5, 19200, 8, 1, even, t/o 1.00 s, poll rate 1000 ms
Data type.............: 16-bit register, output (holding) register table

-- Polling slave... (Ctrl-C to stop)
[1]: 118
[2]: 0
[3]: 310
[4]: 400
[5]: 500
[6]: 0
-- Polling slave... (Ctrl-C to stop)
[1]: 140
[2]: 0
[3]: 310
[4]: 400
[5]: 500
[6]: 0
-- Polling slave... (Ctrl-C to stop)
```

## You are too lazy?

In modbustest directory you find full working example.

I use:

- STM32F3-Discovery board
- Keil uVision 5.22
- STM32CubeMX

#Have fun!#
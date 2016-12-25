/* ===================================================================
Copyright © 2016, AVNET Inc.  

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, 
software distributed under the License is distributed on an 
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
either express or implied. See the License for the specific 
language governing permissions and limitations under the License.

======================================================================== */

#ifndef Hardware_H_
#define Hardware_H_
#include "MODSERIAL.h"

extern I2C i2c;    //SDA, SCL -- define the I2C pins being used
extern MODSERIAL pc; //UART to USB host
//extern SerialBuffered mdm; //UART to WNC modem

//Un-comment the definition below if you want to use the USB rx for another purpose.
//otherwise the USB rx will be used to receive virtual sensor data from Avnet's
//"Sensor Simulator Dashboard"  utility
//#define USE_VIRTUAL_SENSORS

// comment out the following line if color is not supported on the terminal
#define USE_COLOR
#ifdef USE_COLOR
 #define BLK "\033[30m"
 #define RED "\033[31m"
 #define GRN "\033[32m"
 #define YEL "\033[33m"
 #define BLU "\033[34m"
 #define MAG "\033[35m"
 #define CYN "\033[36m"
 #define WHT "\033[37m"
 #define DEF "\033[39m"
#else
 #define BLK
 #define RED
 #define GRN
 #define YEL
 #define BLU
 #define MAG
 #define CYN
 #define WHT
 #define DEF
#endif

#ifdef _ULINK_PRINT
#include "itm_output.h"
#else
#define PRINTF pc.printf
#define PUTS   pc.puts
#endif


#endif

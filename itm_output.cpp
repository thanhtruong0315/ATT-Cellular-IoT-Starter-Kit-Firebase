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

//Used for ULINK output only
//

/* ITM registers */
#define ITM_PORT0_U8          (*((volatile unsigned int  *)0xE0000000))
#define ITM_PORT0_U32         (*((volatile unsigned long *)0xE0000000))
#define ITM_TER               (*((volatile unsigned long *)0xE0000E00))
#define ITM_TCR               (*((volatile unsigned long *)0xE0000E80))

#define ITM_TCR_ITMENA_Msk    (1UL << 0)

/*!< Value identifying \ref ITM_RxBuffer is ready for next character. */
#define ITM_RXBUFFER_EMPTY    0x5AA55AA5

/*!< Variable to receive characters. */
extern
volatile int ITM_RxBuffer;
volatile int ITM_RxBuffer = ITM_RXBUFFER_EMPTY;

/** \brief  ITM Send Character

    The function transmits a character via the ITM channel 0, and
    \li Just returns when no debugger is connected that has booked the output.
    \li Is blocking when a debugger is connected, but the previous character
        sent has not been transmitted.

    \param [in]     ch  Character to transmit.

    \returns            Character to transmit.
 */
int ITM_putc (int ch) {
  if ((ITM_TCR & ITM_TCR_ITMENA_Msk) && /* ITM enabled */
      (ITM_TER & (1UL << 0)        )) { /* ITM Port #0 enabled */
    while (ITM_PORT0_U32 == 0);
    ITM_PORT0_U8 = (int)ch;
  }
  return (ch);
}

/** \brief  ITM Receive Character

    The function inputs a character via the external variable \ref ITM_RxBuffer.
    This variable is monitored and altered by the debugger to provide input.

    \return             Received character.
    \return         -1  No character pending.
 */
int ITM_getc (void) {
  int ch = -1;                      /* no character available */

  if (ITM_RxBuffer != ITM_RXBUFFER_EMPTY) {
    ch = ITM_RxBuffer;
    ITM_RxBuffer = ITM_RXBUFFER_EMPTY;  /* ready for next character */
  }

  return (ch);
}

/** \brief  ITM send string

    The function sends a null terminated string via the external variable \ref ITM_RxBuffer.
    This variable is monitored and altered by the debugger to provide input.

    \return             Received character.
    \return         -1  No character pending.
 */
int ITM_puts (char * str) {
  int i=0;

  while(str[i])
    ITM_putc(str[i++]);
  return i;
}


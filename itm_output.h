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

#ifndef __ITM_OUTPUT__
#define __ITM_OUTPUT__

//This file is only used when building for ULINK output

#define PRINTF(format, ...)     { char xyz[80]; sprintf (xyz, format, ## __VA_ARGS__); ITM_puts(xyz);}
#define PUTS(st)                                                        ITM_puts((char*)st);


int ITM_putc (int ch);
int ITM_getc (void);
int ITM_puts ((char *) str);

#endif

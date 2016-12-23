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

#ifndef __WNC_CONTROL_H_
#define __WNC_CONTROL_H_

static const unsigned WNC_TIMEOUT_MS = 10000;

// Core function that sends data to the WNC UART
extern int send_wnc_cmd(const char * s, string ** r, int ms_timeout);

// Low level command functions
extern void at_init_wnc(bool hardReset = false);
extern void at_sockopen_wnc(const string & ipStr, const char * port );
extern void at_sockclose_wnc(void);
extern int at_dnsresolve_wnc(const char * s, string * ipStr);
extern void at_sockwrite_wnc(const char * s);
extern unsigned at_sockread_wnc(string * pS, unsigned n, unsigned retries);
extern void at_at_wnc(void);
extern int at_send_wnc_cmd(const char * s, string ** r, int ms_timeout);
extern int check_wnc_ready(void);

// High level functions that attempt to correct for things going bad with the WNC
extern void software_init_mdm(void);
extern void resolve_mdm(void);
extern void sockopen_mdm(void);
extern void sockwrite_mdm(const char * s);
extern unsigned sockread_mdm(string * sockData, int len, int retries);
extern void sockclose_mdm(void);
extern void display_modem_firmware_version(void);

#endif



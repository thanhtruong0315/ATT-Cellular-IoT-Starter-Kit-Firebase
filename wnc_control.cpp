/* ===================================================================
Copyright c 2016, AVNET Inc.  

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

#include "mbed.h"
#include <cctype>
#include <string>
#include "config_me.h"
#include "wnc_control.h"
#include "hardware.h"

// Outputs detailed WNC command info
#define WNC_CMD_DEBUG_ON

// Full debug output, longer cmds and extra cellular status checking
#undef WNC_CMD_DEBUG_ON_VERBOSE

extern string MyServerIpAddress;
extern string MySocketData;

int reinitialize_mdm(void);

enum WNC_ERR_e {
    WNC_OK =0,
    WNC_CMD_ERR = -1,
    WNC_NO_RESPONSE = -2,
    WNC_CELL_LINK_DOWN = -3,
    WNC_EXTERR = -4
};

// Contains result of last call to send_wnc_cmd(..)
WNC_ERR_e WNC_MDM_ERR = WNC_OK;

// Contains the RAW WNC UART responses
static string wncStr;
static int socketOpen = 0;

void software_init_mdm(void)
{
 // Temp put here to fix new boards needing init,
 //  the check for on the cellular network was preventing the PDNSET from happening!!!!
 {  
    PUTS("SET APN STRING!\r\n");
    string * pRespStr;
    string cmd_str("AT%PDNSET=1,");
    cmd_str += MY_APN_STR;
    cmd_str += ",IP";
    at_send_wnc_cmd(cmd_str.c_str(), &pRespStr, 4*WNC_TIMEOUT_MS); // Set APN, cmd seems to take a little longer sometimes
 }   

  static bool reportStatus = true;
  do
  {
      PUTS("------------ software_init_mdm! --------->\r\n");
      if (check_wnc_ready() == 0)
      {
          if (reportStatus == false)
          {
              PUTS("Re-connected to cellular network!\n\r");
              reportStatus = true;
          }
    
          // WNC has SIM and registered on network
          do
          {
              WNC_MDM_ERR = WNC_OK;
              at_init_wnc();
              if (WNC_MDM_ERR == WNC_NO_RESPONSE)
              {
                  reinitialize_mdm();
                  at_init_wnc(true);  // Hard reset occurred so make it go through the software init();
              }
          } while (WNC_MDM_ERR != WNC_OK);
      }
      else
      {
          if (reportStatus == true)
          {
               PUTS("Not connected to cellular network!\n\r");
               reportStatus = false;
          }
     // Atempt to re-register
//     string * pRespStr;
//     PUTS("Force re-register!\r\n");
//     at_send_wnc_cmd("AT+CFUN=0,0", &pRespStr, WNC_TIMEOUT_MS);
//     wait_ms(31000);
//     at_send_wnc_cmd("AT+CFUN=1,0", &pRespStr, WNC_TIMEOUT_MS);
//     wait_ms(31000);
          WNC_MDM_ERR = WNC_CELL_LINK_DOWN;
      }
  } while (WNC_MDM_ERR != WNC_OK);
}

void display_modem_firmware_version(void)
{
    string * pRespStr;

    PUTS("<-------- WNC Firmware Revision --------\r\n");
    at_send_wnc_cmd("ATI", &pRespStr, WNC_TIMEOUT_MS);
    PUTS(pRespStr->c_str());
    PUTS("\r\n");    
    PUTS("--------------------------------------->\r\n");
}

void resolve_mdm(void)
{
    do
    {
      WNC_MDM_ERR = WNC_OK;
      at_dnsresolve_wnc(MY_SERVER_URL, &MyServerIpAddress);
      if (WNC_MDM_ERR == WNC_NO_RESPONSE)
      {
        software_init_mdm();
      }
      else if (WNC_MDM_ERR == WNC_CMD_ERR)
      {
        PUTS("Bad URL!!!!!!\r\n");
      }
    } while (WNC_MDM_ERR != WNC_OK);
    
    PRINTF("My Server IP: %s\r\n", MyServerIpAddress.c_str());
}

void sockopen_mdm(void)
{
    do
    {
      WNC_MDM_ERR = WNC_OK;
      at_sockopen_wnc(MyServerIpAddress, MY_PORT_STR);
      if (WNC_MDM_ERR == WNC_NO_RESPONSE)
      {
        software_init_mdm();
      }
      else if (WNC_MDM_ERR == WNC_CMD_ERR)
        PUTS("Socket open fail!!!!\r\n");
      else
        socketOpen = 1;
    } while (WNC_MDM_ERR != WNC_OK);
}

void sockwrite_mdm(const char * s)
{
    if (socketOpen == 1)
    {
    do
    {
      WNC_MDM_ERR = WNC_OK;
      at_sockwrite_wnc(s);
      if (WNC_MDM_ERR == WNC_NO_RESPONSE)
      {
        PUTS("Sock write no response!\r\n");
        software_init_mdm();
      }
      else if (WNC_MDM_ERR == WNC_CMD_ERR)
      {
        PUTS("Socket Write fail!!!\r\n");
        software_init_mdm();
      }else if (WNC_MDM_ERR == WNC_EXTERR)
      {
        PUTS("Socket Disconnected (broken) !!!\r\n");
        sockclose_mdm();
        sockopen_mdm();
        //software_init_mdm();
      }
    } while (WNC_MDM_ERR != WNC_OK);
    }
    else
      PUTS("Socket is closed for write!\r\n");
}

unsigned sockread_mdm(string * sockData, int len, int retries)
{
    unsigned n = 0;
    
    if (socketOpen == 1)
    {
    do
    {
      WNC_MDM_ERR = WNC_OK;
      n = at_sockread_wnc(sockData, len, retries);
      if (WNC_MDM_ERR == WNC_NO_RESPONSE)
      {
        if (n == 0)
            software_init_mdm();
        else
            PUTS("Sock read partial data!!!\r\n");
      }
      else if (WNC_MDM_ERR == WNC_CMD_ERR)
        PUTS("Sock read fail!!!!\r\n");
    } while (WNC_MDM_ERR == WNC_NO_RESPONSE);
    }
    else
    {
      PUTS("Socket is closed for read\r\n");
      sockData->erase();
    }
      
    return (n);
}

void sockclose_mdm(void)
{
    do
    {
      WNC_MDM_ERR = WNC_OK;
      at_sockclose_wnc();
      // Assume close happened even if it went bad
      // going bad will result in a re-init anyways and if close
      // fails we're pretty much in bad state and not much can do
      socketOpen = 0;
      if (WNC_MDM_ERR == WNC_NO_RESPONSE)
      {
        software_init_mdm();
      }
      else if (WNC_MDM_ERR == WNC_CMD_ERR)
        PUTS("Sock close fail!!!\r\n");
    } while (WNC_MDM_ERR != WNC_OK);
}

/**                                                                                                                                                          
 * C++ version 0.4 char* style "itoa":                                                                                                                       
 * Written by Lukas Chmela                                                                                                                                   
 * Released under GPLv3.                                                                                                                                     
*/
 
char* itoa(int value, char* result, int base)                                                                                                          
{                                                                                                                                                        
    // check that the base if valid                                                                                                                      
    if ( base < 2 || base > 36 ) {                                                                                                                       
        *result = '\0';                                                                                                                                  
        return result;                                                                                                                                   
    }                                                                                                                                                    
 
    char* ptr = result, *ptr1 = result, tmp_char;                                                                                                        
    int tmp_value;                                                                                                                                       
 
    do {                                                                                                                                                 
        tmp_value = value;                                                                                                                               
        value /= base;                                                                                                                                   
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];                             
    } while ( value );                                                                                                                                   
 
    // Apply negative sign                                                                                                                               
    if ( tmp_value < 0 )                                                                                                                                 
    *ptr++ = '-';                                                                                                                                    
    *ptr-- = '\0';                                                                                                                                       
 
    while ( ptr1 < ptr ) {                                                                                                                               
    tmp_char = *ptr;                                                                                                                                 
    *ptr-- = *ptr1;                                                                                                                                  
    *ptr1++ = tmp_char;                                                                                                                              
    }                                                                                                                                                    
 
    return result;                                                                                                                                       
}

extern int mdm_sendAtCmdRsp(const char *cmd, const char **rsp_list, int timeout_ms, string * rsp, int * len);

int check_wnc_ready(void)
{
    string * pRespStr;
    size_t pos;
    int regSts;
    int cmdRes1, cmdRes2;

#ifdef WNC_CMD_DEBUG_ON_VERBOSE
    PUTS("<-------- Begin Cell Status ------------\r\n");
#endif
    cmdRes1 = at_send_wnc_cmd("AT+CSQ", &pRespStr, WNC_TIMEOUT_MS);       // Check RSSI,BER
    cmdRes2 = at_send_wnc_cmd("AT+CPIN?", &pRespStr, WNC_TIMEOUT_MS);      // Check if SIM locked
    
    if ((cmdRes1 != 0) && (cmdRes2 != 0))
    {
#ifdef WNC_CMD_DEBUG_ON_VERBOSE
        PUTS("------------ WNC No Response! --------->\r\n");
#endif
        return (-2);      
    }
  
    // If SIM Card not ready don't bother with commands!
    if (pRespStr->find("CPIN: READY") == string::npos)
    {
#ifdef WNC_CMD_DEBUG_ON_VERBOSE
        PUTS("------------ WNC SIM Problem! --------->\r\n");
#endif
        return (-1);
    }
  
    // SIM card OK, now check for signal and cellular network registration
    cmdRes1 = at_send_wnc_cmd("AT+CREG?", &pRespStr, WNC_TIMEOUT_MS);      // Check if registered on network
    if (pRespStr->size() > 0)
    {
    pos = pRespStr->find("CREG: ");
    if (pos != string::npos)
    {
        // The registration is the 2nd arg in the comma separated list
        *pRespStr = pRespStr->substr(pos+8, 1);
        regSts = atoi(pRespStr->c_str());
        // 1 - registered home, 5 - registered roaming
        if ((regSts != 1) && (regSts != 5))
        {
#ifdef WNC_CMD_DEBUG_ON_VERBOSE
            PUTS("------------ WNC Cell Link Down! ------>\r\n");
#endif
            return (-2);
        }
    }

#ifdef WNC_CMD_DEBUG_ON_VERBOSE
    PUTS("------------ WNC Ready ---------------->\r\n");
#endif
    }
    else
    {
#ifdef WNC_CMD_DEBUG_ON_VERBOSE
        PUTS("------------ CREG No Reply !----------->\r\n");
#endif
        return (-2);
    }

    return (0);
}

// Sets a global with failure or success, assumes 1 thread all the time
int send_wnc_cmd(const char * s, string ** r, int ms_timeout)
{
  int cmdRes;
  
  if (check_wnc_ready() < 0)
  {
     static string noRespStr;

#ifdef WNC_CMD_DEBUG_ON
     PUTS("FAIL send cmd: ");
     #ifdef WNC_CMD_DEBUG_ON_VERBOSE
     PUTS(s);
     PUTS("\r\n");
     #else
     string truncStr(s, 50);
     truncStr += "\r\n";
     PUTS(truncStr.c_str());
     #endif
#else
     PUTS("FAIL send cmd!\r\n");
#endif

     WNC_MDM_ERR = WNC_CELL_LINK_DOWN;
     noRespStr.erase();
     *r = &noRespStr;
     return (-3);
  }

#ifdef WNC_CMD_DEBUG_ON
  #ifdef WNC_CMD_DEBUG_ON_VERBOSE
  PUTS("[---------- Network Status -------------\r\n");
  #endif
  string * pRespStr;
  at_send_wnc_cmd("AT@SOCKDIAL?", &pRespStr, 5000);
  #ifdef WNC_CMD_DEBUG_ON_VERBOSE
  PUTS("---------------------------------------]\r\n");
  #endif
#endif

  // If WNC ready, send user command
  cmdRes = at_send_wnc_cmd(s, r, ms_timeout);
  
  if (cmdRes == -1)
     WNC_MDM_ERR = WNC_CMD_ERR;
  
  if (cmdRes == -2)
     WNC_MDM_ERR = WNC_NO_RESPONSE;
     
  if (cmdRes == -3) {
     WNC_MDM_ERR = WNC_EXTERR;
     PRINTF("[[WNC_MDM_ERR = WNC_EXTERR]] \r\n");
  }

 if (cmdRes == 0)
     WNC_MDM_ERR = WNC_OK;
  
  return (cmdRes);
}

int at_send_wnc_cmd(const char * s, string ** r, int ms_timeout)
{
   //Eaddy
  static const char * rsp_lst[] = { "OK", "ERROR","@EXTERR", "+CME", NULL };
  int len;

#ifdef WNC_CMD_DEBUG_ON
  #ifdef WNC_CMD_DEBUG_ON_VERBOSE
  
  #else
  if (strlen(s) > 60)
  {
      string truncStr(s,57);
      truncStr += "...";
      PRINTF("Send: <<%s>>\r\n",truncStr.c_str());
  }
  else
  #endif
      PRINTF("Send: <<%s>>\r\n",s);
#endif

  int res = mdm_sendAtCmdRsp(s, rsp_lst, ms_timeout, &wncStr, &len);
  *r = &wncStr;   // Return a pointer to the static string
      
  if (res >= 0)
  {

#ifdef WNC_CMD_DEBUG_ON   
      PUTS("[");
      #ifdef WNC_CMD_DEBUG_ON_VERBOSE
      PUTS(wncStr.c_str());
      PUTS("]\r\n");
      #else
      if (wncStr.size() < 51)
          PUTS(wncStr.c_str());
      else
      {
          string truncStr = wncStr.substr(0,50) + "...";
          PUTS(truncStr.c_str());
      }
      PUTS("]\r\n");
      #endif
#endif

#if 0
      if (res > 0)
          return -1;
      else
          return 0;
#else
    //Eaddy added         
       if (res == 0) {
          /* OK */
          return 0;
      } else if (res == 2) {
          /* @EXTERR */
          PRINTF("@EXTERR and res = %d \r\n", res);
          return -3;
      } else
          return -1;
#endif
  }
  else
  {
      PUTS("No response from WNC!\n\r");
      return -2;
  }
}


void at_at_wnc(void)
{
    string * pRespStr;
    send_wnc_cmd("AT", &pRespStr, WNC_TIMEOUT_MS); // Heartbeat?
}

void at_init_wnc(bool hardReset)
{
  static bool pdnSet = false;
  static bool intSet = false;
  static bool sockDialSet = false;  
  string * pRespStr;
  int cmdRes;
  
  if (hardReset == true)
  {
      PUTS("Hard Reset!\r\n");
      pdnSet = false;
      intSet = false;
      sockDialSet = false;
  }
  
  PUTS("Start AT init of WNC:\r\n");
  // Quick commands below do not need to check cellular connectivity
  cmdRes = at_send_wnc_cmd("AT", &pRespStr, WNC_TIMEOUT_MS);             // Heartbeat?
  cmdRes += at_send_wnc_cmd("ATE0", &pRespStr, WNC_TIMEOUT_MS);           // Echo Off
  cmdRes += at_send_wnc_cmd("AT+CMEE=2", &pRespStr, WNC_TIMEOUT_MS);      // 2 - verbose error, 1 - numeric error, 0 - just ERROR
  
  // If the simple commands are not working no chance of more complex.
  //  I have seen re-trying commands make it worse.
  if (cmdRes < 0)
  {
      // Since I used the at_send_wnc_cmd I am setting the error state based upon
      //  the responses.  And since these are simple commands, even if the WNC
      //  is saying ERROR, treat it like a no response.
      WNC_MDM_ERR = WNC_NO_RESPONSE;
      return ;
  }
  
  if (intSet == false)
    cmdRes = send_wnc_cmd("AT@INTERNET=1", &pRespStr, WNC_TIMEOUT_MS);
  
  if (cmdRes == 0)
    intSet = true;
  else
    return ;
  
  if (pdnSet == false)
  {
    string cmd_str("AT%PDNSET=1,");
    cmd_str += MY_APN_STR;
    cmd_str += ",IP";
    cmdRes = send_wnc_cmd(cmd_str.c_str(), &pRespStr, 4*WNC_TIMEOUT_MS); // Set APN, cmd seems to take a little longer sometimes
  }
  
  if (cmdRes == 0)
    pdnSet = true;
  else
    return ;
  
  if (sockDialSet == false)
    cmdRes = send_wnc_cmd("AT@SOCKDIAL=1", &pRespStr, WNC_TIMEOUT_MS);
  
  if (cmdRes == 0)
    sockDialSet = true;
  else
    return ;
  
  PUTS("SUCCESS: AT init of WNC!\r\n");
}

void at_sockopen_wnc(const string & ipStr, const char * port )
{
  string * pRespStr;
  send_wnc_cmd("AT@SOCKCREAT=1", &pRespStr, WNC_TIMEOUT_MS);
  string cmd_str("AT@SOCKCONN=1,\"");
  cmd_str += ipStr;
  cmd_str += "\",";
  cmd_str += port;
  cmd_str += ",30";
  int cmd = send_wnc_cmd(cmd_str.c_str(), &pRespStr, 31000);
  if (cmd != WNC_OK) {
      // Per WNC: re-close even if open fails!
      at_sockclose_wnc();
  }
}

void at_sockclose_wnc(void)
{
  string * pRespStr;
  send_wnc_cmd("AT@SOCKCLOSE=1", &pRespStr, WNC_TIMEOUT_MS);
}

int at_dnsresolve_wnc(const char * s, string * ipStr)
{
  string * pRespStr;
  string str(s);
  str = "AT@DNSRESVDON=\"" + str + "\"";
  if (send_wnc_cmd(str.c_str(), &pRespStr, 60000) == 0)
  {
    size_t pos_start = pRespStr->find(":\"") + 2;
    size_t pos_end = pRespStr->find("\"", pos_start) - 1;
    if ((pos_start !=  string::npos) && (pos_end != string::npos))
    {
        if (pos_end > pos_start)
        {
          // Make a copy for use later (the source string is re-used)
          *ipStr = pRespStr->substr(pos_start, pos_end - pos_start + 1);
          return 1;
        }
        else
          PUTS("URL Resolve fail, substr Err\r\n");
    }
    else
      PUTS("URL Resolve fail, no quotes\r\n");
  }
  else
    PUTS("URL Resolve fail, WNC cmd fail\r\n");
  
  *ipStr = "192.168.0.1";
  
  return -1;
}

void at_sockwrite_wnc(const char * s)
{
  string * pRespStr;
  char num2str[6];
  size_t sLen = strlen(s);
  int res;
  if (sLen <= 1500)
  {
    string cmd_str("AT@SOCKWRITE=1,");
    itoa(sLen, num2str, 10);
    cmd_str += num2str;
    cmd_str += ",\"";
    while(*s != '\0')
    {
      itoa((int)*s++, num2str, 16);
      // Always 2-digit ascii hex:
      if (strlen(num2str) == 1)
      {
        num2str[2] = '\0';
        num2str[1] = num2str[0];
        num2str[0] = '0';
      }
      cmd_str += num2str;
    }
    cmd_str += "\"";
    res = send_wnc_cmd(cmd_str.c_str(), &pRespStr, 120000);
    if (res == -3)
        PUTS("sockwrite is disconnect \r\n");
  }
  else
    PUTS("sockwrite Err, string to long\r\n");
}

unsigned at_sockread_wnc(string * pS, unsigned n, unsigned retries = 0)
{
  unsigned i, numBytes = 0;
  string * pRespStr;
  string cmd_str("AT@SOCKREAD=1,");

  // Clean slate
  pS->erase();
  
  if (n <= 1500)
  {
    char num2str[6];
    
    itoa(n, num2str, 10);
    cmd_str += num2str;
    retries += 1;
    while (retries--)
    {
      // Assuming someone is sending then calling this to receive response, invoke
      // a pause to give the response some time to come back and then also
      // between each retry.
      wait_ms(10);
      
      if (send_wnc_cmd(cmd_str.c_str(), &pRespStr, WNC_TIMEOUT_MS) == 0)
      {
        size_t pos_start = pRespStr->find("\"")  + 1;
        size_t pos_end   = pRespStr->rfind("\"") - 1;
      
        // Make sure search finds what it's looking for!
        if (pos_start != string::npos && pos_end != string::npos)
          i = (pos_end - pos_start + 1);  // Num hex chars, 2 per byte
        else
          i = 0;
          
        if (i > 0)
        {
            retries = 1;  // If any data found retry 1 more time to catch data that might be in another
                          //  WNC payload
            string byte;
            while (pos_start < pos_end)
            {
              byte = pRespStr->substr(pos_start, 2);
              *pS += (char)strtol(byte.c_str(), NULL, 16);
              pos_start += 2;
            }
            numBytes += i/2;
        }
      }
      else
      {
          PUTS("no readsock reply!\r\n");
          return (0);
      }
    }
  }
  else
    PUTS("sockread Err, to many to read\r\n");
  
  return (numBytes);
}

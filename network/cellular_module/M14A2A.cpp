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
#include "ATCommand.h"
#include "hardware.h"
#include "M14A2A.h"
const char ok_str[] = "OK";
const char error_str[] = "ERROR";

DigitalOut mdm_uart2_rx_boot_mode_sel(PTC17); // on powerup, 0 = boot mode, 1 = normal boot
DigitalOut mdm_power_on(PTB9);                // 0 = turn modem on, 1 = turn modem off (should be held high for >5 seconds to cycle modem)
DigitalOut mdm_wakeup_in(PTC2);               // 0 = let modem sleep, 1 = keep modem awake -- Note: pulled high on shield

DigitalOut mdm_reset(PTC12); // active high

DigitalOut shield_3v3_1v8_sig_trans_ena(PTC4); // 0 = disabled (all signals high impedence, 1 = translation active
DigitalOut mdm_uart1_cts(PTD0);

M14A2A::M14A2A(PinName tx, PinName rx, int baud)
{
  at_cmd = new ATCommand(tx, rx, baud);
}

int M14A2A::initCellularInternet(const char *apn, const char *username, const char *password)
{
  // Temp put here to fix new boards needing init,
  //  the check for on the cellular network was preventing the PDNSET from happening!!!!
  {
    PRINTF("BEGIN initCellularInternet\r\n\r\n");
    if (hw_init())
    {
      PRINTF("Cellular is running...\r\n");
    }
    else
    {
      PRINTF("Cellular is sleeping...\r\n");
      return 0;
    }
    const char *rsp_lst[] = {ok_str, error_str, NULL};
    string cmd_str("AT%PDNSET=1,");
    cmd_str += apn;
    cmd_str += ",IP";
    PRINTF("Send Command\r\n\r\n");

    at_cmd->sendCommand(cmd_str.c_str(), rsp_lst, 4 * WNC_TIMEOUT_MS); // Set APN, cmd seems to take a little longer sometimes
    PRINTF("End initCellularInternet\r\n\r\n");
  }

  /*static bool reportStatus = true;
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
          at_init_wnc(true); // Hard reset occurred so make it go through the software init();
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
  } while (WNC_MDM_ERR != WNC_OK);*/

  return 0;
}

bool M14A2A::hw_init()
{
  // Hard reset the modem (doesn't go through
  // the signal level translator)
  mdm_reset = 0;

  // disable signal level translator (necessary
  // for the modem to boot properly).  All signals
  // except mdm_reset go through the level translator
  // and have internal pull-up/down in the module. While
  // the level translator is disabled, these pins will
  // be in the correct state.
  shield_3v3_1v8_sig_trans_ena = 0;

  // While the level translator is disabled and ouptut pins
  // are tristated, make sure the inputs are in the same state
  // as the WNC Module pins so that when the level translator is
  // enabled, there are no differences.
  mdm_uart2_rx_boot_mode_sel = 1; // UART2_RX should be high
  mdm_power_on = 0;               // powr_on should be low
  mdm_wakeup_in = 1;              // wake-up should be high
  mdm_uart1_cts = 0;              // indicate that it is ok to send

  // Now, wait for the WNC Module to perform its initial boot correctly
  wait(1.0);

  // The WNC module initializes comms at 115200 8N1 so set it up
  // _at_serial->baud(115200);

  //Now, enable the level translator, the input pins should now be the
  //same as how the M14A module is driving them with internal pull ups/downs.
  //When enabled, there will be no changes in these 4 pins...
  shield_3v3_1v8_sig_trans_ena = 1;

  // Now, give the modem 60 seconds to start responding by
  // sending simple 'AT' commands to modem once per second.
  Timer timer;
  timer.start();
  while (timer.read() < 60)
  {
    const char *rsp_lst[] = {ok_str, error_str, NULL};
    int rc = at_cmd->sendCommand("AT", rsp_lst, 500);
    if (rc == 0)
      return true; //timer.read();
    wait_ms(1000 - (timer.read_ms() % 1000));
    PRINTF("\r%d", timer.read_ms() / 1000);
  }
  return false;
}

int M14A2A::isModuleReady(void)
{
  /*string * pRespStr;
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
    }*/

  return (0);
}
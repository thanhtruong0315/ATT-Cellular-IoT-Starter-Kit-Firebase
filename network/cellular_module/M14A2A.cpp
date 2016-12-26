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

enum WNC_ERR_e
{
  WNC_OK = 0,
  WNC_CMD_ERR = -1,
  WNC_NO_RESPONSE = -2,
  WNC_CELL_LINK_DOWN = -3,
  WNC_EXTERR = -4
};

WNC_ERR_e WNC_MDM_ERR = WNC_OK;

M14A2A::M14A2A(PinName tx, PinName rx, int baud)
{
  at_cmd = new ATCommand(tx, rx, baud);
}

void M14A2A::setAPN(const char *apn, const char *username, const char *password)
{
  this->apn = apn;
  this->username = username;
  this->password = password;
}

int M14A2A::connectInternet()
{
  // Temp put here to fix new boards needing init,
  //  the check for on the cellular network was preventing the PDNSET from happening!!!!
  {
    if (hw_init())
    {
#ifdef M14A2A_DEBUG
      PRINTF("Cellular is running...\r\n");
#endif
    }
    else
    {
#ifdef M14A2A_DEBUG
      PRINTF("Cellular is sleeping...\r\n");
#endif
      return 0;
    }
    const char *rsp_lst[] = {ok_str, error_str, NULL};
    string cmd_str("AT%PDNSET=1,");
    cmd_str += this->apn;
    cmd_str += ",IP";

    at_cmd->sendCommand(cmd_str.c_str(), rsp_lst, 4 * WNC_TIMEOUT_MS); // Set APN, cmd seems to take a little longer sometimes
  }

  static bool reportStatus = true;
  do
  {
    PUTS("------------ software_init_mdm! --------->\r\n");
    if (isReady() == 0)
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
        this->sw_init();
        if (WNC_MDM_ERR == WNC_NO_RESPONSE)
        {
          reinitialize_hw();
          this->sw_init(true); // Hard reset occurred so make it go through the software init();
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

void M14A2A::reinitialize_hw(void)
{
// Initialize the modem
#ifdef M14A2A_DEBUG
  PRINTF(GRN "Modem RE-initializing..." DEF "\r\n");
#endif
  if (!hw_init())
  {
#ifdef M14A2A_DEBUG
    PRINTF(RED "\n\rModem RE-initialization failed!" DEF "\n");
#endif
  }
#ifdef M14A2A_DEBUG
  PRINTF("\r\n");
#endif
}

int M14A2A::isReady(void)
{
  string *pRespStr = new string;
  size_t pos;
  int regSts;
  int cmdRes1, cmdRes2;

#ifdef M14A2A_DEBUG
  PUTS("<-------- Begin Cell Status ------------\r\n");
#endif
  cmdRes1 = at_cmd->sendCommandRsp("AT+CSQ", WNC_TIMEOUT_MS, pRespStr);   // Check RSSI,BER
  cmdRes2 = at_cmd->sendCommandRsp("AT+CPIN?", WNC_TIMEOUT_MS, pRespStr); // Check if SIM locked

  if ((cmdRes1 != 0) && (cmdRes2 != 0))
  {
#ifdef M14A2A_DEBUG
    PUTS("------------ WNC No Response! --------->\r\n");
#endif
    return (-2);
  }

  // If SIM Card not ready don't bother with commands!
  if (pRespStr->find("CPIN: READY") == string::npos)
  {
#ifdef M14A2A_DEBUG
    PUTS("------------ WNC SIM Problem! --------->\r\n");
#endif
    return (-1);
  }

  // SIM card OK, now check for signal and cellular network registration
  cmdRes1 = at_cmd->sendCommandRsp("AT+CREG?", WNC_TIMEOUT_MS, pRespStr); // Check if registered on network
  if (pRespStr->size() > 0)
  {
    pos = pRespStr->find("CREG: ");
    if (pos != string::npos)
    {
      // The registration is the 2nd arg in the comma separated list
      *pRespStr = pRespStr->substr(pos + 8, 1);
      regSts = atoi(pRespStr->c_str());
      // 1 - registered home, 5 - registered roaming
      if ((regSts != 1) && (regSts != 5))
      {
#ifdef M14A2A_DEBUG
        PUTS("------------ WNC Cell Link Down! ------>\r\n");
#endif
        return (-2);
      }
    }

#ifdef M14A2A_DEBUG
    PUTS("------------ WNC Ready ---------------->\r\n");
#endif
  }
  else
  {
#ifdef M14A2A_DEBUG
    PUTS("------------ CREG No Reply !----------->\r\n");
#endif
    return (-2);
  }

  return (0);
}

void M14A2A::sw_init(bool hardReset)
{
  static bool pdnSet = false;
  static bool intSet = false;
  static bool sockDialSet = false;
  string *pRespStr = new string;
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
  cmdRes = this->at_cmd->sendCommandRsp("AT", WNC_TIMEOUT_MS, pRespStr);         // Heartbeat?
  cmdRes += this->at_cmd->sendCommandRsp("ATE0", WNC_TIMEOUT_MS, pRespStr);      // Echo Off
  cmdRes += this->at_cmd->sendCommandRsp("AT+CMEE=2", WNC_TIMEOUT_MS, pRespStr); // 2 - verbose error, 1 - numeric error, 0 - just ERROR

  // If the simple commands are not working no chance of more complex.
  //  I have seen re-trying commands make it worse.
  if (cmdRes < 0)
  {
    // Since I used the at_send_wnc_cmd I am setting the error state based upon
    //  the responses.  And since these are simple commands, even if the WNC
    //  is saying ERROR, treat it like a no response.
    WNC_MDM_ERR = WNC_NO_RESPONSE;
    return;
  }

  if (intSet == false)
    cmdRes = this->at_cmd->sendCommandRsp("AT@INTERNET=1", WNC_TIMEOUT_MS, pRespStr);

  if (cmdRes == 0)
    intSet = true;
  else
    return;

  if (pdnSet == false)
  {
    string cmd_str("AT%PDNSET=1,");
    cmd_str += this->apn;
    cmd_str += ",IP";
    cmdRes = this->at_cmd->sendCommandRsp(cmd_str.c_str(), 4 * WNC_TIMEOUT_MS, pRespStr); // Set APN, cmd seems to take a little longer sometimes
  }

  if (cmdRes == 0)
    pdnSet = true;
  else
    return;

  if (sockDialSet == false)
    cmdRes = this->at_cmd->sendCommandRsp("AT@SOCKDIAL=1", WNC_TIMEOUT_MS, pRespStr);

  if (cmdRes == 0)
    sockDialSet = true;
  else
    return;

  PUTS("SUCCESS: AT init of WNC!\r\n");
}

char* M14A2A::getIPAddress()
{
  string *pRespStr = new string;
  int cmdRes;

  PUTS("Get IP Address: \r\n");
  // Quick commands below do not need to check cellular connectivity
  cmdRes = this->at_cmd->sendCommandRsp("AT+CGCONTRDP=1", WNC_TIMEOUT_MS, pRespStr); // Heartbeat?

  if (WNC_OK == cmdRes)
  {
    if (pRespStr->size() > 0)
    {
      string ss;
      size_t pe;
      size_t ps = pRespStr->rfind("\"");
      if (ps != string::npos)
      {
        ps += 2; // Skip the , after the "
        pe = ps;

        pe = pRespStr->find(".", pe);
        if (pe == string::npos)
          return NULL;
        else
          pe += 1;
        pe = pRespStr->find(".", pe);
        if (pe == string::npos)
          return NULL;
        else
          pe += 1;
        pe = pRespStr->find(".", pe);
        if (pe == string::npos)
          return NULL;
        else
          pe += 1;
        pe = pRespStr->find(".", pe);
        if (pe == string::npos)
          return NULL;
        else
          pe += 1;

        ss = pRespStr->substr(ps, pe - 1 - ps);
        char *cstr = new char[ss.length() + 1];
        strcpy(cstr, ss.c_str());
        return cstr;
      }
    }
  }

  return 0;
}
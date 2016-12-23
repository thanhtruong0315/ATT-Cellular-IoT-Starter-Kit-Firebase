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

#define MDM_DBG_OFF                             0
#define MDM_DBG_AT_CMDS                         (1 << 0)
int mdm_dbgmask = MDM_DBG_OFF;

#define WNC_WAIT_FOR_AT_CMD_MS 40

DigitalOut  mdm_uart2_rx_boot_mode_sel(PTC17);  // on powerup, 0 = boot mode, 1 = normal boot
DigitalOut  mdm_power_on(PTB9);                 // 0 = turn modem on, 1 = turn modem off (should be held high for >5 seconds to cycle modem)
DigitalOut  mdm_wakeup_in(PTC2);                // 0 = let modem sleep, 1 = keep modem awake -- Note: pulled high on shield

DigitalOut  mdm_reset(PTC12);                   // active high      

DigitalOut  shield_3v3_1v8_sig_trans_ena(PTC4); // 0 = disabled (all signals high impedence, 1 = translation active
DigitalOut  mdm_uart1_cts(PTD0);

#define TOUPPER(a) (a) //toupper(a)

const char ok_str[] = "OK";
const char error_str[] = "ERROR";

#define MDM_OK                                  0
#define MDM_ERR_TIMEOUT                         -1

#define MAX_AT_RSP_LEN                          255

ssize_t mdm_getline(char *buff, size_t size, int timeout_ms) {
    int cin = -1;
    int cin_last;
    
    if (NULL == buff || size == 0) {
        return -1;
    }

    size_t len = 0;
    Timer timer;
    timer.start();
    while ((len < (size-1)) && (timer.read_ms() < timeout_ms)) {
        if (mdm.readable()) {
            cin_last = cin;
            cin = mdm.getc();
            if (isprint(cin)) {
                buff[len++] = (char)cin;
                continue;
            } else if (('\r' == cin_last) && ('\n' == cin)) {
                break;
            }
        }
//        wait_ms(1);
    }
    buff[len] = (char)NULL;
    
    return len;
}

int mdm_sendAtCmd(const char *cmd, const char **rsp_list, int timeout_ms) {
    // Per WNC wait:
    wait_ms(WNC_WAIT_FOR_AT_CMD_MS);

    if (cmd && strlen(cmd) > 0) {
        if (mdm_dbgmask & MDM_DBG_AT_CMDS) {
            PRINTF(MAG "ATCMD: " DEF "--> " GRN "%s" DEF "\n", cmd);
        }
        mdm.puts(cmd);
        mdm.puts("\r\n");
    }

    if (rsp_list) {
        Timer   timer;
        char    rsp[MAX_AT_RSP_LEN+1];
        int     len;
        
        timer.start();
        while (timer.read_ms() < timeout_ms) {
            len = mdm_getline(rsp, sizeof(rsp), timeout_ms - timer.read_ms());
            
            if (len < 0)
                return MDM_ERR_TIMEOUT;

            if (len == 0)
                continue;
                
            if (mdm_dbgmask & MDM_DBG_AT_CMDS) {
                PRINTF(MAG "ATRSP: " DEF "<-- " CYN "%s" DEF "\n", rsp);
            }
        
            if (rsp_list) {
                int rsp_idx = 0;
                while (rsp_list[rsp_idx]) {
                    if (strcasecmp(rsp, rsp_list[rsp_idx]) == 0) {
                        return rsp_idx;
                    } else if (strncmp(rsp, "@EXTERR", 7) == 0){
                        pc.printf("----- We got EXTERR ---\r\n");
                        return 2;    
                    } else if (strncmp(rsp, "+CME", 4) == 0){
                        return 3;    
                    } 
                    rsp_idx++;
                }
            }
        }
        return MDM_ERR_TIMEOUT;
    }
    return MDM_OK;
}

int mdm_init(void) {
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
    mdm_uart2_rx_boot_mode_sel = 1;   // UART2_RX should be high
    mdm_power_on = 0;                 // powr_on should be low
    mdm_wakeup_in = 1;                // wake-up should be high
    mdm_uart1_cts = 0;                // indicate that it is ok to send

   // Now, wait for the WNC Module to perform its initial boot correctly
    wait(1.0);
  
    // The WNC module initializes comms at 115200 8N1 so set it up
     mdm.baud(115200);
    
    //Now, enable the level translator, the input pins should now be the
    //same as how the M14A module is driving them with internal pull ups/downs.
    //When enabled, there will be no changes in these 4 pins...
    shield_3v3_1v8_sig_trans_ena = 1;

    // Now, give the modem 60 seconds to start responding by
    // sending simple 'AT' commands to modem once per second.
    Timer timer;
    timer.start();
    while (timer.read() < 60) {
        const char * rsp_lst[] = { ok_str, error_str, NULL };
        int rc = mdm_sendAtCmd("AT", rsp_lst, 500);
        if (rc == 0)
            return true; //timer.read();
        wait_ms(1000 - (timer.read_ms() % 1000));
        PRINTF("\r%d",timer.read_ms()/1000);
    }
    return false;       
}

int mdm_sendAtCmdRsp(const char *cmd, const char **rsp_list, int timeout_ms, string * rsp, int * len) {
    static char cmd_buf[3200];  // Need enough room for the WNC sockreads (over 3000 chars)
    size_t n = strlen(cmd);

    // Per WNC wait:
    wait_ms(WNC_WAIT_FOR_AT_CMD_MS);

    if (cmd && n > 0) {
        if (mdm_dbgmask & MDM_DBG_AT_CMDS) {
            PRINTF(MAG "ATCMD: " DEF "--> " GRN "%s" DEF "\n", cmd);
        }
//        mdm.puts(cmd);
//        mdm.puts("\r\n");
        while (n--) {
            mdm.putc(*cmd++);
            wait_us(1000);
        };
        mdm.putc('\r');
        wait_us(1000);
        mdm.putc('\n');
        wait_us(1000);
    }

    if (rsp_list) {
        rsp->erase(); // Clean up from prior cmd response
        *len = 0;
        Timer   timer;
        timer.start();
        while (timer.read_ms() < timeout_ms) {
            int lenCmd = mdm_getline(cmd_buf, sizeof(cmd_buf), timeout_ms - timer.read_ms());

            if (lenCmd == 0)
                continue;

            if (lenCmd < 0)
                return MDM_ERR_TIMEOUT;
            else {
                *len += lenCmd;
                *rsp += cmd_buf;
            }

            if (mdm_dbgmask & MDM_DBG_AT_CMDS) {
                PRINTF(MAG "ATRSP: " DEF "<-- " CYN "%s" DEF "\n", cmd_buf);
            }

            int rsp_idx = 0;
            // TODO: Test if @EXTERR:<code>
            while (rsp_list[rsp_idx]) {
                if (strcasecmp(cmd_buf, rsp_list[rsp_idx]) == 0) {
                    return rsp_idx;
                } else if (strncmp(cmd_buf, "@EXTERR", 7) == 0){
                    PRINTF("----- We got EXTERR ---\r\n");
                    return 2;    
                } else if (strncmp(cmd_buf, "+CME", 4) == 0){
                    return 3;    
                } 
                rsp_idx++;
            }
        }
        return MDM_ERR_TIMEOUT;
    }

    return MDM_OK;
}

void reinitialize_mdm(void)
{
    // Initialize the modem
    PRINTF(GRN "Modem RE-initializing..." DEF "\r\n");
    if (!mdm_init()) {
        PRINTF(RED "\n\rModem RE-initialization failed!" DEF "\n");
    }
    PRINTF("\r\n");
}
// These are built on the fly
string MyServerIpAddress;
string MySocketData;

//********************************************************************************************************************************************
//* Process JSON response messages
//********************************************************************************************************************************************
bool extract_JSON(char* search_field, char* found_string)
{
    char* beginquote;
    char* endquote;
    beginquote = strchr(search_field, '{'); //start of JSON
    endquote = strchr(search_field, '}'); //end of JSON
    if (beginquote)
    {
        uint16_t ifoundlen;
        if (endquote)
        {
            ifoundlen = (uint16_t) (endquote - beginquote) + 1;
            strncpy(found_string, beginquote, ifoundlen );
            found_string[ifoundlen] = 0; //null terminate
            return true;
        }
        else
        {
            endquote = strchr(search_field, '\0'); //end of string...  sometimes the end bracket is missing
            ifoundlen = (uint16_t) (endquote - beginquote) + 1;
            strncpy(found_string, beginquote, ifoundlen );
            found_string[ifoundlen] = 0; //null terminate
            return false;
        }
    }
    else
    {
        return false;
    }
} //extract_JSON

int cell_modem_init() 
{
    int i;

    pc.baud(115200);
     // Initialize the modem
    PRINTF(GRN "Modem initializing... will take up to 60 seconds" DEF "\r\n");
    do {
        i=mdm_init();
        if (!i) {
            PRINTF(RED "Modem initialization failed!" DEF "\n");
        }
    } while (!i);
    
    //Software init
    software_init_mdm();
 
    // Resolve URL to IP address to connect to
    resolve_mdm();
    // Open the socket (connect to the server)
    sockopen_mdm();
    return (0);
}

int cell_modem_Sendreceive(char* tx_string, char* rx_string) 
{
    int iStatus = 0; //error by default
    PRINTF(DEF "\r\n");
    PRINTF(BLU "Sending to modem : %s" DEF "\r\n", &tx_string[0]); 
    sockwrite_mdm(&tx_string[0]);
    if (sockread_mdm(&MySocketData, 1024, 20))
    {
        PRINTF(DEF "\r\n");
        PRINTF(YEL "Read back : %s" DEF "\r\n", MySocketData.c_str());
        char stringToCharBuf[BUF_SIZE_FOR_N_MAX_SOCKREAD*MAX_WNC_SOCKREAD_PAYLOAD+1]; // WNC can return max of 1500 (per sockread)
        if ((MySocketData.length() + 1) < sizeof(stringToCharBuf))
        {
            strcpy(stringToCharBuf, MySocketData.c_str());
            if (extract_JSON(stringToCharBuf, &rx_string[0]))
            {
                PRINTF(GRN "JSON : %s" DEF "\n", &rx_string[0]);
                iStatus = 1; //all good
            }
        }
        else
        {
            PRINTF(RED "BUFFER not big enough for sock data!" DEF "\r\n");
        }
    }
    else
    {
        PRINTF(RED "No response..." DEF "\r\n");
    }
    return iStatus;
}

void display_wnc_firmware_rev(void)
{
    display_modem_firmware_version();
}
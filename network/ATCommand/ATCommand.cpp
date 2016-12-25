/*



*/

#include "ATCommand.h"
#include <stdarg.h>
#include <cctype>
#include <string>
#include "hardware.h"
ATCommand::ATCommand(PinName tx, PinName rx, int baud)
{
    _at_serial = new MODSERIAL(tx, rx);
    _at_serial->baud(baud);
}

int ATCommand::sendCommand(const char *cmd, const char **rsp_list, int timeout_ms)
{
        // Per WNC wait:
    wait_ms(WNC_WAIT_FOR_AT_CMD_MS);
        PRINTF("Begin sendCommand\r\n\r\n");

    if (cmd && strlen(cmd) > 0) {
        _at_serial->puts(cmd);
        _at_serial->puts("\r\n");
    }

    if (rsp_list) {
        char    rsp[MAX_AT_RSP_LEN+1];
        int     len;
        
        Timer timer;
        timer.start();
        while (timer.read_ms() < timeout_ms) {
            len = getline(rsp, sizeof(rsp), timeout_ms - timer.read_ms());
                    PRINTF("Begin sendCommand %s\r\n", rsp);

            if (len < 0)
                return MDM_ERR_TIMEOUT;

            if (len == 0)
                continue;
                
            if (rsp_list) {
                int rsp_idx = 0;
                while (rsp_list[rsp_idx]) {
                    if (strcasecmp(rsp, rsp_list[rsp_idx]) == 0) {
                        return rsp_idx;
                    } else if (strncmp(rsp, "@EXTERR", 7) == 0){
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

ssize_t ATCommand::getline(char *buff, size_t size, int timeout_ms) {
    int cin = -1;
    int cin_last;
    
    if (NULL == buff || size == 0) {
        return -1;
    }

    size_t len = 0;
    Timer timer;
    timer.start();
    while ((len < (size-1)) && (timer.read_ms() < timeout_ms)) {
        if (_at_serial->readable()) {
            cin_last = cin;
            cin = _at_serial->getc();
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
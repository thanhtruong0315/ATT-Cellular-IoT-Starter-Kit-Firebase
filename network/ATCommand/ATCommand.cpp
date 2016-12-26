/*



*/

#include "ATCommand.h"
#include <stdarg.h>
#include <cctype>
#include <string>
#include "hardware.h"
#include "mbed.h"

ATCommand::ATCommand(PinName tx, PinName rx, int baud)
{
    // FIX ME: why can not use new operator for MODSERIAL
    //_at_serial = new MODSERIAL(PTD3, PTD2);
    static MODSERIAL at_serial(tx, rx);
    _at_serial = &at_serial;
    _at_serial->baud(baud);
}

ATCommand::~ATCommand()
{
    delete _at_serial;
}

void ATCommand::sendCommandBySerial(const char *cmd)
{
    size_t n = strlen(cmd);
    wait_ms(WNC_WAIT_FOR_AT_CMD_MS);
    // Send command to module
    if (cmd && n > 0)
    {
        while (n--)
        {
            _at_serial->putc(*cmd++);
            wait_us(1000);
        };
        _at_serial->putc('\r');
        wait_us(1000);
        _at_serial->putc('\n');
        wait_us(1000);
    }
}

int ATCommand::send(const char *cmd, const char **rsp_list, int timeout_ms, string *response, int *response_len)
{
    static char cmd_buf[3200]; // Need enough room for the WNC sockreads (over 3000 chars)

    // send command to module by serial
    this->sendCommandBySerial(cmd);

    if (rsp_list)
    {
        if (response != NULL)
        {
            response->erase(); // Clean up from prior cmd response
            *response_len = 0;
        }

        Timer timer;
        timer.start();

        while (timer.read_ms() < timeout_ms)
        {
            int lenCmd = getline(cmd_buf, sizeof(cmd_buf), timeout_ms - timer.read_ms());

            // buffer is nothing
            if (lenCmd == 0)
                continue;

            if (lenCmd < 0)
            {
                return MDM_ERR_TIMEOUT;
            }
            else if (response != NULL)
            {
                *response_len += lenCmd;
                *response += cmd_buf;
            }

            // check string which is returned from module
            int checkResult = this->verifyATCode(rsp_list, cmd_buf);
            if (checkResult != -1)
                return checkResult;
        }
        return MDM_ERR_TIMEOUT;
    }

    return MDM_OK;
}

int ATCommand::verifyATCode(const char **rsp_list, char *at_code)
{
    int rsp_idx = 0;
    // TODO: Test if @EXTERR:<code>
    while (rsp_list[rsp_idx])
    {
        if (strcasecmp(at_code, rsp_list[rsp_idx]) == 0)
        {
            return rsp_idx;
        }
        else if (strncmp(at_code, "@EXTERR", 7) == 0)
        {
            PRINTF("----- We got EXTERR ---\r\n");
            return 2;
        }
        else if (strncmp(at_code, "+CME", 4) == 0)
        {
            return 3;
        }
        rsp_idx++;
    }
    return -1;
}

int ATCommand::sendCommand(const char *cmd, const char **rsp_list, int timeout_ms)
{
    int res;
    res = this->send(cmd, rsp_list, timeout_ms, NULL, NULL);
    return res;
}

int ATCommand::sendCommandRsp(const char *cmd, int timeout_ms, string *rsp)
{
    int res;
    int len;

    static const char *rsp_lst[] = {"OK", "ERROR", "@EXTERR", "+CME", NULL};
    res = this->send(cmd, rsp_lst, timeout_ms, rsp, &len);

    return res;
}

ssize_t ATCommand::getline(char *buff, size_t size, int timeout_ms)
{
    int cin = -1;
    int cin_last;

    if (NULL == buff || size == 0)
    {
        return -1;
    }

    size_t len = 0;
    Timer timer;
    timer.start();
    while ((len < (size - 1)) && (timer.read_ms() < timeout_ms))
    {
        if (_at_serial->readable())
        {
            cin_last = cin;
            cin = _at_serial->getc();
            if (isprint(cin))
            {
                buff[len++] = (char)cin;
                continue;
            }
            else if (('\r' == cin_last) && ('\n' == cin))
            {
                break;
            }
        }
        //        wait_ms(1);
    }
    buff[len] = (char)NULL;

    return len;
}
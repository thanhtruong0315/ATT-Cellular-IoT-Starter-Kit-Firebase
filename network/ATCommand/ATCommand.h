/*




*/

#ifndef AT_COMMAND_H
#define AT_COMMAND_H
#include "mbed.h"
#include "MODSERIAL.h"
#include <cctype>
#include <string>

#define DEFAULT_TIMEOUT_MS = 2000; // 2000 ms
#define WNC_WAIT_FOR_AT_CMD_MS 40
#define MDM_OK 0
#define MDM_ERR_TIMEOUT -1

#define MAX_AT_RSP_LEN 255
#define AT_DEBUG

// Dependencies: MODSERIAL
class ATCommand
{
public:
  ATCommand(PinName tx, PinName rx, int baud);
  ~ATCommand();
  // we have to implement Mutex for multiple threads which access cellular module at the same time.

  /**
    Send command & get string.

    @param cmd: AT command
    @param timeout: time for excuting function in milisecond
    @param rsp: string is returned from module
    @return 0: successful, another one: failure
  */
  int sendCommandRsp(const char *cmd, int timeout_ms, string *rsp);

  /**
    Send command to module.

    @param cmd: AT command
    @param timeout_ms: time for excuting function in milisecond
    @param rsp_list: list AT code
    @return 0: successful, another one: failure
  */
  int sendCommand(const char *cmd, const char **rsp_list, int timeout_ms);

protected:
private:
  ssize_t getline(char *buff, size_t size, int timeout_ms);
  int send(const char *cmd, const char **rsp_list, int timeout_ms, string *response = 0, int *response_len = 0);
  void sendCommandBySerial(const char *cmd);
  int verifyATCode(const char **rsp_list, char *at_code);

  MODSERIAL *_at_serial; // tx, rx
};
#endif
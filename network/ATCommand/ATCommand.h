/*




*/

#ifndef AT_COMMAND_H
#define AT_COMMAND_H
#include "mbed.h"
#include "MODSERIAL.h"

#define DEFAULT_TIMEOUT_MS = 2000; // 2000 ms
#define WNC_WAIT_FOR_AT_CMD_MS 40
#define MDM_OK 0
#define MDM_ERR_TIMEOUT -1

#define MAX_AT_RSP_LEN 255

class ATCommand
{
public:
  ATCommand(PinName tx, PinName rx, int baud);

  // we have to implement Mutex for multiple threads which access cellular module at the same time.
  int sendCommand(const char *cmd, const char **rsp_list, int timeout_ms);

protected:
private:
  ssize_t getline(char *buff, size_t size, int timeout_ms);
  MODSERIAL *_at_serial; // tx, rx
};
#endif
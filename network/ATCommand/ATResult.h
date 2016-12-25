#ifndef AT_RESULT_H
#define AT_RESULT_H
class ATResult
{
  public:
    ATTResult(char *data_buf)
    {
        data = data_buf;
    }
    enum
    {
        AT_OK,
        AT_ERROR,
        AT_CONNECT,
        AT_CMS_ERROR,
        AT_CME_ERROR
    } result;
    int code;
    char *data;
};
#endif
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

#ifndef _WNC_CONTROL_H_
#define _WNC_CONTROL_H_
#include "ATCommand.h"
#include "mbed.h"

class CellularModuleInterface
{
public:
  virtual int connectInternet();
  virtual char *getIPAddress();
  virtual void setAPN(const char *apn, const char *username = 0, const char *password = 0);
};

#endif

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

#ifndef __CONFIG_ME_H_
#define __CONFIG_ME_H_

// User must set these for own context:

#define BUF_SIZE_FOR_N_MAX_SOCKREAD (10)
#define MAX_WNC_SOCKREAD_PAYLOAD (1500)

// This is the server's base URL name.  Example "www.google.com"
// Note that when you Fork a FLOW, it will typically assign  either
// "run-east.att.io" or "run-west.att.io", so be sure to check this.
static const char * MY_SERVER_URL       = "run-east.att.io";

// These are FLOW fields from the Endpoints tab:
#define FLOW_BASE_URL                   "/69e8e8fe7d2b2/5d5ac3b2e84e/9fb99385eb9f191/in/flow"
#define FLOW_INPUT_NAME                 "/climate"

// Unless you want to use a different protocol, this field should be left as is:
#define FLOW_URL_TYPE                   " HTTP/1.1\r\nHost: "

// This identifier specifies with which FLOW device you are communicating. 
// If you only have one devive there then you can just leave this as is.
// Once your FLOW device has been initialized (Virtual Device Initialize clicked),
// the Virtual Device will show up in M2X.  This is its "DEVICE SERIAL" field
#define FLOW_DEVICE_NAME                "iot-kit"

// This constant defines how often sensors are read and sent up to FLOW
#define SENSOR_UPDATE_INTERVAL_MS       5000; //5 seconds

// Specify here how many sensor parameters you want reported to FLOW.
// You can use only the temperature and humidity from the shield HTS221
// or you can add the reading of the FXO8700CQ motion sensor on the FRDM-K64F board
// or if you have a SiLabs PMOD plugged into the shield, you can add its proximity sensor,
// UV light, visible ambient light and infrared ambient light readings
// If you run the Windows "Sensor Simulator" utility, 8 additional virtual
// sensors can also be made available via USB.
#define TEMP_HUMIDITY_ONLY                                      1
#define TEMP_HUMIDITY_ACCELEROMETER                             2
#define TEMP_HUMIDITY_ACCELEROMETER_GPS                         3
#define TEMP_HUMIDITY_ACCELEROMETER_PMODSENSORS                 4
#define TEMP_HUMIDITY_ACCELEROMETER_PMODSENSORS_VIRTUALSENSORS  5
static int iSensorsToReport = TEMP_HUMIDITY_ACCELEROMETER; //modify this to change your selection

// This is the APN name for the cellular network, you will need to change this, check the instructions included with your SIM card kit:
static const char * MY_APN_STR          = "m2m.com.attz";

//This is for normal HTTP.  If you want to use TCP to a specific port, change that here:
static const char * MY_PORT_STR         = "80";

#endif

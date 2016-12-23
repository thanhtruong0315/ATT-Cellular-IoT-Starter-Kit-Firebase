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

#include "mbed.h"
#include "sensors.h"
#include "hardware.h"
#include "config_me.h"
#include "FXOS8700CQ.h"
#include "HTS221.h"
#include "xadow_gps.h"
#include <string>

//I2C for pmod sensors:
#define Si1145_PMOD_I2C_ADDR   0xC0 //this is for 7-bit addr 0x60 for the Si7020
#define Si7020_PMOD_I2C_ADDR   0x80 //this is for 7-bit addr 0x4 for the Si7020

// Storage for the data from the motion sensor
SRAWDATA accel_data;
SRAWDATA magn_data;
//InterruptIn fxos_int1(PTC6); // unused, common with SW2 on FRDM-K64F
InterruptIn fxos_int2(PTC13); // should just be the Data-Ready interrupt
bool fxos_int2_triggered = false;
void trigger_fxos_int2(void)
{
    fxos_int2_triggered = true;

}

/*------------------------------------------------------------------------------
 * Perform I2C single read.
 *------------------------------------------------------------------------------*/
unsigned char I2C_ReadSingleByte(unsigned char ucDeviceAddress)
{
    char rxbuffer [1];
    i2c.read(ucDeviceAddress, rxbuffer, 1 );
    return (unsigned char)rxbuffer[0];
} //I2C_ReadSingleByte()

/*------------------------------------------------------------------------------
 * Perform I2C single read from address.
 *------------------------------------------------------------------------------*/
unsigned char I2C_ReadSingleByteFromAddr(unsigned char ucDeviceAddress, unsigned char Addr)
{
    char txbuffer [1];
    char rxbuffer [1];
    txbuffer[0] = (char)Addr;
    i2c.write(ucDeviceAddress, txbuffer, 1 );
    i2c.read(ucDeviceAddress, rxbuffer, 1 );
    return (unsigned char)rxbuffer[0];
} //I2C_ReadSingleByteFromAddr()

/*------------------------------------------------------------------------------
 * Perform I2C read of more than 1 byte.
 *------------------------------------------------------------------------------*/
int I2C_ReadMultipleBytes(unsigned char ucDeviceAddress, char *ucData, unsigned char ucLength)
{
    int status;
    status = i2c.read(ucDeviceAddress, ucData, ucLength);
    return status;
} //I2C_ReadMultipleBytes()

/*------------------------------------------------------------------------------
 * Perform I2C write of a single byte.
 *------------------------------------------------------------------------------*/
int I2C_WriteSingleByte(unsigned char ucDeviceAddress, unsigned char Data, bool bSendStop)
{
    int status;
    char txbuffer [1];
    txbuffer[0] = (char)Data; //data
    status = i2c.write(ucDeviceAddress, txbuffer, 1, !bSendStop); //true: do not send stop
    return status;
} //I2C_WriteSingleByte()

/*------------------------------------------------------------------------------
 * Perform I2C write of 1 byte to an address.
 *------------------------------------------------------------------------------*/
int I2C_WriteSingleByteToAddr(unsigned char ucDeviceAddress, unsigned char Addr, unsigned char Data, bool bSendStop)
{
    int status;
    char txbuffer [2];
    txbuffer[0] = (char)Addr; //address
    txbuffer[1] = (char)Data; //data
    //status = i2c.write(ucDeviceAddress, txbuffer, 2, false); //stop at end
    status = i2c.write(ucDeviceAddress, txbuffer, 2, !bSendStop); //true: do not send stop
    return status;
} //I2C_WriteSingleByteToAddr()

/*------------------------------------------------------------------------------
 * Perform I2C write of more than 1 byte.
 *------------------------------------------------------------------------------*/
int I2C_WriteMultipleBytes(unsigned char ucDeviceAddress, char *ucData, unsigned char ucLength, bool bSendStop)
{
    int status;
    status = i2c.write(ucDeviceAddress, ucData, ucLength, !bSendStop); //true: do not send stop
    return status;
} //I2C_WriteMultipleBytes()

bool bSi7020_present = false;
void Init_Si7020(void)
{
    char SN_7020 [8];
    //SN part 1:
    I2C_WriteSingleByteToAddr(Si7020_PMOD_I2C_ADDR, 0xFA, 0x0F, false);
    I2C_ReadMultipleBytes(Si7020_PMOD_I2C_ADDR, &SN_7020[0], 4);

    //SN part 1:
    I2C_WriteSingleByteToAddr(Si7020_PMOD_I2C_ADDR, 0xFC, 0xC9, false);
    I2C_ReadMultipleBytes(Si7020_PMOD_I2C_ADDR, &SN_7020[4], 4);

    char Ver_7020 [2];
    //FW version:
    I2C_WriteSingleByteToAddr(Si7020_PMOD_I2C_ADDR, 0x84, 0xB8, false);
    I2C_ReadMultipleBytes(Si7020_PMOD_I2C_ADDR, &Ver_7020[0], 2);

    if (SN_7020[4] != 0x14)
    {
        bSi7020_present = false;
        PRINTF("Si7020 sensor not found\r\n");
    }
    else 
    {
        bSi7020_present = true;
        PRINTF("Si7020 SN = 0x%02X%02X%02X%02X%02X%02X%02X%02X\n", SN_7020[0], SN_7020[1], SN_7020[2], SN_7020[3], SN_7020[4], SN_7020[5], SN_7020[6], SN_7020[7]);
        PRINTF("Si7020 Version# = 0x%02X\n", Ver_7020[0]);
    } //bool bSi7020_present = true

} //Init_Si7020()

void Read_Si7020(void)
{
    if (bSi7020_present)
    {
        char Humidity [2];
        char Temperature [2];
        //Command to measure humidity (temperature also gets measured):
        I2C_WriteSingleByte(Si7020_PMOD_I2C_ADDR, 0xF5, false); //no hold, must do dummy read
        I2C_ReadMultipleBytes(Si7020_PMOD_I2C_ADDR, &Humidity[0], 1); //dummy read, should get an nack until it is done
        wait (0.05); //wait for measurement.  Can also keep reading until no NACK is received
        //I2C_WriteSingleByte(Si7020_PMOD_I2C_ADDR, 0xE5, false); //Hold mod, the device does a clock stretch on the read until it is done (crashes the I2C bus...
        I2C_ReadMultipleBytes(Si7020_PMOD_I2C_ADDR, &Humidity[0], 2); //read humidity
        //PRINTF("Read Si7020 Humidity = 0x%02X%02X\n", Humidity[0], Humidity[1]);
        int rh_code = (Humidity[0] << 8) + Humidity[1];
        float fRh = (125.0*rh_code/65536.0) - 6.0; //from datasheet
        //PRINTF("Si7020 Humidity = %*.*f %%\n", 4, 2, fRh); //double % sign for escape //PRINTF("%*.*f\n", myFieldWidth, myPrecision, myFloatValue);
        sprintf(SENSOR_DATA.Humidity_Si7020, "%0.2f", fRh);
        
        //Command to read temperature when humidity is already done:
        I2C_WriteSingleByte(Si7020_PMOD_I2C_ADDR, 0xE0, false);
        I2C_ReadMultipleBytes(Si7020_PMOD_I2C_ADDR, &Temperature[0], 2); //read temperature
        //PRINTF("Read Si7020 Temperature = 0x%02X%02X\n", Temperature[0], Temperature[1]);
        int temp_code = (Temperature[0] << 8) + Temperature[1];
        float fTemp = (175.72*temp_code/65536.0) - 46.85; //from datasheet in Celcius
        //PRINTF("Si7020 Temperature = %*.*f deg C\n", 4, 2, fTemp);
        sprintf(SENSOR_DATA.Temperature_Si7020, "%0.2f", fTemp);
    } //bool bSi7020_present = true

} //Read_Si7020()

/*------------------------------------------------------------------------------
 * The following are aliases so that the Si1145 coding examples can be used as-is.
 *------------------------------------------------------------------------------*/
unsigned char ReadFrom_Si1145_Register(unsigned char reg) //returns byte from I2C Register 'reg'
{
    unsigned char result = I2C_ReadSingleByteFromAddr(Si1145_PMOD_I2C_ADDR, reg);
    return (result);
} //ReadFrom_Si1145_Register()

void WriteTo_Si1145_Register(unsigned char reg, unsigned char value) //writes 'value' into I2C Register reg'
{
    I2C_WriteSingleByteToAddr(Si1145_PMOD_I2C_ADDR, reg, value, true);
} //WriteTo_Si1145_Register()

#define REG_PARAM_WR 0x17
#define REG_PARAM_RD 0x2E
#define REG_COMMAND 0x18
#define REG_RESPONSE 0x20
#define REG_HW_KEY 0x07
#define HW_KEY_VAL0 0x17
#define REG_MEAS_RATE_LSB 0x08
#define REG_MEAS_RATE_MSB 0x09
#define REG_PS_LED21 0x0F
#define REG_PS_LED3 0x10
#define MAX_LED_CURRENT 0xF
#define PARAM_CH_LIST 0x01
#define REG_ALS_VIS_DATA0 0x22
#define REG_ALS_VIS_DATA1 0x23
#define REG_ALS_IR_DATA0 0x24
#define REG_ALS_IR_DATA1 0x25
#define REG_PS1_DATA0 0x26
#define REG_PS1_DATA1 0x27
#define REG_PS2_DATA0 0x28
#define REG_PS2_DATA1 0x29
#define REG_PS3_DATA0 0x2A
#define REG_PS3_DATA1 0x2B
#define REG_UVINDEX0 0x2C
#define REG_UVINDEX1 0x2D
int Si1145_ParamSet(unsigned char address, unsigned char value) //writes 'value' into Parameter 'address'
{
    char txbuffer [3];
    txbuffer[0] = (char)REG_PARAM_WR; //destination
    txbuffer[1] = (char)value;
    txbuffer[2] = (char)(0xA0 + (address & 0x1F));
    int retval;
    //if((retval = _waitUntilSleep(si114x_handle))!=0) return retval;
    retval = I2C_WriteMultipleBytes(Si1145_PMOD_I2C_ADDR, &txbuffer[0], 3, true);
    if(retval!=0) return retval;
    while(1)
    {
        retval=ReadFrom_Si1145_Register(REG_PARAM_RD);
        if (retval==value) break;
    }
    return (0);
} //Si1145_ParamSet()

void PsAlsForce(void) //equivalent to WriteTo_Si1145_Register(REG_COMMAND,0x07).  This forces PS and ALS measurements
{
    WriteTo_Si1145_Register(REG_COMMAND,0x07);
} //PsAlsForce()

bool bSi1145_present = false;
void Init_Si1145(void)
{
    unsigned char readbyte;
    //Read Si1145 part ID:
    readbyte = ReadFrom_Si1145_Register(0x00);
    if (readbyte != 0x45)
    {
        bSi1145_present = false;
        PRINTF("Si1145 sensor not found\r\n");
    }
    else
    {
        bSi1145_present = true;
        PRINTF("Si1145 Part ID : 0x%02X\n", readbyte);
        //Initialize Si1145 by writing to HW_KEY (I2C Register 0x07 = 0x17)
        WriteTo_Si1145_Register(REG_HW_KEY, HW_KEY_VAL0);
    
        // Initialize LED Current
        // I2C Register 0x0F = 0xFF
        // I2C Register 0x10 = 0x0F
        WriteTo_Si1145_Register(REG_PS_LED21,(MAX_LED_CURRENT<<4) + MAX_LED_CURRENT);
        WriteTo_Si1145_Register(REG_PS_LED3, MAX_LED_CURRENT);

        // Parameter 0x01 = 0x37
        //Si1145_ParamSet(PARAM_CH_LIST, ALS_IR_TASK + ALS_VIS_TASK + PS1_TASK + PS2_TASK + PS3_TASK);
        //Si1145_ParamSet(0x01, 0x37); //CHLIST is address 0x01 in the parameter RAM.  It defines which sensors are enabled (here, some)
        Si1145_ParamSet(0x01, 0x7F); //CHLIST is address 0x01 in the parameter RAM.  It defines which sensors are enabled (here, all but UV.  One can only use AUX or UV but here we use AUX because UV does not work...)
        // I2C Register 0x18 = 0x0x07 //This is PSALS_FORCE to the Command register => Force a single PS (proximity sensor) and ALS (ambient light sensor) reading - The factory code has this as 0x05 which only does PS...
        PsAlsForce(); // can also be written as WriteTo_Si1145_Register(REG_COMMAND,0x07);
        WriteTo_Si1145_Register(REG_COMMAND, 0x0F);//command to put it into auto mode
        //Set MES_RATE to 0x1000.  I.e. the device will automatically wake up every 16 * 256* 31.25 us = 0.128 seconds to measure
        WriteTo_Si1145_Register(REG_MEAS_RATE_LSB, 0x00);
        WriteTo_Si1145_Register(REG_MEAS_RATE_MSB, 0x10);
    } //bSi1145_present = true
} //Init_Si1145()

void Read_Si1145(void)
{
    if (bSi1145_present)
    {
        // Once the measurements are completed, here is how to reconstruct them
        // Note very carefully that 16-bit registers are in the 'Little Endian' byte order
        // It may be more efficient to perform block I2C Reads, but this example shows
        // individual reads of registers
    
        int PS1 = ReadFrom_Si1145_Register(REG_PS1_DATA0) + 256 * ReadFrom_Si1145_Register(REG_PS1_DATA1);
        int PS2 = ReadFrom_Si1145_Register(REG_PS2_DATA0) + 256 * ReadFrom_Si1145_Register(REG_PS2_DATA1);
        int PS3 = ReadFrom_Si1145_Register(REG_PS3_DATA0) + 256 * ReadFrom_Si1145_Register(REG_PS3_DATA1);
        //PRINTF("PS1_Data = %d\n", PS1);
        //PRINTF("PS2_Data = %d\n", PS2);
        //PRINTF("PS3_Data = %d\n", PS3);
        //OBJECT PRESENT?
#if (0)
        if(PS1 < 22000){
            //PRINTF("Object Far\n");
            sprintf(SENSOR_DATA.Proximity, "Object Far\0");
        }
        else if(PS1 < 24000)
        {
            //PRINTF("Object in Vicinity\n");
            sprintf(SENSOR_DATA.Proximity, "Object in Vicinity\0");
        }
        else if (PS1 < 30000)
        {
            //PRINTF("Object Near\n");
            sprintf(SENSOR_DATA.Proximity, "Object Near\0");
        }
        else
        {
            //PRINTF("Object Very Near\n");
            sprintf(SENSOR_DATA.Proximity, "Object Very Near\0");
        }
#else    
        sprintf(SENSOR_DATA.Proximity, "%d\0", PS1);
#endif            
    
        //Force ALS read:
        //WriteTo_Si1145_Register(REG_COMMAND, 0x06);
        //wait (0.1);
        int ALS_VIS = ReadFrom_Si1145_Register(REG_ALS_VIS_DATA0) + 256 * ReadFrom_Si1145_Register(REG_ALS_VIS_DATA1);
        int ALS_IR = ReadFrom_Si1145_Register(REG_ALS_IR_DATA0) + 256 * ReadFrom_Si1145_Register(REG_ALS_IR_DATA1);
        int UV_INDEX = ReadFrom_Si1145_Register(REG_UVINDEX0) + 256 * ReadFrom_Si1145_Register(REG_UVINDEX1);
        //PRINTF("ALS_VIS_Data = %d\n", ALS_VIS);
        //PRINTF("ALS_IR_Data = %d\n", ALS_IR);
        //PRINTF("UV_INDEX_Data = %d\n", UV_INDEX);
    
        //PRINTF("Ambient Light Visible  Sensor = %d\n", ALS_VIS);
        sprintf(SENSOR_DATA.AmbientLightVis, "%d", ALS_VIS);
        //PRINTF("Ambient Light Infrared Sensor = %d\n", ALS_IR);
        sprintf(SENSOR_DATA.AmbientLightIr, "%d", ALS_IR);
        //float fUV_value = (UV_INDEX -50.0)/10000.0;
        float fUV_value = (UV_INDEX)/100.0; //this is the aux reading
        //PRINTF("UV_Data = %0.2f\n", fUV_value);
        sprintf(SENSOR_DATA.UVindex, "%0.2f", fUV_value);
    } //bSi1145_present = true
} //Read_Si1145()

//********************************************************************************************************************************************
//* Read the FXOS8700CQ - 6-axis combo Sensor Accelerometer and Magnetometer
//********************************************************************************************************************************************
bool bMotionSensor_present = false;
void Init_motion_sensor()
{
    // Note: this class is instantiated here because if it is statically declared, the cellular shield init kills the I2C bus...
    // Class instantiation with pin names for the motion sensor on the FRDM-K64F board:
    FXOS8700CQ fxos(PTE25, PTE24, FXOS8700CQ_SLAVE_ADDR1); // SDA, SCL, (addr << 1)
    int iWhoAmI = fxos.get_whoami();

    PRINTF("FXOS8700CQ WhoAmI = %X\r\n", iWhoAmI);
    // Iterrupt for active-low interrupt line from FXOS
    // Configured with only one interrupt on INT2 signaling Data-Ready
    //fxos_int2.fall(&trigger_fxos_int2);
    if (iWhoAmI != 0xC7)
    {
        bMotionSensor_present = false;
        PRINTF("FXOS8700CQ motion sensor not found\r\n");
    }
    else
    {
        bMotionSensor_present = true;
        fxos.enable();
    }
} //Init_motion_sensor()

void Read_motion_sensor()
{
    // Note: this class is instantiated here because if it is statically declared, the cellular shield init kills the I2C bus...
    // Class instantiation with pin names for the motion sensor on the FRDM-K64F board:
    FXOS8700CQ fxos(PTE25, PTE24, FXOS8700CQ_SLAVE_ADDR1); // SDA, SCL, (addr << 1)
    if (bMotionSensor_present)
    {
        fxos.enable();
        fxos.get_data(&accel_data, &magn_data);
        //PRINTF("Roll=%5d, Pitch=%5d, Yaw=%5d;\r\n", magn_data.x, magn_data.y, magn_data.z);
        sprintf(SENSOR_DATA.MagnetometerX, "%5d", magn_data.x);
        sprintf(SENSOR_DATA.MagnetometerY, "%5d", magn_data.y);
        sprintf(SENSOR_DATA.MagnetometerZ, "%5d", magn_data.z);
    
        //Try to normalize (/2048) the values so they will match the eCompass output:
        float fAccelScaled_x, fAccelScaled_y, fAccelScaled_z;
        fAccelScaled_x = (accel_data.x/2048.0);
        fAccelScaled_y = (accel_data.y/2048.0);
        fAccelScaled_z = (accel_data.z/2048.0);
        //PRINTF("Acc: X=%2.3f Y=%2.3f Z=%2.3f;\r\n", fAccelScaled_x, fAccelScaled_y, fAccelScaled_z);
        sprintf(SENSOR_DATA.AccelX, "%2.3f", fAccelScaled_x);
        sprintf(SENSOR_DATA.AccelY, "%2.3f", fAccelScaled_y);
        sprintf(SENSOR_DATA.AccelZ, "%2.3f", fAccelScaled_z);
    } //bMotionSensor_present
} //Read_motion_sensor()


//********************************************************************************************************************************************
//* Read the HTS221 temperature & humidity sensor on the Cellular Shield
//********************************************************************************************************************************************
// These are to be built on the fly
string my_temp;
string my_humidity;
HTS221 hts221;

#define CTOF(x)  ((x)*1.8+32)
bool bHTS221_present = false;
void Init_HTS221()
{
    int i;
    void hts221_init(void);
    i = hts221.begin();
    if (i)
    {
        bHTS221_present = true;
        PRINTF(BLU "HTS221 Detected (0x%02X)\n\r",i);
        PRINTF("  Temp  is: %0.2f F \n\r",CTOF(hts221.readTemperature()));
        PRINTF("  Humid is: %02d %%\n\r",hts221.readHumidity());
    }
    else
    {
        bHTS221_present = false;
        PRINTF(RED "HTS221 NOT DETECTED!\n\r");
    }
} //Init_HTS221()

void Read_HTS221()
{
    if (bHTS221_present)
    {
        sprintf(SENSOR_DATA.Temperature, "%0.2f", CTOF(hts221.readTemperature()));
        sprintf(SENSOR_DATA.Humidity, "%02d", hts221.readHumidity());
    } //bHTS221_present
} //Read_HTS221()

bool bGPS_present = false;
void Init_GPS(void)
{
    char scan_id[GPS_SCAN_SIZE+2]; //The first two bytes are the response length (0x00, 0x04)
    I2C_WriteSingleByte(GPS_DEVICE_ADDR, GPS_SCAN_ID, true); //no hold, must do read

    unsigned char i;
    for(i=0;i<(GPS_SCAN_SIZE+2);i++)
    {
        scan_id[i] = I2C_ReadSingleByte(GPS_DEVICE_ADDR);
    }

    if(scan_id[5] != GPS_DEVICE_ID)
    {
        bGPS_present = false;
        PRINTF("Xadow GPS not found\n");
    }
    else 
    {
        bGPS_present = true;
        PRINTF("Xadow GPS Scan ID response = 0x%02X%02X (length), 0x%02X%02X%02X%02X\r\n", scan_id[0], scan_id[1], scan_id[2], scan_id[3], scan_id[4], scan_id[5]);
        char status = gps_get_status();
        if ((status != 'A') && (iSensorsToReport == TEMP_HUMIDITY_ACCELEROMETER_GPS))
        { //we must wait for GPS to initialize
            PRINTF("Waiting for GPS to become ready... ");
            while (status != 'A')
            {
                wait (5.0);        
                status = gps_get_status();
                unsigned char num_satellites = gps_get_sate_in_veiw();
                PRINTF("%c%d", status, num_satellites);
            }
            PRINTF("\r\n");
        } //we must wait for GPS to initialize
        PRINTF("gps_check_online is %d\r\n", gps_check_online());
        unsigned char *data;
        data = gps_get_utc_date_time();       
        PRINTF("gps_get_utc_date_time : %d-%d-%d,%d:%d:%d\r\n", data[0], data[1], data[2], data[3], data[4], data[5]); 
        PRINTF("gps_get_status        : %c ('A' = Valid, 'V' = Invalid)\r\n", gps_get_status());
        PRINTF("gps_get_latitude      : %c:%f\r\n", gps_get_ns(), gps_get_latitude());
        PRINTF("gps_get_longitude     : %c:%f\r\n", gps_get_ew(), gps_get_longitude());
        PRINTF("gps_get_altitude      : %f meters\r\n", gps_get_altitude());
        PRINTF("gps_get_speed         : %f knots\r\n", gps_get_speed());
        PRINTF("gps_get_course        : %f degrees\r\n", gps_get_course());
        PRINTF("gps_get_position_fix  : %c\r\n", gps_get_position_fix());
        PRINTF("gps_get_sate_in_view  : %d satellites\r\n", gps_get_sate_in_veiw());
        PRINTF("gps_get_sate_used     : %d\r\n", gps_get_sate_used());
        PRINTF("gps_get_mode          : %c ('A' = Automatic, 'M' = Manual)\r\n", gps_get_mode());
        PRINTF("gps_get_mode2         : %c ('1' = no fix, '1' = 2D fix, '3' = 3D fix)\r\n", gps_get_mode2()); 
    } //bool bGPS_present = true
} //Init_GPS()

void Read_GPS()
{
    unsigned char gps_satellites = 0; //default
    int lat_sign;
    int long_sign;
    if (bGPS_present)
    {
        if ((gps_get_status() == 'A') && (gps_get_mode2() != '1'))
        {
            gps_satellites = gps_get_sate_in_veiw(); //show the number of satellites
        }
        if (gps_get_ns() == 'S')
        {
            lat_sign = -1; //negative number
        }
        else
        {
            lat_sign = 1;
        }    
        if (gps_get_ew() == 'W')
        {
            long_sign = -1; //negative number
        }
        else
        {
            long_sign = 1;
        }    
#if (0)
        PRINTF("gps_satellites        : %d\r\n", gps_satellites);
        PRINTF("gps_get_latitude      : %f\r\n", (lat_sign * gps_get_latitude()));
        PRINTF("gps_get_longitude     : %f\r\n", (long_sign * gps_get_longitude()));
        PRINTF("gps_get_altitude      : %f meters\r\n", gps_get_altitude());
        PRINTF("gps_get_speed         : %f knots\r\n", gps_get_speed());
        PRINTF("gps_get_course        : %f degrees\r\n", gps_get_course());
#endif
        sprintf(SENSOR_DATA.GPS_Satellites, "%d", gps_satellites);
        sprintf(SENSOR_DATA.GPS_Latitude, "%f", (lat_sign * gps_get_latitude()));
        sprintf(SENSOR_DATA.GPS_Longitude, "%f", (long_sign * gps_get_longitude()));
        sprintf(SENSOR_DATA.GPS_Altitude, "%f", gps_get_altitude());
        sprintf(SENSOR_DATA.GPS_Speed, "%f", gps_get_speed());
        sprintf(SENSOR_DATA.GPS_Course, "%f", gps_get_course());
    } //bGPS_present
} //Read_GPS()

#ifdef USE_VIRTUAL_SENSORS
bool bUsbConnected = false;
volatile uint8_t usb_uart_rx_buff[256];
//volatile uint8_t usb_uart_tx_buff[256];
volatile unsigned char usb_uart_rx_buff_putptr = 0;
volatile unsigned char usb_uart_rx_buff_getptr = 0;
//volatile unsigned char usb_uart_tx_buff_putptr = 0;
//volatile unsigned char usb_uart_tx_buff_getptr = 0;
char usbhost_rx_string[256];
unsigned char usbhost_rxstring_index;
char usbhost_tx_string[256];


float f_sensor1_value = 12.3;
float f_sensor2_value = 45.6;
float f_sensor3_value = 78.9;
float f_sensor4_value = 78.9;
float f_sensor5_value = 78.9;
float f_sensor6_value = 78.9;
float f_sensor7_value = 78.9;
float f_sensor8_value = 78.9;
char usb_sensor_string[110];


//********************************************************************************************************************************************
//* Parse the input sensor data from the USB host
//********************************************************************************************************************************************
int parse_usbhost_message()
{
    //PRINTF("String = %s\n", usbhost_rx_string); //test
    uint8_t length;
    uint8_t x ;
    //It seems that sscanf needs 11 characters to store a 7-character number.  There must be some formatting and termination values...
    char Record[8][11]; //There are 8 sensors with up to 7 characters each
    char StringRecord[110]; //There are is a sensor "string" with up to 100 characters in it

    // Data format is:  "S1:1234,S2:5678,S3:1234,S4:5678,S5:1234,S6:5678,S7:5678,S8:5678,S9:abcde...\n"
    int args_assigned = sscanf(usbhost_rx_string, "%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^\n]", Record[0], Record[1], Record[2], Record[3], Record[4], Record[5], Record[6], Record[7],  StringRecord);

    //StringRecord[109] = '\0';
    //PRINTF("Last = %s\n", StringRecord); //test

    if (args_assigned == 9)
    { //sscanf was able to assign all 9 values
        for (x=0; x < 8; x++)  // loop through the 8 sensors
        {
            // Strip the "Sx:" label characters from the field value
            length = strlen(Record[x]);             // max of 7 characters but could be less
            strncpy(Record[x], Record[x] + 3, length);
            Record[x][length] = '\0';              // null termination character manually added
        }
        length = strlen(StringRecord);
        strncpy(StringRecord, StringRecord + 3, length);
        StringRecord[length] = '\0';   // null termination character manually added
    
        if ((usbhost_rx_string[0] == 'S') && (usbhost_rx_string[1] == '1')) //The message starts with "S1"
        {
            f_sensor1_value = atof(Record[0]);
            f_sensor2_value = atof(Record[1]);
            f_sensor3_value = atof(Record[2]);
            f_sensor4_value = atof(Record[3]);
            f_sensor5_value = atof(Record[4]);
            f_sensor6_value = atof(Record[5]);
            f_sensor7_value = atof(Record[6]);
            f_sensor8_value = atof(Record[7]);
            sprintf(usb_sensor_string,StringRecord);
            //PRINTF("Received = %s, %s, %s, %s, %s, %s, %s, %s, %s\n", Record[0], Record[1], Record[2], Record[3], Record[4], Record[5], Record[6], Record[7], usb_sensor_string); //test
            sprintf(SENSOR_DATA.Virtual_Sensor1, "%s", Record[0]);
            sprintf(SENSOR_DATA.Virtual_Sensor2, "%s", Record[1]);
            sprintf(SENSOR_DATA.Virtual_Sensor3, "%s", Record[2]);
            sprintf(SENSOR_DATA.Virtual_Sensor4, "%s", Record[3]);
            sprintf(SENSOR_DATA.Virtual_Sensor5, "%s", Record[4]);
            sprintf(SENSOR_DATA.Virtual_Sensor6, "%s", Record[5]);
            sprintf(SENSOR_DATA.Virtual_Sensor7, "%s", Record[6]);
            sprintf(SENSOR_DATA.Virtual_Sensor8, "%s", Record[7]);
        }
    } //sscanf was able to assign all values
    return args_assigned;
} //parse_usbhost_message()

//********************************************************************************************************************************************
//* Process any received message from the USB host
//********************************************************************************************************************************************
void process_usb_rx(unsigned char ucNewRxByte)
{
    if (ucNewRxByte == '?')
    { //just pinging
        usbhost_rxstring_index = 0;
        return;
    } //just pinging
    usbhost_rx_string[usbhost_rxstring_index++] = ucNewRxByte;
    if (ucNewRxByte == '\n')
    { //end of message
        usbhost_rx_string[usbhost_rxstring_index] = 0; //null terminate string
        usbhost_rxstring_index = 0;
        parse_usbhost_message();
    } //end of message
} //process_usb_rx()

void ProcessUsbInterface(void)
{
    //Process the USB host UART receive commands:
    if (usb_uart_rx_buff_getptr != usb_uart_rx_buff_putptr)
    {
        bUsbConnected = true;
        while (usb_uart_rx_buff_getptr != usb_uart_rx_buff_putptr)
        {
            unsigned char ucByteFromHost = usb_uart_rx_buff[usb_uart_rx_buff_getptr++]; //Copy latest received byte
            process_usb_rx(ucByteFromHost);
        }  //while (usb_uart_rx_buff_getptr != usb_uart_rx_buff_putptr)
    } // if there are USB UART bytes to receive
    //USB host UART transmit:
    //while (usb_uart_tx_buff_getptr != usb_uart_tx_buff_putptr)
    //{
        //pc.putc(usb_uart_tx_buff[usb_uart_tx_buff_getptr++]);
    //}
} //ProcessUsbInterface()

// This function is called when a character goes into the RX buffer.
void UsbUartRxCallback(MODSERIAL_IRQ_INFO *info) 
{
    // Get the pointer to our MODSERIAL object that invoked this callback.
    MODSERIAL *pc = info->serial;
    while (pc->readable())
    {
        usb_uart_rx_buff[usb_uart_rx_buff_putptr++] = pc->getcNb();
    }
}
#endif
 
void sensors_init(void)
{
#ifdef USE_VIRTUAL_SENSORS
   pc.attach(&UsbUartRxCallback, MODSERIAL::RxIrq);
#endif
    Init_HTS221();
    Init_Si7020();
    Init_Si1145();
    Init_motion_sensor();
    Init_GPS();
} //sensors_init

void read_sensors(void)
{
    Read_HTS221();
    Read_Si7020();
    Read_Si1145();
    Read_motion_sensor();
    Read_GPS();
} //read_sensors

/**
  ******************************************************************************
  * @file    QxAutoMLWork.c
  * @author  Qeexo Kernel Development team
  * @version V1.0.0
  * @date    30-Sep-2020
  * @brief   Auto ML module for Inference 
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020 Qeexo Co.
  * All rights reserved
  *
  *
  * ALL INFORMATION CONTAINED HEREIN IS AND REMAINS THE PROPERTY OF QEEXO, CO.
  * THE INTELLECTUAL AND TECHNICAL CONCEPTS CONTAINED HEREIN ARE PROPRIETARY TO
  * QEEXO, CO. AND MAY BE COVERED BY U.S. AND FOREIGN PATENTS, PATENTS IN PROCESS,
  * AND ARE PROTECTED BY TRADE SECRET OR COPYRIGHT LAW. DISSEMINATION OF
  * THIS INFORMATION OR REPRODUCTION OF THIS MATERIAL IS STRICTLY FORBIDDEN UNLESS
  * PRIOR WRITTEN PERMISSION IS OBTAINED OR IS MADE PURSUANT TO A LICENSE AGREEMENT
  * WITH QEEXO, CO. ALLOWING SUCH DISSEMINATION OR REPRODUCTION.
  *
  ******************************************************************************
 */

#include "QxAutoMLUser.h"

/* Customer Native Fucntions*/
extern void NativeInitSensor(void);
extern void NativeOSDelay(int msec);
extern int NativeOSGetTick(void);

/* Start of native sensor driver interface declaration */

/*
    This function returns accel & gyro fifo sample data count, which is the remainning
    sensor data number, each number's size is 6 bytes 3axis.
    Please refer to https://content.arduino.cc/assets/Nano_BLE_Sense_lsm9ds1.pdf
*/
extern short lsm9ds1_read_fifocount();

/*
    This function read the accel and gyro sensor data to the gived input 'data' buffer, the read number
    dependeds on parameter 'remaining'.
    Please refer to https://content.arduino.cc/assets/Nano_BLE_Sense_lsm9ds1.pdf
 */
extern int lsm9ds1_read_fifoData(
                unsigned short remaining, 
                unsigned short* accel_data, 
                unsigned short* gyro_data);

void* QxAudioHal_Read(unsigned short* data_bits, unsigned short* data_len);

/* End of native sensor driver interface declaration */

static void QxFillDataFrame()
{

   unsigned short data_len;
   unsigned short data_bits;
   
   void* data;

/* Start of native reading accel and gyro data from sensor driver */

    #define MAX_FIFO_BUFFER 32 //samples
    #define AXIS_NUMBER 3 
    
    #define MIN(a,b) ((a < b) ? a : b)

    unsigned short read_samples = 0, remained_samples = 0;
    
#if defined (QXAUTOMLCONFIG_SENSOR_ENABLE_ACCEL) || defined (QXAUTOMLCONFIG_SENSOR_ENABLE_GYRO)

    remained_samples = lsm9ds1_read_fifocount();

    if(remained_samples > 0) {
        static unsigned short accel_data[AXIS_NUMBER * MAX_FIFO_BUFFER];
        static unsigned short gyro_data[AXIS_NUMBER * MAX_FIFO_BUFFER];

        read_samples = MIN(remained_samples, MAX_FIFO_BUFFER);

        lsm9ds1_read_fifoData(read_samples, accel_data, gyro_data);

/* End of native reading accel and gyro data from sensor driver */

/* Fill Accel data if it is enabled in staic engine library */
#ifdef QXAUTOMLCONFIG_SENSOR_ENABLE_ACCEL
    QxFillSensorData(SENSOR_TYPE_ACCEL, accel_data, read_samples*6);
#endif

/* Fill Gryo data if it is enabled in staic engine library */
#ifdef QXAUTOMLCONFIG_SENSOR_ENABLE_GYRO
    QxFillSensorData(SENSOR_TYPE_GYRO, gyro_data, read_samples*6);
#endif
     }
#endif

/* Fill Magnetic data if it is enabled in staic engine library */
#ifdef QXAUTOMLCONFIG_SENSOR_ENABLE_MAG
    data = lsm9ds1_mag_read(NULL, &data_bits, &data_len);
    QxFillSensorData(SENSOR_TYPE_MAG, data, data_len);
#endif

/* Fill Microphone data if it is enabled in staic engine library */
#ifdef QXAUTOMLCONFIG_SENSOR_ENABLE_MICROPHONE
    data = QxAudioHal_Read(&data_bits, &data_len);
    QxFillSensorData(SENSOR_TYPE_MICROPHONE, data, data_len);
#endif

}

int QxAutoMLWork()
{
    static int time_sum = 0;
    static int result = -1;

    /* Qeexo AutoML work interval user fixed 10ms */
    int work_interval = 10; 

    /* Customer implement this API to intialize device sensor at the first time be called */
    /* After private pointer variables is assigned, we call this function
    to init any sensors that are used by current static classify engine libarary. */
    NativeInitSensor();
    
    /* Get current tick in ms */
    int tick = NativeOSGetTick();

    QxFillDataFrame();

    time_sum += 10;
    if (time_sum >= PRED_CLASSIFICATION_INTERVAL_IN_MSECS) { 
         /* Call classify periodically each pred_interval */
        result = QxClassify();
        time_sum = 0; 
    }
        
    int diff = NativeOSGetTick() - tick;

    if(diff < work_interval) {
        NativeOSDelay(work_interval - diff);
    }

    return result;
}



#pragma once

#ifndef QEEXO_AUTOML_SENSOR_CONFIGS_H_
#define QEEXO_AUTOML_SENSOR_CONFIGS_H_

#include "QxAutoMLUser.h"

 char accel_buffer[163*6];
  char gyro_buffer[163*6];
 char magno_buffer[39*6];

const tQxAutoMLSensor enabled_sensors[] = {
    {
        QXSENSOR_TYPE_ACCEL,
        4.0f,
        417.0f,
        163,
        6,
 	   accel_buffer,
    },

    {
        QXSENSOR_TYPE_GYRO,
        500.0f,
        417.0f,
        163,
        6,
          gyro_buffer,
    },

    {
        QXSENSOR_TYPE_MAG,
        50.0f,
        100.0f,
        39,
        6,
		   magno_buffer,
    },

};

const int enabled_sensors_count = sizeof(enabled_sensors)/sizeof(enabled_sensors[0]);
#endif

/*
 * Copyright (c) 2022-2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Sensor driver for B-U585I-IOT02A

#include "sensor_drv.h"
#include "sensor_drv_hw.h"
#include "sensor_config.h"

#include "b_u585i_iot02a_env_sensors.h"
#include "b_u585i_iot02a_motion_sensors.h"

#include "ism330dhcx_fifo.h"
#include "audio_drv.h"

extern ISM330DHCX_Object_t ISM330DHCX_Obj;

#ifndef SENSOR_NO_LOCK
#include "cmsis_os2.h"

static uint8_t ISM330DHCX_ActiveFlags = 0U;

// Microphone 
#define MICROPHONE_SAMPLE_SIZE  SENSOR6_SAMPLE_SIZE
#define MICROPHONE_BLOCK_SIZE   SENSOR6_BLOCK_SIZE
#define MICROPHONE_BLOCK_NUM    SENSOR6_BLOCK_NUM
#define MICROPHONE_BUF_SIZE     MICROPHONE_SAMPLE_SIZE * MICROPHONE_BLOCK_SIZE * MICROPHONE_BLOCK_NUM

         static sensorId_t      Microphone_Id;
         static sensorEvent_t   Microphone_callback    = NULL;
         static uint32_t        Microphone_event_mask  = 0U;
         static uint32_t        Microphone_BlockIdxOut = 0U;
volatile static uint32_t        Microphone_BlockCntIn  = 0U;
volatile static uint32_t        Microphone_BlockCntOut = 0U;
         static uint8_t         Microphone_Buf[MICROPHONE_BUF_SIZE];

// Mutex lock
static osMutexId_t lock_id  = NULL;
static uint32_t    lock_cnt = 0U;

static inline void sensorLockCreate (void) {
  if (lock_cnt == 0U) {
    lock_id = osMutexNew(NULL);
  }
  lock_cnt++;
}
static inline void sensorLockDelete (void) {
  if (lock_cnt != 0U) {
    lock_cnt--;
    if (lock_cnt == 0U) {
      osMutexDelete(lock_id);
    }
  }
}
static inline void sensorLock (void) {
  osMutexAcquire(lock_id, osWaitForever);
}
static inline void sensorUnLock (void) {
  osMutexRelease(lock_id);
}
#else
static inline void sensorLockCreate (void) {}
static inline void sensorLockDelete (void) {}
static inline void sensorLock       (void) {}
static inline void sensorUnLock     (void) {}
#endif


// Temperature Sensor

static int32_t TemperatureSensor_Enable (void) {
  int32_t ret = SENSOR_ERROR;
  float   value;

  sensorLockCreate();
  sensorLock();
  if (BSP_ENV_SENSOR_Enable(0, ENV_TEMPERATURE) == BSP_ERROR_NONE) {
    BSP_ENV_SENSOR_GetValue(0, ENV_TEMPERATURE, &value);
    ret = SENSOR_OK;
  }
  sensorUnLock();

  return ret;
}

static int32_t TemperatureSensor_Disable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLock();
  if (BSP_ENV_SENSOR_Disable(0, ENV_TEMPERATURE) == BSP_ERROR_NONE) {
    ret = SENSOR_OK;
  }
  sensorUnLock();
  sensorLockDelete();

  return ret;
}

static uint32_t TemperatureSensor_GetOverflow (void) {
  return 0U;
}

static uint32_t TemperatureSensor_ReadSamples (uint32_t num_samples, void *buf) {
  uint32_t num = 0U;
  int32_t  ret;
  uint8_t  stat;
  float    value;

  (void)num_samples;

  sensorLock();
  ret = HTS221_TEMP_Get_DRDY_Status(Env_Sensor_CompObj[0], &stat);
  if ((ret == 0) && (stat != 0U)) {
    if (BSP_ENV_SENSOR_GetValue(0, ENV_TEMPERATURE, &value) == BSP_ERROR_NONE) {
      memcpy(buf, &value, sizeof(float));
      num = 1U;
    }
  }
  sensorUnLock();

  return num;
}

sensorDrvHW_t sensorDrvHW_0 = {
  NULL,
  TemperatureSensor_Enable,
  TemperatureSensor_Disable,
  TemperatureSensor_GetOverflow,
  TemperatureSensor_ReadSamples,
  NULL
};


// Humidity Sensor

static int32_t HumiditySensor_Enable (void) {
  int32_t ret = SENSOR_ERROR;
  float   value;

  sensorLockCreate();
  sensorLock();
  if (BSP_ENV_SENSOR_Enable(0, ENV_HUMIDITY) == BSP_ERROR_NONE) {
    BSP_ENV_SENSOR_GetValue(0, ENV_HUMIDITY, &value);
    ret = SENSOR_OK;
  }
  sensorUnLock();

  return ret;
}

static int32_t HumiditySensor_Disable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLock();
  if (BSP_ENV_SENSOR_Disable(0, ENV_HUMIDITY) == BSP_ERROR_NONE) {
    ret = SENSOR_OK;
  }
  sensorUnLock();
  sensorLockDelete();

  return ret;
}

static uint32_t HumiditySensor_GetOverflow (void) {
  return 0U;
}

static uint32_t HumiditySensor_ReadSamples (uint32_t num_samples, void *buf) {
  uint32_t num = 0U;
  int32_t  ret;
  uint8_t  stat;
  float    value;

  (void)num_samples;

  sensorLock();
  ret = HTS221_HUM_Get_DRDY_Status(Env_Sensor_CompObj[0], &stat);
  if ((ret == 0) && (stat != 0U)) {
    if (BSP_ENV_SENSOR_GetValue(0, ENV_HUMIDITY, &value) == BSP_ERROR_NONE) {
      memcpy(buf, &value, sizeof(float));
      num = 1U;
    }
  }
  sensorUnLock();

  return num;
}

sensorDrvHW_t sensorDrvHW_1 = {
  NULL,
  HumiditySensor_Enable,
  HumiditySensor_Disable,
  HumiditySensor_GetOverflow,
  HumiditySensor_ReadSamples,
  NULL
};


// Pressure Sensor

static int32_t PressureSensor_Enable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLockCreate();
  sensorLock();
  if (BSP_ENV_SENSOR_Enable(1, ENV_PRESSURE) == BSP_ERROR_NONE) {
    ret = SENSOR_OK;
  }
  sensorUnLock();

  return ret;
}

static int32_t PressureSensor_Disable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLock();
  if (BSP_ENV_SENSOR_Disable(1, ENV_PRESSURE) == BSP_ERROR_NONE) {
    ret = SENSOR_OK;
  }
  sensorUnLock();
  sensorLockDelete();

  return ret;
}

static uint32_t PressureSensor_GetOverflow (void) {
  return 0U;
}

static uint32_t PressureSensor_ReadSamples (uint32_t num_samples, void *buf) {
  uint32_t num = 0U;
  int32_t  ret;
  uint8_t  stat;
  float    value;

  (void)num_samples;

  sensorLock();
  ret = LPS22HH_PRESS_Get_DRDY_Status(Env_Sensor_CompObj[1], &stat);
  if ((ret == 0) && (stat != 0U)) {
    if (BSP_ENV_SENSOR_GetValue(1, ENV_PRESSURE, &value) == BSP_ERROR_NONE) {
      memcpy(buf, &value, sizeof(float));
      num = 1U;
    }
  }
  sensorUnLock();

  return num;
}

sensorDrvHW_t sensorDrvHW_2 = {
  NULL,
  PressureSensor_Enable,
  PressureSensor_Disable,
  PressureSensor_GetOverflow,
  PressureSensor_ReadSamples,
  NULL
};


// Accelerometer

static int32_t Accelerometer_Enable (void) {
  uint8_t sample[6];
  int32_t ret = SENSOR_ERROR;

  sensorLockCreate();
  sensorLock();
  if (ISM330DHCX_FIFO_Init(ISM330DHCX_ID_ACCELEROMETER) == 0) {
    if (ISM330DHCX_ActiveFlags == 0U) {
      // Clear ISM330DHCX FIFO
      while (ISM330DHCX_FIFO_Read(ISM330DHCX_ID_ACCELEROMETER, 1, sample) != 0U);
    }
    if (ISM330DHCX_ACC_Enable(&ISM330DHCX_Obj) == 0) {
      ISM330DHCX_ActiveFlags |= (1U << ISM330DHCX_ID_ACCELEROMETER);
      ret = SENSOR_OK;
    }
  }
  sensorUnLock();

  return ret;
}

static int32_t Accelerometer_Disable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLock();
  if (ISM330DHCX_ACC_Disable(&ISM330DHCX_Obj) == 0) {
    if (ISM330DHCX_FIFO_Uninit(ISM330DHCX_ID_ACCELEROMETER) == 0) {
      ISM330DHCX_ActiveFlags &= ~(1U << ISM330DHCX_ID_ACCELEROMETER);
      ret = SENSOR_OK;
    }
  }
  sensorUnLock();
  sensorLockDelete();

  return ret;
}

static uint32_t Accelerometer_GetOverflow (void) {
  return 0U;
}

static uint32_t Accelerometer_ReadSamples (uint32_t num_samples, void *buf) {
  uint32_t num;

  sensorLock();
  num = ISM330DHCX_FIFO_Read(ISM330DHCX_ID_ACCELEROMETER, num_samples, buf);
  sensorUnLock();

  return num;
}

sensorDrvHW_t sensorDrvHW_3 = {
  NULL,
  Accelerometer_Enable,
  Accelerometer_Disable,
  Accelerometer_GetOverflow,
  Accelerometer_ReadSamples,
  NULL
};


// Gyroscope

static int32_t Gyroscope_Enable (void) {
  uint8_t sample[6];
  int32_t ret = SENSOR_ERROR;

  sensorLockCreate();
  sensorLock();
  if (ISM330DHCX_FIFO_Init(ISM330DHCX_ID_GYROSCOPE) == 0) {
    if (ISM330DHCX_ActiveFlags == 0U) {
      // Clear ISM330DHCX FIFO
      while (ISM330DHCX_FIFO_Read(ISM330DHCX_ID_GYROSCOPE, 1, sample) != 0U);
    }
    if (ISM330DHCX_GYRO_Enable(&ISM330DHCX_Obj) == 0) {
      ISM330DHCX_ActiveFlags |= (1U << ISM330DHCX_ID_GYROSCOPE);
      ret = SENSOR_OK;
    }
  }
  sensorUnLock();

  return ret;
}

static int32_t Gyroscope_Disable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLock();
  if (BSP_MOTION_SENSOR_Disable(0, MOTION_GYRO) == BSP_ERROR_NONE) {
    if (ISM330DHCX_FIFO_Uninit(ISM330DHCX_ID_GYROSCOPE) == 0) {
      ISM330DHCX_ActiveFlags &= ~(1U << ISM330DHCX_ID_GYROSCOPE);
      ret = SENSOR_OK;
    }
  }
  sensorUnLock();
  sensorLockDelete();

  return ret;
}

static uint32_t Gyroscope_GetOverflow (void) {
  return 0U;
}

static uint32_t Gyroscope_ReadSamples (uint32_t num_samples, void *buf) {
  uint32_t num;

  sensorLock();
  num = ISM330DHCX_FIFO_Read(ISM330DHCX_ID_GYROSCOPE, num_samples, buf);
  sensorUnLock();

  return num;
}

sensorDrvHW_t sensorDrvHW_4 = {
  NULL,
  Gyroscope_Enable,
  Gyroscope_Disable,
  Gyroscope_GetOverflow,
  Gyroscope_ReadSamples,
  NULL
};


// Magnetometer

static int32_t Magnetometer_Enable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLockCreate();
  sensorLock();
  if (BSP_MOTION_SENSOR_Enable(1, MOTION_MAGNETO) == BSP_ERROR_NONE) {
    ret = SENSOR_OK;
  }
  sensorUnLock();

  return ret;
}

static int32_t Magnetometer_Disable (void) {
  int32_t ret = SENSOR_ERROR;

  sensorLock();
  if (BSP_MOTION_SENSOR_Disable(1, MOTION_MAGNETO) == BSP_ERROR_NONE) {
    ret = SENSOR_OK;
  }
  sensorUnLock();
  sensorLockDelete();

  return ret;
}

static uint32_t Magnetometer_GetOverflow (void) {
  return 0U;
}

static uint32_t Magnetometer_ReadSamples (uint32_t num_samples, void *buf) {
  uint32_t num = 0U;
  int32_t  ret;
  uint8_t  stat;
  BSP_MOTION_SENSOR_AxesRaw_t axes;

  (void)num_samples;

  sensorLock();
  ret = IIS2MDC_MAG_Get_DRDY_Status(Motion_Sensor_CompObj[1], &stat);
  if ((ret == 0) && (stat != 0U)) {
    if (BSP_MOTION_SENSOR_GetAxesRaw(1, MOTION_MAGNETO, &axes) == BSP_ERROR_NONE) {
      memcpy(buf, &axes, sizeof(BSP_MOTION_SENSOR_AxesRaw_t));
      num = 1U;
    }
  }
  sensorUnLock();

  return num;
}

sensorDrvHW_t sensorDrvHW_5 = {
  NULL,
  Magnetometer_Enable,
  Magnetometer_Disable,
  Magnetometer_GetOverflow,
  Magnetometer_ReadSamples,
  NULL
};

// Audio microphone
void AudioDrv_Callback (uint32_t event) {
  uint32_t ev = 0U;

  if ((event & AUDIO_DRV_EVENT_RX_DATA) != 0U) {
    Microphone_BlockCntIn++;
    if ((Microphone_BlockCntIn - Microphone_BlockCntOut) >= (MICROPHONE_BLOCK_NUM)) {
      if ((Microphone_event_mask & SENSOR_EVENT_OVERFLOW) != 0U) {
        event |= SENSOR_EVENT_OVERFLOW;
      }
    }
    if ((Microphone_event_mask & SENSOR_EVENT_DATA) != 0U) {
      event |= SENSOR_EVENT_DATA;
    }
    if ((ev != 0U) && (Microphone_callback != NULL)) {
      Microphone_callback(Microphone_Id, ev);
    }
  }
}

static int32_t Microphone_RegisterEvents (sensorId_t id, sensorEvent_t event_cb, uint32_t event_mask) {
  Microphone_Id         = id;
  Microphone_callback   = event_cb;
  Microphone_event_mask = event_mask;

  return SENSOR_OK;
}

static int32_t Microphone_Enable (void) {
  int32_t ret;

  ret = AudioDrv_SetBuf(AUDIO_DRV_INTERFACE_RX,
                        Microphone_Buf, 
                        MICROPHONE_BLOCK_NUM,
                        MICROPHONE_BLOCK_SIZE / MICROPHONE_SAMPLE_SIZE);
  
  if (ret == AUDIO_DRV_OK) {
    ret = AudioDrv_Control(AUDIO_DRV_CONTROL_RX_ENABLE);
  }

  if (ret == AUDIO_DRV_OK) {
    ret = SENSOR_OK;
  } else {
    ret = SENSOR_ERROR;
  }

  return ret;
}

static int32_t Microphone_Disable (void) {
  int32_t ret = SENSOR_ERROR;

  if (AudioDrv_Control(AUDIO_DRV_CONTROL_RX_DISABLE) == AUDIO_DRV_OK) {
    ret = SENSOR_OK;
  }

  return ret;
}

static uint32_t Microphone_GetOverflow (void) {
  return 0U;
}

static void *Microphone_GetBlockData (void) {
  void *block = NULL;

  if ((Microphone_BlockCntIn - Microphone_BlockCntOut) > 0) {
    block = Microphone_Buf + (Microphone_BlockIdxOut * MICROPHONE_BLOCK_SIZE);

    Microphone_BlockIdxOut++;
    if(Microphone_BlockIdxOut == MICROPHONE_BLOCK_NUM) {
      Microphone_BlockIdxOut = 0;
    }
    Microphone_BlockCntOut++;
  }
  return block;
}

sensorDrvHW_t sensorDrvHW_6 = {
  Microphone_RegisterEvents,
  Microphone_Enable,
  Microphone_Disable,
  Microphone_GetOverflow,
  NULL,
  Microphone_GetBlockData
};

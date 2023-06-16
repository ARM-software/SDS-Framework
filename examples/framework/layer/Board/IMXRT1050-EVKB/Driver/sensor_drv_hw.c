/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
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

// Sensor driver for IMXRT1050-EVKB

#include "sensor_drv.h"
#include "sensor_drv_hw.h"
#include "sensor_config.h"

#include "fsl_common.h"
#include "fsl_fxos.h"

#ifndef SENSOR_NO_LOCK
#include "cmsis_os2.h"

extern fxos_handle_t g_fxosHandle;

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


// Accelerometer

static int32_t Accelerometer_Enable (void) {
  int32_t ret = SENSOR_ERROR;
  uint8_t tmp = 0U;

  sensorLockCreate();
  sensorLock();
  // Setup the Active mode
  FXOS_WriteReg(&g_fxosHandle, CTRL_REG1, ACTIVE);
  // Read Control register to ensure we are in Active mode
  FXOS_ReadReg(&g_fxosHandle, CTRL_REG1, &tmp, 1U);
  if ((tmp & ACTIVE_MASK) == ACTIVE) {
    ret = SENSOR_OK;
  }
  sensorUnLock();

  return ret;
}

static int32_t Accelerometer_Disable (void) {
  int32_t ret = SENSOR_ERROR;
  uint8_t tmp = 0U;

  sensorLock();
  // Setup the Standby mode
  FXOS_WriteReg(&g_fxosHandle, CTRL_REG1, STANDBY);
  // Read Control register to ensure we are in Standby mode
  FXOS_ReadReg(&g_fxosHandle, CTRL_REG1, &tmp, 1U);
  if ((tmp & ACTIVE_MASK) == STANDBY) {
    ret = SENSOR_OK;
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
  int32_t  ret;
  uint8_t tmp = 0U;
  uint8_t tmp_buff[6];

  (void)num_samples;

  sensorLock();
  ret = FXOS_ReadReg(&g_fxosHandle, F_STATUS_REG, &tmp, 1U);
  if ((ret == 0) && ((tmp & ZYXDR_MASK) != 0U)) {
    if ((FXOS_ReadReg(&g_fxosHandle, OUT_X_MSB_REG, tmp_buff, 6U)) == kStatus_Success) {
      memcpy(buf, &tmp_buff, sizeof(tmp_buff));
      num = 1U;
    }
  }
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


// Magnetometer

static int32_t Magnetometer_Enable (void) {
  int32_t ret = SENSOR_ERROR;
  uint8_t tmp = 0U;

  sensorLockCreate();
  sensorLock();

  // Read CTRL_REG1 register to ensure if Acelerometer sensor is active
  FXOS_ReadReg(&g_fxosHandle, CTRL_REG1, &tmp, 1U);
  if ((tmp & M_HMS0_MASK) != 0U) {
    // Setup the Hybrid mode (Acelerometer and Magnetometer)
    FXOS_WriteReg(&g_fxosHandle, M_CTRL_REG1, (M_RST_MASK | M_OSR_MASK | M_HMS_MASK));
    FXOS_WriteReg(&g_fxosHandle, M_CTRL_REG2, (M_HYB_AUTOINC_MASK));
    ret = SENSOR_OK;
  }
  else   if ((tmp & M_HMS0_MASK) == 0U) {
    // Setup only Magnetometer sensor as active
    FXOS_WriteReg(&g_fxosHandle, M_CTRL_REG1, (M_RST_MASK | M_OSR_MASK | M_HMS1_MASK));
    ret = SENSOR_OK;
  }
  sensorUnLock();

  return ret;
}

static int32_t Magnetometer_Disable (void) {
  int32_t ret = SENSOR_ERROR;
  uint8_t tmp = 0U;

  sensorLock();
  // Read CTRL_REG1 register to ensure if Acelerometer/Magnetometer sensor is active
  FXOS_ReadReg(&g_fxosHandle, CTRL_REG1, &tmp, 1U);
  if ((tmp & M_HMS_MASK) != 0U) {
    // Acelerometer sensor stay active
    FXOS_ReadReg(&g_fxosHandle, M_CTRL_REG1, &tmp, 1U);
    tmp &= ~(1UL << 0U);
    tmp &= ~(1UL << 1U);
    FXOS_WriteReg(&g_fxosHandle, M_CTRL_REG1, tmp);
    ret = SENSOR_OK;
  }
  else if ((tmp & M_HMS1_MASK) != 0U) {
    // Setup the Standby mode
    FXOS_WriteReg(&g_fxosHandle, CTRL_REG1, STANDBY);
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
  uint32_t num;
  int32_t  ret;
  uint8_t tmp = 0U;
  uint8_t tmp_buff[6];

  (void)num_samples;

  sensorLock();
  ret = FXOS_ReadReg(&g_fxosHandle, M_DR_STATUS_REG, &tmp, 1U);
  if ((ret == 0) && ((tmp & ZYXDR_MASK) != 0U)) {
    if ((FXOS_ReadReg(&g_fxosHandle, M_OUT_X_MSB_REG, tmp_buff, 6U)) == kStatus_Success) {
      memcpy(buf, &tmp_buff, sizeof(tmp_buff));
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

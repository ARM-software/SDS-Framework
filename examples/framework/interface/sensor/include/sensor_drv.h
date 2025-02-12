/*
 * Copyright (c) 2022-2025 Arm Limited. All rights reserved.
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

#ifndef SENSOR_DRV_H_
#define SENSOR_DRV_H_

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define SensorDrv_(name) SensorDrv_##name
#define SensorDrv(name)  SensorDrv_(name)

// ==== Sensor Driver Interface ====

/// \brief       Macro to get data size (in bytes).
/// \param[in]   dataType  Type of the sensor data (SENSOR_DRV_DATA_xxx).
/// \return      Data size (in bytes) for the specified data type.
#define SENSOR_DRV_DATA_SIZE(dataType)         \
  (dataType == SENSOR_DRV_DATA_INT8   ? 1U :   \
   dataType == SENSOR_DRV_DATA_UINT8  ? 1U :   \
   dataType == SENSOR_DRV_DATA_INT16  ? 2U :   \
   dataType == SENSOR_DRV_DATA_UINT16 ? 2U :   \
   dataType == SENSOR_DRV_DATA_INT32  ? 4U :   \
   dataType == SENSOR_DRV_DATA_UINT32 ? 4U :   \
   dataType == SENSOR_DRV_DATA_FLOAT  ? 4U : 0U)

/// Data type definitions
#define SENSOR_DRV_DATA_INT8    1U              ///<  8-bit signed integer
#define SENSOR_DRV_DATA_UINT8   2U              ///<  8-bit unsigned integer
#define SENSOR_DRV_DATA_INT16   3U              ///< 16-bit signed integer
#define SENSOR_DRV_DATA_UINT16  4U              ///< 16-bit unsigned integer
#define SENSOR_DRV_DATA_INT32   5U              ///< 32-bit signed integer
#define SENSOR_DRV_DATA_UINT32  6U              ///< 32-bit unsigned integer
#define SENSOR_DRV_DATA_FLOAT   7U              ///< 32-bit floating point

/// Sensor Information.
typedef const struct {
  char         *name;                           ///< Sensor name
  uint16_t      channels;                       ///< Number of channels (e.g.: thermometer - 1 ch, accelerometer - 3 ch (x,y,z))
  uint16_t      data_type;                      ///< Sensor data type (SENSOR_DRV_DATA_xxx)
  float         data_rate;                      ///< Data rate in Hz
  float         scale;                          ///< Conversion multiplier used to translate raw data into measurement units
  float         offset;                         ///< Offset (in measurement units)
  char         *unit;                           ///< Measurement unit (recommended: https://www.iana.org/assignments/senml/senml.xhtml)
  uint32_t      fifo_size;                      ///< FIFO size in bytes
  uint32_t      threshold;                      ///< Data event threshold in bytes
} SensorDrv_Info_t;

/// Sensor status flags.
typedef struct {
    uint32_t active   : 1;                      ///< Active state: 1 = active (Capture started), 0 = inactive (Capture stopped)
    uint32_t overflow : 1;                      ///< Overflow flag (cleared on read)
    uint32_t reserved : 30;
} SensorDrv_Status_t;

/// Function return codes
#define SENSOR_DRV_OK               (0)         ///< Operation completed successfully
#define SENSOR_DRV_ERROR            (-1)        ///< Operation failed
#define SENSOR_DRV_UNSUPPORTED      (-2)        ///< Operation not supported

/// Events
#define SENSOR_DRV_EVENT_DATA       (1UL << 0)  ///< Data available. Triggered when data threshold is reached or exceeded
#define SENSOR_DRV_EVENT_OVERFLOW   (1UL << 1)  ///< Data overflow detected

/// \brief       Sensor Events callback function type
/// \param[in]   event  Events notification mask.
typedef void (*SensorDrv_Event_t) (uint32_t event);

/// \fn          int32_t Initialize (SensorDrv_Event_t event_cb, uint32_t event_mask)
/// \brief       Initializes the sensor driver and registers a callback function for sensor events.
/// \param[in]   event_cb    Pointer to the callback function (\ref SensorDrv_Event_t).
/// \param[in]   event_mask  Event mask specifying the events to be notified (SENSOR_DRV_EVENT_x).
/// \return      Return code.

/// \fn          int32_t Uninitialize (void)
/// \brief       Un-initializes the sensor driver.
/// \return      Return code.

/// \fn          SensorDrv_Info_t *GetInfo (void)
/// \brief       Gets sensor information.
/// \return      Pointer to the sensor information structure - \ref SensorDrv_Info_t.

/// \fn          int32_t CaptureStart (void)
/// \brief       Starts capturing sensor data.
/// \return      Return code.

/// \fn          int32_t CaptureStop (void)
/// \brief       Stops capturing sensor data.
/// \return      Return code.

/// \fn          SensorDrv_Status_t GetStatus (void)
/// \brief       Returns sensor's current status.
/// \return      The sensor status - \ref SensorDrv_Status_t.

/// \fn          uint32_t Read (void *buf, uint32_t buf_size)
/// \brief       Reads data from the sensor into the provided buffer.
/// \param[out]  buf         Pointer to the buffer to store the read data.
/// \param[in]   buf_size    Buffer size in bytes.
/// \return      The number of bytes successfully read.

/// Sensor driver access structure.
typedef struct {
  int32_t            (*Initialize)       (SensorDrv_Event_t event_cb, uint32_t event_mask); ///< Pointer to \ref Initialize : Initializes the sensor driver.
  int32_t            (*Uninitialize)     (void);                                            ///< Pointer to \ref Uninitialize : Un-initializes the sensor driver.
  SensorDrv_Info_t  *(*GetInfo)          (void);                                            ///< Pointer to \ref GetInfo : Gets sensor information.
  int32_t            (*CaptureStart)     (void);                                            ///< Pointer to \ref CaptureStart : Starts capturing sensor data.
  int32_t            (*CaptureStop)      (void);                                            ///< Pointer to \ref CaptureStop : Stops capturing sensor data.
  SensorDrv_Status_t (*GetStatus)        (void);                                            ///< Pointer to \ref GetStatus : Returns sensor's current status.
  uint32_t           (*Read)             (void *buf, uint32_t buf_size);                    ///< Pointer to \ref Read : Reads data from the sensor into the provided buffer.
} const SensorDrv_t;

#ifdef  __cplusplus
}
#endif

#endif  /* SENSOR_DRV_H_ */

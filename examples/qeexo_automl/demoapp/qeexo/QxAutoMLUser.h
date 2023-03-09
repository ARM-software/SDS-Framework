#pragma once

#ifndef MIDDLEWARES_QEEXO_INCLUDE_APPS_QXAUTOMLCONFIG_USER_H_
#define MIDDLEWARES_QEEXO_INCLUDE_APPS_QXAUTOMLCONFIG_USER_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 * SENSOR ENABLE DEFINITIONS
 ************************************************/
 #define QXAUTOMLCONFIG_SENSOR_ENABLE_ACCEL
 #define QXAUTOMLCONFIG_SENSOR_ENABLE_GYRO
 #define QXAUTOMLCONFIG_SENSOR_ENABLE_MAG

#ifdef QXAUTOMLCONFIG_SENSOR_ENABLE_ACCEL
#define QXAUTOMLCONFIG_SENSOR_ACCEL_FSR  4.0f
#define QXAUTOMLCONFIG_SENSOR_ACCEL_ODR  417.0f
#define QXAUTOMLCONFIG_SENSOR_ACCEL_BUFCOUNT  163
#define QXAUTOMLCONFIG_SENSOR_ACCEL_BYTES_PER_SAMPLE  6
#endif

#ifdef QXAUTOMLCONFIG_SENSOR_ENABLE_GYRO
#define QXAUTOMLCONFIG_SENSOR_GYRO_FSR  500.0f
#define QXAUTOMLCONFIG_SENSOR_GYRO_ODR  417.0f
#define QXAUTOMLCONFIG_SENSOR_GYRO_BUFCOUNT  163
#define QXAUTOMLCONFIG_SENSOR_GYRO_BYTES_PER_SAMPLE  6
#endif

#ifdef QXAUTOMLCONFIG_SENSOR_ENABLE_MAG
#define QXAUTOMLCONFIG_SENSOR_MAG_FSR  50.0f //50gauss
#define QXAUTOMLCONFIG_SENSOR_MAG_ODR  100.0f //100hz
#define QXAUTOMLCONFIG_SENSOR_MAG_BUFCOUNT  39
#define QXAUTOMLCONFIG_SENSOR_MAG_BYTES_PER_SAMPLE  6
#endif

/* sensor type definitions inside inference Engine. 
 * Customer should define the sensor index as the same sequence regarding to 
 * fill the prediction data buffer with multiple sensors*/
typedef enum {
  QXSENSOR_TYPE_NONE = 0, /*!< None defined sensor */
  QXSENSOR_TYPE_ACCEL, /*!< Default accelerometer sensor */
  QXSENSOR_TYPE_GYRO, /*!< Default gyroscope sensor */
  QXSENSOR_TYPE_MAG, /*!< Megnotometer sensor */
  QXSENSOR_TYPE_PRESSURE, /*!< Pressure sensor */
  QXSENSOR_TYPE_TEMPERATURE, /*!< Temperature sensor */
  QXSENSOR_TYPE_HUMIDITY, /*!< Humidity sensor */
  QXSENSOR_TYPE_MICROPHONE, /*!< Microphone sensor */
  
  QXSENSOR_TYPE_ACCEL_LOWPOWER, /*!< Additional lowpower accelometer sensor */
  QXSENSOR_TYPE_ACCEL_HIGHSENSITIVE, /*!< Additional high sensitive accelometer sensor */
  QXSENSOR_TYPE_TEMPERATURE_EXT1, /*!< Additional temperature sensor */
  QXSENSOR_TYPE_PROXIMITY, /*!< Proximity sensor */
  QXSENSOR_TYPE_AMBIENT, /*!< Ambient light sensor */
  QXSENSOR_TYPE_LIGHT,  /*!<Light sensor with one axis>*/

  QXSENSOR_TYPE_TVOC, /*!< ZMOD gas sensor (TVOC) */
  QXSENSOR_TYPE_ECO2, /*!< ZMOD gas sensor (ECO2) */
  QXSENSOR_TYPE_ETOH, /*!< ZMOD gas sensor (ETOH) */
  QXSENSOR_TYPE_RCDA, /*!< ZMOD gas sensor (RCDA) */
  QXSENSOR_TYPE_IAQ, /*!< ZMOD gas sensor (IAQ) */
  QXSENSOR_TYPE_RMOX, /*!< ZMOD gas sensor (MOx array) */
  QXSENSOR_TYPE_CURRENT,  /*!<Current sensor with one axis>*/
  QXSENSOR_TYPE_MAX
}QXOSensorType;

typedef struct{
  QXOSensorType type;
  float fsr;
  float odr;
  int buf_count;
  int sample_bytes;
  char * buf;
}tQxAutoMLSensor;

/* Qeexo static engine APIs */
extern void QxFillSensorData(QXOSensorType type, void* data, int data_len);
extern int QxClassify(void);
extern float *QxGetProbabilities(int *numOfClasses);

extern const tQxAutoMLSensor enabled_sensors[];
extern const int enabled_sensors_count;

/* Desired period of time between the start of the previous classification and the next request for classification (in msecs) */
#define PRED_CLASSIFICATION_INTERVAL_IN_MSECS 100

#ifdef __cplusplus
}
#endif

#endif /* MIDDLEWARES_QEEXO_INCLUDE_APPS_QXAUTOMLCONFIG_USER_H_ */

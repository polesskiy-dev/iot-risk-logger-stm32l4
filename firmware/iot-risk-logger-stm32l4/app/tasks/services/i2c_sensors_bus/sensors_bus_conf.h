#ifndef SENSORS_BUS_CONF_H
#define SENSORS_BUS_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef I2C_BUS_SERVICE_QUEUE_DEPTH
#define I2C_BUS_SERVICE_QUEUE_DEPTH    8U
#endif

#ifndef I2C_BUS_SERVICE_TASK_STACK
#define I2C_BUS_SERVICE_TASK_STACK     512U
#endif

#ifndef I2C_BUS_SERVICE_TASK_PRIO
#define I2C_BUS_SERVICE_TASK_PRIO      osPriorityAboveNormal
#endif

#ifndef I2C_BUS_SERVICE_FLAG_DONE
#define I2C_BUS_SERVICE_FLAG_DONE      (1U << 0)
#endif

#ifdef __cplusplus
}
#endif

#endif //SENSORS_BUS_CONF_H
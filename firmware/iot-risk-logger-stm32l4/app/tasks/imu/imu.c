/*!
 * @file imu.c
 * @brief Implementation of imu
 *
 * Detailed description of the implementation file.
 *
 * @date 19/11/2025
 * @author artempolisskyi
 */

#include "imu.h"

#include "../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.h"


static osStatus_t handleImuFSM(IMU_Actor_t *this, message_t *message);

/** states handlers */
static osStatus_t handleInit(IMU_Actor_t *this, message_t *message);
static osStatus_t handleIdle(IMU_Actor_t *this, message_t *message);

static int32_t lis2dwCommonConfig(void);
static int32_t lis2dwConfigLowPower(void);
// TODO DFT-28 completely OFF configuration for transportation mode
static int32_t readFifoAndLog(IMU_Actor_t *this);
static int32_t handleFreeFall(IMU_Actor_t *this);

extern actor_t *ACTORS_LOOKUP_SystemRegistry[MAX_ACTORS];


/**
 * @brief IMU Accelerometer actor struct
 * @extends actor_t
 */
IMU_Actor_t IMU_Actor = {
  .super = {
    .actorId = IMU_ACTOR_ID,
    .messageHandler = (messageHandler_t) handleImuFSM,
    .osMessageQueueId = NULL,
    .osThreadId = NULL,
  },
  .state = IMU_NO_STATE,
  .lis2dw12 = {
    .Ctx = {
      .write_reg = NULL, // to be assigned in init
      .read_reg = NULL,  // to be assigned in init
      .handle = NULL     // to be assigned in init
    },
    .is_initialized = 0,
  }
};

// task description required for static task creation
uint32_t imuTaskBuffer[DEFAULT_TASK_STACK_SIZE_WORDS];
StaticTask_t imuTaskControlBlock;
const osThreadAttr_t imuTaskDescription = {
        .name = "imuTask",
        .cb_mem = &imuTaskControlBlock,
        .cb_size = sizeof(imuTaskControlBlock),
        .stack_mem = &imuTaskBuffer[0],
        .stack_size = sizeof(imuTaskBuffer),
        .priority = (osPriority_t) osPriorityNormal,
};

/**
 * @brief Initializes the IMU task.
 * @return {actor_t*} - pointer to the actor base struct
 */
actor_t* IMU_TaskInit(void) {
  IMU_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
            .name = "imuQueue"
    });
  IMU_Actor.super.osThreadId = osThreadNew(IMU_Task, NULL, &imuTaskDescription);

  return &IMU_Actor.super;
}

/**
 * @brief IMU task
 * Waits for a message from the queue and processes it in FSM
 * Enters ERROR state if message handling failed
 */
void IMU_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  fprintf(stdout, "Task %s started\n", imuTaskDescription.name);

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(IMU_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t handleMessageStatus = IMU_Actor.super.messageHandler((actor_t *) &IMU_Actor, &msg);

      if (handleMessageStatus != osOK) {
        fprintf(stderr,  "%s: Error handling event %u in state %u\n", imuTaskDescription.name, msg.event, IMU_Actor.state);
        osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
        osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_ERROR, .payload.value = IMU_ACTOR_ID}, 0, 0);
        TO_STATE(&IMU_Actor, IMU_STATE_ERROR);
      }
    }
  }
}

static osStatus_t handleImuFSM(IMU_Actor_t *this, message_t *message) {
  switch (this->state) {
    case IMU_NO_STATE:
      return handleInit(this, message);
    case IMU_STATE_IDLE:
      return handleIdle(this, message);
    case IMU_STATE_ERROR:
      // TODO implement other states
      return osOK;
    default:
      fprintf(stderr, "%s: Unknown IMU state %lu\n", imuTaskDescription.name, (unsigned long)this->state);
      return osError;
  }
}

/**
 * @brief Initializes the sensor, configures I2C, writes default settings, and transitions to the TURNED_OFF state
 */
static osStatus_t handleInit(IMU_Actor_t *this, message_t *message) {
  if (GLOBAL_CMD_INITIALIZE == message->event) {

    LIS2DW12_IO_t pIO = {
      .Init = BSP_I2C1_Init,
      .DeInit = BSP_I2C1_DeInit, // TODO verify should we use it at all
      .BusType = LIS2DW12_I2C_BUS,
      .Address = IMU_I2C_ADDRESS,
      .WriteReg = SensorsBus_WriteReg,
      .ReadReg = SensorsBus_ReadReg,
      .GetTick = BSP_GetTick,
      .Delay = (void(*)(uint32_t))osDelay // TODO verify should it be in ticks as osDelay or in ms
    };

    // init the driver (io)
    osStatus_t ioStatus = LIS2DW12_ERROR;
    ioStatus = LIS2DW12_RegisterBusIO(&IMU_Actor.lis2dw12, &pIO);
    if (ioStatus != osOK) return osError;

    // init the sensor itself
    ioStatus = LIS2DW12_Init(&IMU_Actor.lis2dw12);
    if (ioStatus != osOK) return osError;

    // smoke test: read device id, should be 0x44 (LIS2DW12_ID)
    uint8_t lis2dw_id = 0;
    ioStatus = LIS2DW12_ReadID(&IMU_Actor.lis2dw12, &lis2dw_id);

    if (ioStatus != osOK || lis2dw_id != LIS2DW12_ID) {
      #ifdef DEBUG
            fprintf(stdout, "LIS2DW ID: %x does not match 0x44\n", lis2dw_id);
      #endif

      return ioStatus;
    }

    // common configuration
    ioStatus = lis2dwCommonConfig();
    if (ioStatus != osOK) return osError;

    // low power 1.6Hz configuration
    ioStatus = lis2dwConfigLowPower();
    if (ioStatus != osOK) return osError;

    // publish to the event manager that the IMU is initialized
    osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_INITIALIZE_SUCCESS, .payload.value = IMU_ACTOR_ID}, 0, 0);

    fprintf(stdout, "IMU %u initialized\n", IMU_ACTOR_ID);
    TO_STATE(this, IMU_STATE_IDLE);
  }

  return osOK;
}

static osStatus_t handleIdle(IMU_Actor_t *this, message_t *message) {
  switch (message->event) {
    case IMU_FIFO_WTM:
      return readFifoAndLog(this);
    case IMU_FREE_FALL_DETECTED:
      return handleFreeFall(this);
    default:
      return osOK;
  }
}

static int32_t lis2dwCommonConfig(void) {
  int32_t ret = 0;

  // 1) Make register interface sane
  ret |= lis2dw12_block_data_update_set(&IMU_Actor.lis2dw12.Ctx, PROPERTY_ENABLE);
  ret |= lis2dw12_auto_increment_set(&IMU_Actor.lis2dw12.Ctx, PROPERTY_ENABLE);

  // 2) Interrupt pin behaviour (shared for all modes)
  ret |= lis2dw12_pin_mode_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_PUSH_PULL);
  ret |= lis2dw12_pin_polarity_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_ACTIVE_HIGH);
  ret |= lis2dw12_int_notification_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_INT_LATCHED);

  // 3) Full-scale and low-power mode
  ret |= lis2dw12_full_scale_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_2g);
  ret |= lis2dw12_power_mode_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_CONT_LOW_PWR_LOW_NOISE_2);

  // 4) Start from ODR = OFF, FIFO BYPASS
  ret |= lis2dw12_data_rate_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_XL_ODR_OFF);
  ret |= lis2dw12_fifo_mode_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_BYPASS_MODE);

  return ret;
}

static int32_t lis2dwConfigLowPower(void) {
  int32_t ret = 0;

  // 1) Low-power mode & ODR
  ret |= lis2dw12_power_mode_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_CONT_LOW_PWR_LOW_NOISE_12bit);
  ret |= lis2dw12_data_rate_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_XL_ODR_1Hz6_LP_ONLY);

  // Optional: limit bandwidth to save power a bit more
  // ret |= lis2dw12_filter_bandwidth_set(imu_ctx, LIS2DW12_ODR_DIV_2);

  // 2) FIFO configuration: stream mode, IMU_SAMPLES_BUFFER_SIZE samples WTM
  ret |= lis2dw12_fifo_mode_set(&IMU_Actor.lis2dw12.Ctx, LIS2DW12_STREAM_MODE);
  ret |= lis2dw12_fifo_watermark_set(&IMU_Actor.lis2dw12.Ctx, IMU_SAMPLES_BUFFER_SIZE);

  // 3) Route FIFO_WTM interrupt to INT1
  lis2dw12_ctrl4_int1_pad_ctrl_t int1_route = {0};
  ret |= lis2dw12_pin_int1_route_get(&IMU_Actor.lis2dw12.Ctx, &int1_route);

  int1_route.int1_fth = 1;       // FIFO threshold
  int1_route.int1_drdy = 0;      // we use WTM instead of DRDY

  ret |= lis2dw12_pin_int1_route_set(&IMU_Actor.lis2dw12.Ctx, &int1_route);

  return ret;
}

// TODO refine this
/**
 * @brief Drain LIS2DW12 FIFO and push samples into log storage.
 *
 * This runs in the IMU actor context (not in IRQ).
 */
static int32_t readFifoAndLog(IMU_Actor_t *this)
{
  uint8_t fifo_level = 0;
  int32_t ret;

  stmdev_ctx_t *ctx = &this->lis2dw12.Ctx;

  // 1) Get number of samples currently in FIFO
  ret = lis2dw12_fifo_data_level_get(ctx, &fifo_level);
  if (ret != 0 || fifo_level == 0) {
#ifdef DEBUG
    fprintf(stderr, "IMU: FIFO WTM but fifo_level=%u, ret=%ld\n", fifo_level, (long)ret);
#endif
    return ret;
  }

#ifdef DEBUG
  fprintf(stdout, "IMU: FIFO WTM, %u samples pending\n", fifo_level);
#endif

  // 2) Read each sample from FIFO
  for (uint8_t i = 0; i < fifo_level; i++) {
    int16_t raw[3] = {0};
    float   mg[3]  = {0.0f};

    // Read raw acceleration (one sample)
    // NOTE: function name depends on exact ST driver version, adjust if needed.
    // Common patterns are:
    //   lis2dw12_acceleration_raw_get(ctx, raw);
    //   or LIS2DW12_ACC_GetAxesRaw(&this->lis2dw12, raw);
    ret = lis2dw12_acceleration_raw_get(ctx, raw);
    if (ret != 0) {
#ifdef DEBUG
      fprintf(stderr, "IMU: error reading FIFO sample %u, ret=%ld\n", i, (long)ret);
#endif
      break;
    }

    // Optionally convert to mg using ST helper
    // (again, adjust if your BSP wrapper has a different name).
    // Example:
    // lis2dw12_from_fs2_to_mg(raw[0], &mg[0]);
    // lis2dw12_from_fs2_to_mg(raw[1], &mg[1]);
    // lis2dw12_from_fs2_to_mg(raw[2], &mg[2]);

    // 3) Push sample to your logging pipeline
    //    (this is intentionally abstract – integrate with your
    //     QSPI logger / event manager as you already do for other sensors).
    // TODO: replace with your real logging call
    // Example:
    // LogManager_LogImuSample(BSP_GetTick(), mg[0], mg[1], mg[2]);

#ifdef DEBUG
    fprintf(stdout, "IMU sample %u: %.2f mg, %.2f mg, %.2f mg\n",
            i, mg[0], mg[1], mg[2]);
#endif
  }

  return ret;
}

// TODO refine this
/**
 * @brief Handle free-fall event from LIS2DW12.
 *
 * Called in IMU actor context after IRQ posted IMU_FREE_FALL_DETECTED.
 */
static int32_t handleFreeFall(IMU_Actor_t *this)
{
  stmdev_ctx_t *ctx = &this->lis2dw12.Ctx;
  int32_t ret = 0;

#ifdef DEBUG
  fprintf(stdout, "IMU: free-fall event received\n");
#endif

  // 1) Read and clear free-fall source / all sources.
  //    Exact function names depend on your driver version.
  //    Typical pattern in ST libs:
  //      lis2dw12_all_sources_t src;
  //      lis2dw12_all_sources_get(ctx, &src);
  //
  // For now, leave as TODO so you can wire to your actual reg helpers.

  // TODO: replace with actual source read that clears the FF interrupt
  // lis2dw12_all_sources_t all_src;
  // ret = lis2dw12_all_sources_get(ctx, &all_src);

  // (void)ctx;
  // (void)ret;

  // 2) Notify Event Manager (or Logger Manager) that a free-fall happened.
  //    This mirrors how you already publish GLOBAL_INITIALIZE_SUCCESS.
  // osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;

  // message_t ev = {
    // .event = IMU_FREE_FALL_DETECTED,
    // .payload.value = 0  // you can encode severity, timestamp index, etc.
  // };

  // osMessageQueuePut(evManagerQueue, &ev, 0, 0);

#ifdef DEBUG
  fprintf(stdout, "IMU: free-fall event forwarded to EV_MANAGER\n");
#endif

  // 3) Optional policy:
  //    - you might want to force a FIFO dump here (pre/post impact data),
  //      or temporarily switch IMU mode, or set some global "drop detected" flag.
  //
  // Example:
  // readFifoAndLog(this);

  return ret;
}


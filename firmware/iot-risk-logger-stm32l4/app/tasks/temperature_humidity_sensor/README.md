# Temperature & Humidity Sensor Task

### Overview

### State Diagram

<details>
  <summary>Diagram as a code</summary>

```plantuml
@startuml
title Temperature & Humidity Sensor FSM
hide empty description

note "Publishes: \nGLOBAL_INITIALIZE_SUCCESS\nGLOBAL_ERROR" as N1

IDLE: Initialized\nready for commands, low power mode
CONTINUOUS_MEASURE: Device is measuring continuously\ngenerates interrupt on threshold exceed\n\nGLOBAL_WAKE_N_READ: Cron Read Measurements
ERROR: Error state\n\nGLOBAL_ERROR: Error message

[*] --> IDLE : GLOBAL_CMD_INITIALIZE

IDLE --> IDLE : SINGLE_SHOT_READ
IDLE --> IDLE : SET_LIMIT
IDLE --> CONTINUOUS_MEASURE : GLOBAL_CMD_START_CONTINUOUS_SENSING
IDLE --> ERROR : ERROR

CONTINUOUS_MEASURE --> CONTINUOUS_MEASURE : GLOBAL_WAKE_N_READ
CONTINUOUS_MEASURE --> CONTINUOUS_MEASURE : LIMIT_INT
CONTINUOUS_MEASURE --> ERROR : ERROR

ERROR --> IDLE : RECOVER (TODO)
@enduml
```
</details>

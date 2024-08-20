# Light Sensor Task

### Overview

### State Diagram 

<details>
  <summary>Diagram as a code</summary>
  
```plantuml
@startuml
title Light Sensor FSM
hide empty description

note "Publishes: \nGLOBAL_INITIALIZE_SUCCESS\nGLOBAL_ERROR" as N1

TURNED_OFF: Initialized, turned off\nready for commands, low power mode
CONTINUOUS_MEASURE: Device is measuring continuously\ngenerates interrupt on threshold exceed\n\nGLOBAL_WAKE_N_READ: Cron Read Measurements
OUT_OF_RANGE: Lux is out of range, limits are swapped\nreturn to measurements after lux returns in limits
ERROR: Error state\n\nGLOBAL_ERROR: Error message

[*] --> TURNED_OFF : GLOBAL_CMD_INITIALIZE

TURNED_OFF --> TURNED_OFF : SINGLE_SHOT_READ
TURNED_OFF --> TURNED_OFF : SET_LIMIT
TURNED_OFF --> CONTINUOUS_MEASURE : GLOBAL_CMD_START_CONTINUOUS_SENSING
TURNED_OFF --> ERROR : ERROR

CONTINUOUS_MEASURE --> CONTINUOUS_MEASURE : GLOBAL_WAKE_N_READ
CONTINUOUS_MEASURE --> OUT_OF_RANGE : LIMIT_INT
CONTINUOUS_MEASURE --> TURNED_OFF : TURN_OFF
CONTINUOUS_MEASURE --> ERROR : ERROR

OUT_OF_RANGE --> CONTINUOUS_MEASURE : LIMIT_INT
OUT_OF_RANGE --> OUT_OF_RANGE : CRON_READ
OUT_OF_RANGE --> TURNED_OFF : TURN_OFF
OUT_OF_RANGE --> ERROR : ERROR

ERROR --> TURNED_OFF : RECOVER (TODO)
@enduml
```
</details>

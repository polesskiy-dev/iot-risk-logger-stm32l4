# Light Sensor Task

### Overview

### State Diagram 

<details>
  <summary>Diagram as a code</summary>
  
```plantuml
@startuml{docs/assets/diagrams/light_sensor_fsm.png}
title Light Sensor FSM
hide empty description
TURNED_OFF: Initialized, turned off\nready for commands, low power mode
CONTINUOUS_MEASURE: Device is measuring continuously\ngenerates interrupt on threshold exceed
OUT_OF_RANGE: Lux is out of range, limits are swapped\nreturn to measurements after lux returns in limits
[*] --> TURNED_OFF : INITIALIZE
TURNED_OFF --> TURNED_OFF : SINGLE_SHOT_READ
TURNED_OFF --> TURNED_OFF : SET_LIMIT
TURNED_OFF --> CONTINUOUS_MEASURE : MEASURE_CONTINUOUSLY
CONTINUOUS_MEASURE --> CONTINUOUS_MEASURE : CRON_READ
CONTINUOUS_MEASURE --> OUT_OF_RANGE : LIMIT_INT
OUT_OF_RANGE --> CONTINUOUS_MEASURE : LIMIT_INT
OUT_OF_RANGE --> OUT_OF_RANGE : CRON_READ
OUT_OF_RANGE --> TURNED_OFF : TURN_OFF
CONTINUOUS_MEASURE --> TURNED_OFF : TURN_OFF
ERROR --> TURNED_OFF : RECOVER
TURNED_OFF --> ERROR : ERROR
CONTINUOUS_MEASURE --> ERROR : ERROR
OUT_OF_RANGE --> ERROR : ERROR
@enduml
```
</details>

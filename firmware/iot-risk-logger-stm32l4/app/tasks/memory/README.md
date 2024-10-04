# MEMORY Task

### Overview

### State Diagram

<details>
  <summary>Diagram as a code</summary>

```plantuml
@startuml
title MEMORY FSM
hide empty description

note "Publishes: \nGLOBAL_MEASUREMENTS_WRITE_SUCCESS\nGLOBAL_ERROR" as N1

SLEEP: Initialized\nready for commands, low power mode
WRITE: Writing measurements to memory\n\nGLOBAL_MEASUREMENTS_WRITE_SUCCESS: Data written
ERROR: Error state\n\nGLOBAL_ERROR: Error message

[*] --> SLEEP : GLOBAL_CMD_INITIALIZE
SLEEP --> SLEEP : GLOBAL_TEMPERATURE_HUMIDITY_MEASUREMENTS_READY
SLEEP --> SLEEP : GLOBAL_LIGHT_MEASUREMENTS_READY
SLEEP --> WRITE : MEASUREMENTS_WRITE

WRITE --> SLEEP : GLOBAL_MEASUREMENTS_WRITE_SUCCESS

SLEEP --> ERROR : ERROR
WRITE --> ERROR : ERROR
@enduml
```
</details>
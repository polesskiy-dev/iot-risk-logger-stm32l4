# MEMORY Task

### Overview

### State Diagram

<details>
  <summary>Diagram as a code</summary>

```plantuml
@startuml
title MEMORY FSM
hide empty description

note "Publishes: GLOBAL_ERROR" as N1

SLEEP: Initialized\nready for commands, low power mode
WRITE: Writing measurements to memory\n\nGLOBAL_MEASUREMENTS_WRITE_SUCCESS: Data written
ERROR: Error state\n\nGLOBAL_ERROR: Error message

[*] --> SLEEP : GLOBAL_CMD_INITIALIZE
SLEEP --> SLEEP : GLOBAL_TEMPERATURE_HUMIDITY_MEASUREMENTS_READY

SLEEP --> SLEEP : GLOBAL_LIGHT_MEASUREMENTS_READY
SLEEP --> SLEEP : GLOBAL_CMD_READ_LOG_CHUNK
note on link
    publishes GLOBAL_LOG_CHUNK_READ_SUCCESS
end note
SLEEP --> SLEEP : GLOBAL_CMD_READ_SETTINGS
note on link
    publishes GLOBAL_SETTINGS_READ_SUCCESS
end note

SLEEP --> WRITE : GLOBAL_CMD_WRITE_SETTINGS
note on link
    publishes GLOBAL_SETTINGS_WRITE_SUCCESS
end note
SLEEP --> WRITE : MEASUREMENTS_WRITE
note on link
    publishes GLOBAL_MEASUREMENTS_WRITE_SUCCESS
end note

WRITE --> SLEEP : GLOBAL_MEASUREMENTS_WRITE_SUCCESS
WRITE --> SLEEP : GLOBAL_SETTINGS_WRITE_SUCCESS

SLEEP --> ERROR : ERROR
WRITE --> ERROR : ERROR
@enduml
```
</details>
# NFC Task

### Overview

### State Diagram

<details>
  <summary>Diagram as a code</summary>

```plantuml
@startuml
title NFC FSM
hide empty description
STANDBY: RF free\nI2C free
[*] --> STANDBY : INITIALIZE
STANDBY --> ERROR : ERROR
@enduml
```
</details>

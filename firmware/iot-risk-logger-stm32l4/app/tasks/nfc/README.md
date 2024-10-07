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
MAILBOX_TRANSMISSION: Mailbox data exchange
ERROR: Error state\n\nGLOBAL_ERROR: Error message

[*] --> STANDBY : GLOBAL_CMD_INITIALIZE
STANDBY --> MAILBOX_TRANSMISSION : NFC_GPO_INTERRUPT
STANDBY --> ERROR : ERROR
@enduml
```
</details>

### Mailbox Data Exchange Protocol (ST25FTM) Packet Format

| **Field**   | **Description**                                    |
|-------------|----------------------------------------------------|
| **Command** | 1 byte (e.g., `WRITE_PART`, `WRITE_FINAL`, `READ`) |
| **Length**  | 1 byte size of the payload in bytes                |
| **Payload** | up to 252 bytes                                    |
| **Checksum**| 2 bytes (standard CRC16 for error detection)       |  

### Mailbox Data Exchange Protocol (ST25FTM) 

<details>
  <summary>Diagram as a code</summary>
  
```plantuml
@startuml
title ST25FTM Protocol Data Exchange (Abstract)

actor "Mobile phone" as reader
participant "ST25DV Tag" as tag
participant "STM32L4 MCU" as mcu

== Initial Setup ==
reader -> tag : Initiate Communication
tag -> mcu : Signal via Mailbox (Data Request)
mcu -> mcu : Prepare Data for Transmission

== Data Transmission (Chunk 1) ==
mcu -> tag : Write Data Chunk 1 to Mailbox (T2H)
tag -> reader : Notify Data Ready (Chunk 1)
reader -> tag : Read Data Chunk 1 from Mailbox
reader -> reader : Verify Data Integrity (Chunk 1)
reader -> tag : Acknowledge Chunk 1 (ACK)

== Data Transmission (Chunk 2) ==
mcu -> mcu : Prepare Next Data Chunk (Chunk 2)
mcu -> tag : Write Data Chunk 2 to Mailbox (T2H)
tag -> reader : Notify Data Ready (Chunk 2)
reader -> tag : Read Data Chunk 2 from Mailbox
reader -> reader : Verify Data Integrity (Chunk 2)
reader -> tag : Acknowledge Chunk 2 (ACK)

== Finalization ==
reader -> reader : Combine and Process Full Data
reader -> tag : End Communication

@enduml
```
</details>

### Mailbox Data Exchange Protocol (ST25FTM) for NOR Flash Data transfer

<details>
  <summary>Diagram as a code</summary>
  
```plantuml
@startuml
actor "Mobile phone" as reader
participant "ST25DV Tag" as tag
participant "STM32L4 MCU" as mcu

== Data Request ==

reader -> tag : Request Data (Read NOR Flash Page)
tag -> mcu : Signal Data Request (Mailbox Interrupt)

== Data Chunk 1 (128 bytes) ==

mcu -> mcu : Read NOR Flash (First 128 bytes)
mcu -> tag : Write Chunk 1 to T2H Mailbox
tag -> reader : Notify Data Available (Chunk 1)
reader -> tag : Read T2H Mailbox (Chunk 1)

reader -> reader : Process Data (Chunk 1)
reader -> tag : ACK for Chunk 1

== Data Chunk 2 (128 bytes) ==

mcu -> mcu : Read NOR Flash (Next 128 bytes)
mcu -> tag : Write Chunk 2 to T2H Mailbox
tag -> reader : Notify Data Available (Chunk 2)
reader -> tag : Read T2H Mailbox (Chunk 2)

reader -> reader : Process Data (Chunk 2)
reader -> tag : ACK for Chunk 2

== Data Exchange Complete ==
reader -> reader : Assemble Complete 256 bytes

@enduml
```
</details>

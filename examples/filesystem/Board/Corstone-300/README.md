# Board: Corstone-300 (AVH-SSE-300)

## Corstone-300 Board Layer

Device: **SSE-300-MPS3**

System Core Clock: **32 MHz**

The Arm Virtual Hardware is configured with `fvp_config.txt` file.

### System Configuration

| System resource       | Setting
|:----------------------|:---------------------------------------------
| Heap                  | 768 kB (configured in the `mps3-sse-300.sct`)
| Stack (MSP)           |  32 kB (configured in the `mps3-sse-300.sct`)

### STDIO mapping

**STDIO** is routed to terminal/console of Host machine running the simulation.

### CMSIS-Driver mapping

| CMSIS-Driver          | Peripheral            | Connection
|:----------------------|:----------------------|:------------------------------
| CMSIS-Driver VIO      | RAM Memory            | CMSIS_VIO

### Other Driver mapping

| Driver                | VSI Peripheral        | Source/Destination
|:----------------------|:----------------------|:------------------------------
| Audio In              | 0                     | Audio WAVE file
| Audio Out             | 1                     | Audio WAVE file
| Video In              | 4, 5                  | Camera, video WMV, AVI or MP4 file
| Video Out             | 6, 7                  | Display, video WMV, AVI or MP4 file

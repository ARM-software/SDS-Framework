# SDSIO Interface

The SDSIO components offer flexible SDS communication interfaces. You may choose between these interface components that are stored in the folders: `./sds/sdsio/client`, `./sds/sdsio/fs` or `./sds/sdsio/vsi`. These interfaces can be accessed as CMSIS software components for integration into the target system:

```yml
  - component: SDS:IO:Socket                     # IoT Socket Interface (Ethernet or WiFi)
  - component: SDS:IO:USB&MDK USB                # USB Interface
  - component: SDS:IO:RTT                        # RTT Interface
  - component: SDS:IO:Serial&CMSIS USART         # USART Interface
  - component: SDS:IO:File System&MDK FS         # Memory card
  - component: SDS:IO:File System&Semihosting    # Simulation or Debugger via Semihosting interface
  - component: SDS:IO:VSI                        # VSI Simulation interface of an AVH FVP
  - component: SDS:IO:Custom                     # Source code template for custom implementation
```

To simplify usage further, the following pre-configured SDS interface layers in *csolution project format* are available. These connect via various interfaces to the SDSIO-Server, which provides read/write access to SDS data files.

- [Ethernet Interface](#layer-sdsio_network) using the MDK-Middleware Network component.
- [USB Bulk Interface](#layer-sdsio_usb) using the MDK-Middleware USB component.
- [RTT Interface](#layer-sdsio_rtt) using the SEGGER RTT component.
- [Memory Card Interface](#layer-sdsio_fs) using the MDK-Middleware File System component.
- [VSI Interface](#layer-sdsio_fvp) using the ARM FVP simulation VSI interface.

## Layer: sdsio_network

The [`layer/sdsio/network/sdsio_network.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/network) is configured for recording and playback via the Ethernet interface. It uses the  [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil) Network component.

To access [SDS data files](theory.md#sds-data-files), start the [SDSIO-Server](utilities.md#sdsio-server) on the host computer with:

```bash
>python sdsio-server.py socket
Press 'Ctrl+C' or 'X' to exit.
Socket server listening on 172.20.10.2:5050
```

The SDSIO Server displays the IP address on which it is listening.
This is the IP address that the target hardware must connect to. Configure this address in `./layer/sdsio/network/RTE/SDS/sdsio_client_socket_config.h` file
by editing the `SDSIO_SOCKET_SERVER_IP` macro.

## Layer: sdsio_usb

The [`layer/sdsio/usb/sdsio_usb.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/usb) is configured for recording and playback via the USB interface. It uses the [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil) USB Device component and connects to a host computer through a USB interface.

To access [SDS data files](theory.md#sds-data-files), start the [SDSIO-Server](utilities.md#sdsio-server) on the host computer with:

```bash
>sdsio-server.py usb
Press 'Ctrl+C' or 'X' to exit.
Starting USB Server...
Waiting for SDSIO Client USB device...
```

## Layer: sdsio_rtt

The [`layer/sdsio/rtt/sdsio_rtt.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/rtt) is configured for recording and playback using the SEGGER RTT component for I/O via a debug adapter.

To access [SDS data files](theory.md#sds-data-files) using RTT with J-Link, start the [SDSIO-Server](utilities.md#sdsio-server) on the host computer with:

```bash
>sdsio-server.py socket --ipaddr 127.0.0.1 --port 19021 --connect-mode --connect-message "$$SEGGER_TELNET_ConfigStr=RTTCh;1$$"
Press 'Ctrl+C' or 'X' to exit.
Starting USB Server...
Waiting for SDSIO Client USB device...
```

## Layer: sdsio_fs

The [`layer/sdsio/filesystem/sdsio_fs.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/filesystem) is configured for recording to a Memory Card. It uses the MDK-Middleware File System component.

## Layer: sdsio_fvp

The [`template/sdsio/fvp/sdsio_fvp.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/template/sdsio/fvp) targets AVH FVP simulation and is configured for playback from the host computer. It uses the [SDSIO VSI interface](https://arm-software.github.io/AVH/main/simulation/html/group__arm__vsi.html) implemented by the file `vsi/python/arm_vsi3.py`, which is loaded by the FVP simulation model. Since the SDSIO-Server functionality is implemented in `arm_vsi3.py`, no separate SDSIO-Server is required.

### Configuration File: sdsio.yml

The SDSIO VSI interface can be configured using a [`*.sdsio.yml` control file](utilities.md#sdsio-control-file-sdsioyml).

The VSI3 Python script (`arm_vsi3.py`) locates the control file as follows:

- If the environment variable `SDSIO_FVP` is set, it must be an **absolute path** to either a config file or a directory:
  - **File** — used directly as the config file.
  - **Directory** — searched for a config file in this order: `sdsio.yml`, `sdsio.yaml`, first `*.sdsio.yml` (alphabetical), first `*.sdsio.yaml` (alphabetical).
  - If the path does not exist or is not absolute, the search falls back to the working directory.
- If `SDSIO_FVP` is not set (or falls back), the **simulator working directory** is searched using the same file order: `sdsio.yml`, `sdsio.yaml`, first `*.sdsio.yml` (alphabetical), first `*.sdsio.yaml` (alphabetical).

If a control file is found, the script reads the `sdsio:` root node. If `workdir:` is a relative path, it is interpreted relative to the simulator working directory.

### Log File: sdsio.log

During FVP simulation, an `sdsio.log` file is generated that records all [SDS data file](theory.md#sds-data-files) access operations.

**Example:**

```txt
Created by ...\Board\Corstone-300\vsi\python\arm_vsi3.py

SDSIO VSI version 3.0.0
SDSIO configuration YAML: ...\SDS.sdsio.yml
sdsFlags = 0xB0000000
Playback: ML_In (...\SDS Recordings\ML_In.0.sds)
Record:   ML_Out (...\SDS Recordings\ML_Out.0.p.sds)
...
Closed:   ML_In (...\SDS Recordings\ML_In.0.sds)
Closed:   ML_Out (...\SDS Recordings\ML_Out.0.p.sds)

```

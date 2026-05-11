# SDSIO Interface

The SDSIO components offer flexible SDS communication interfaces. You may choose between these interface components that are stored in the folders: `./sds/sdsio/client`, `./sds/sdsio/fs` or `./sds/sdsio/vsi`. These interfaces can be accessed as CMSIS software components for integration into the target system:

```yml
  - component: SDS:IO:Socket                     # IoT Socket Interface (Ethernet or WiFi)
  - component: SDS:IO:USB&MDK USB                # USB Interface
  - component: SDS:IO:Serial&CMSIS USART         # USART Interface
  - component: SDS:IO:File System&MDK FS         # Memory card
  - component: SDS:IO:File System&Semihosting    # Simulation or Debugger via Semihosting interface
  - component: SDS:IO:VSI                        # VSI Simulation interface of an AVH FVP
  - component: SDS:IO:Custom                     # Source code template for custom implementation
```

To simplify usage further, the following pre-configured SDS interface layers in *csolution project format* are available. These connect via various interfaces to the SDSIO Server, which provides read/write access to SDS data files.

- [Ethernet Interface](#layer-sdsio_network) using the MDK-Middleware Network component.
- [USB Bulk Interface](#layer-sdsio_usb) using the MDK-Middleware USB component.
- [Memory Card Interface](#layer-sdsio_fs) using the MDK-Middleware File System component.

## Layer: sdsio_network

The [`layer/sdsio/network/sdsio_network.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/network) is configured for recording and playback via the Ethernet interface. It uses the  [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil) Network component.

For [SDS data file](theory.md#sds-data-files) access the [SDSIO-Server](utilities.md#sdsio-server) is started on the Host computer as shown below.  This output an IP address as shown below:

```bash
>python sdsio-server.py socket
Press Ctrl+C to exit.
Socket server listening on 172.20.10.2:5050
```

This is the IP address that the target hardware needs to connect to and it is required to configure this IP address in the file `./layer/sdsio/network/RTE/SDS/sdsio_client_socket_config.h` with  the define `SDSIO_SOCKET_SERVER_IP`.

!!! Note
    - The firewall of the Host computer must be configured so that it can be reached from the target device under its IP address.

### Usage on Windows

The easier way to connect the Host computer to the target board via Ethernet is to use a local network switch. With a standard network installation, the target hardware and the Host computer should be connected to the same LAN, and the DHCP server should assign a dynamic IP address automatically.

A firewall may restrict Ethernet access. The instructions below are for the Windows Defender Firewall.

- Open **Windows Security - Firewall & network protection - Allow an app through firewall**.
- Click **Change settings**, then allow your Python runtime (`python.exe`) on the network profile you use (usually Private).

If this is a managed corporate PC, Group Policy or endpoint security may still block it, so your IT may need to whitelist the IP address and port.

## Layer: sdsio_usb

The [`layer/sdsio/usb/sdsio_usb.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/usb) is configured for recording and playback via the USB Device interface. It uses the [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil) USB Device component and connects via a USB interface to a Host computer.

For [SDS data file](theory.md#sds-data-files) access start the [SDSIO-Server](utilities.md#sdsio-server) on the Host computer with:

```bash
>sdsio-server.py usb
Press Ctrl+C to exit.
Starting USB Server...
Waiting for SDSIO Client USB device...
```

Once the SDSIO-Server is running start the application. The SDSIO-Server outputs:

```bash
SDSIO Client USB device connected.
```

For recording, the SDSIO-Server outputs:

```bash
Record:   ML_In (.\ML_In.0.sds).
Record:   ML_Out (.\ML_Out.0.sds).
..
Closed:   ML_In (.\ML_In.0.sds).
Closed:   ML_Out (.\ML_Out.0.sds).
```

For playback, the SDSIO-Server outputs:

```bash
Playback: ML_In (.\ML_In.0.sds).
Record:   ML_Out (.\ML_Out.0.p.sds).
Closed:   ML_In (.\ML_In.0.sds).
Closed:   ML_Out (.\ML_Out.0.p.sds).
```

## Layer: sdsio_rtt

The [`layer/sdsio/rtt/sdsio_rtt.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/rtt) is configured for recording and playback using the SEGGER RTT component for I/O via a debug adapter.

## Layer: sdsio_fs

The [`layer/sdsio/filesystem/sdsio_fs.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/filesystem) is configured for recording to a Memory Card. It uses the MDK-Middleware File System component.

## Layer: sdsio_fvp

The [`template/sdsio/fvp/sdsio_fvp.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/template/sdsio/fvp) targets AVH FVP simulation and is configured for recording and playback to/from the Host computer. It uses the [SDSIO VSI interface](https://arm-software.github.io/AVH/main/simulation/html/group__arm__vsi.html) implemented by the file `vsi/python/arm_vsi3.py`, which is loaded by the FVP simulation model. As the SDSIO server is implemented in `arm_vsi3.py` there is no separate SDSIO server needed.

### Configuration File: sdsio.yml

The SDSIO VSI interface can be configured using the `sdsio.yml` file in the current working folder that is used to start the FVP simulator. This configuration settings define the [SDS data file](theory.md#sds-data-files) access during FVP simulation.

**Description:**

```yml
```

**Example:**

```yml
```

### Log File: sdsio.log

During the FVP simulation a log file is generated that lists the [SDS data file](theory.md#sds-data-files) access.

**Example:**

```txt
Created by ...\Board\Corstone-300\vsi\python\arm_vsi3.py

Playback:  ML_In (.\algorithm\SDS Recordings\ML_In.0.sds).
Record:    ML_Out (.\algorithm\SDS Recordings\ML_Out.0.p.sds).
Closed:    ML_In.
Closed:    ML_Out.
Playback:  ML_In (.\algorithm\SDS Recordings\ML_In.1.sds).
Record:    ML_Out (.\algorithm\SDS Recordings\ML_Out.1.p.sds).
Closed:    ML_In.
Closed:    ML_Out.
   :
   :
Playback:  ML_In (.\algorithm\SDS Recordings\ML_In.11.sds).
Record:    ML_Out (.\algorithm\SDS Recordings\ML_Out.11.p.sds).
Closed:    ML_In.
Closed:    ML_Out.
```

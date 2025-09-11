# SDSIO Interface

The SDSIO components offer flexible recorder and playback interfaces. You may choose between these interface components that are stored in the folder `./sds/source/sdsio`. These interfaces can be accessed as CMSIS software components for integration into the target system:

```yml
  - component: SDS:IO:Socket                     # Socket Interface (Ethernet or WiFi)
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

The [`layer/network/sdsio_network.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/network) is configured for recording and playback via Ethernet interface. It uses the  [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil) Network component.

For [SDS data file](theory.md#sds-data-files) access the [SDSIO-Server](utilities.md#sdsio-server) is started on the Host computer as shown below.  This output an IP address as shown below:

```bash
>Press Ctrl+C to exit.
Socket server listening on 172.20.10.2:5050
```

This is the IP address that the target hardware needs to connect and it is require to configure this IP address in the file `./layer/network/RTE/SDS/sdsio_config_socket.h` with  the define `SDSIO_SOCKET_SERVER_IP`.

!!! Note
    - It is important that a firewall of the Host computer is configured so that it can be reached from the target device undre its IP address. 

## Layer: sdsio_usb

The [`layer/usb/sdsio_usb.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/usb) is configured for recording and playback via USB Device interface. It uses the [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil) USB Device component and connects via a USB interface to a Host computer.

For [SDS data file](theory.md#sds-data-files) access the [SDSIO-Server](utilities.md#sdsio-server) is started on the Host computer with:

```bash
>sdsio-server.py usb
```

## Layer: sdsio_fs

The [`layer/filesystem/sdsio_fs.clayer`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/filesystem) is configured for recording and playback to/from the Memory Card. It uses the MDK-Middleware File System component.

## Layer: sdsio_fvp

The [`template/sdsio/fvp/sdsio_fvp.clayer`](https://github.com/ARM-software/SDS-Framework/tree/main/template/sdsio/fvp) targets AVH FVP simulation and is configured for recording and playback to/from the Host computer. It uses the [SDSIO VSI interface](https://arm-software.github.io/AVH/main/simulation/html/group__arm__vsi.html) implemented by the file `vsi/python/arm_vsi3.py`, which is loaded by the FVP simulation model. As the SDSIO server is implemented in `arm_vsi3.py` there is no separate SDSIO server needed.

### Configuration File: sdsio.yml

The SDSIO VSI interface can be configured using the `sdsio.yml` file in the current working folder that is used to start the FVP simulator. This configuration settings define the [SDS data file](theory.md#sds-data-files) access during FVP simulation.

Node                 | Description
:--------------------|:------------------------------------
`dir:`               | Directory that contains the [SDS data files](theory.md#sds-data-files).
`idx-start:`         | First index of the [SDS data file](theory.md#sds-data-files) that are accessed; default: 0.
`idx-end:`           | Last index of the [SDS data file](theory.md#sds-data-files) that are accessed; default no upper limit.
`idx-list:`          | Index lists of [SDS data files](theory.md#sds-data-files) that are accessed; if given `index-start:` and `index-end:` are not used.

**Example:**

```yml
dir: ./algorithm/SDS Recordings
idx-start: 0
idx-end: 11
#idx-list: [1, 3]             # list of index to check (when applied, idx-start/end is ignored)
```

### Protocol File: sdsio.log

During the FVP simulation a log file is generated that lists the [SDS data file](theory.md#sds-data-files accesses.

**Example:**

```txt
Created by ...\Board\Corstone-300\vsi\python\arm_vsi3.py

Playback:  DataInput (.\algorithm\SDS Recordings\DataInput.0.sds).
Record:    DataOutput (.\algorithm\SDS Recordings\DataOutput.0.sds).
Closed:    DataInput.
Closed:    DataOutput.
Playback:  DataInput (.\algorithm\SDS Recordings\DataInput.1.sds).
Record:    DataOutput (.\algorithm\SDS Recordings\DataOutput.1.sds).
Closed:    DataInput.
Closed:    DataOutput.
   :
   :
Playback:  DataInput (.\algorithm\SDS Recordings\DataInput.11.sds).
Record:    DataOutput (.\algorithm\SDS Recordings\DataOutput.11.sds).
Closed:    DataInput.
Closed:    DataOutput.
Playback:  DataInput - ACCESS REJECTED. Index excluded in sdsio.yml
```


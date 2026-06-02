# SDS with SDSIO via RTT

This layer provides SDS with SDSIO using the RTT (Real Time Transfer) communication.
It is implemented with the SEGGER RTT component.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Stream__Interface.html) data streaming,
- [SDSIO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDSIO__Interface.html) SDSIO interface,
- [RTT](https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/) communication channel.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Stream&CMSIS-RTOS2
  - component: SDS:IO:RTT
```

## RTT Configuration

The following SEGGER RTT software components are required:

```yml
  - component: SEGGER:RTT
```

The RTT channel number used for SDSIO is configured in
`./RTE/SDS/sdsio_client_rtt_config.h` with the `SDSIO_RTT_CHANNEL` define (default: `1`).

## Starting SDSIO-Server

The SDSIO layer communicates with the host PC via an RTT socket exposed by the
debug probe software. The [SDSIO-Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server)
runs on the host and connects to the debug probe socket using
[socket connect mode](https://arm-software.github.io/SDS-Framework/main/utilities.html#socket-mode).

Any debug probe software that exposes RTT data over a TCP socket can be used.
Examples are provided below for **SEGGER J-Link** and **pyOCD**.

---

### SEGGER J-Link

J-Link exposes RTT data via a TELNET-like TCP socket on `localhost`, port **19021**
(default), while a J-Link connection is active (e.g., during a debug session).

After connecting, the client has **100 ms** to send a
[SEGGER TELNET Config String](https://kb.segger.com/J-Link_RTT_TELNET_Channel) that selects
the RTT channel. The SDSIO-Server should sends this string automatically via `--connect`,
and discards any initial response from J-Link during the `--connect-time` window.

#### Start SDSIO-Server for J-Link

Replace `<channel>` with the value of `SDSIO_RTT_CHANNEL` (default: `1`):

```bash
python sdsio-server.py socket --ipaddr 127.0.0.1 --port 19021 \
    --connect "$$SEGGER_TELNET_ConfigStr=RTTCh;<channel>$$"
```

Example for the default channel 1:

```bash
python sdsio-server.py socket --ipaddr 127.0.0.1 --port 19021 \
    --connect "$$SEGGER_TELNET_ConfigStr=RTTCh;1$$"
```

#### SDSIO control file for J-Link

Alternatively, configure via an `sdsio.yml` control file (see
[SDSIO Control File](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-control-file-sdsioyml)):

```yml
sdsio:
  interface:
    socket:
      ipaddr: 127.0.0.1
      port: 19021
      connect: "$$SEGGER_TELNET_ConfigStr=RTTCh;1$$"
      connect-time: 100

  workdir: ./SDS Recordings
```

Then start the server with:

```bash
python sdsio-server.py -c sdsio.yml
```

---

### pyOCD

pyOCD can expose each RTT channel as a TCP socket server. No connect message is required.
The RTT channel can be configured via a `*.cbuild-run.yml` file, CLI options, or a `pyocd.yml`
session options file — refer to the
[pyOCD RTT documentation](https://open-cmsis-pack.github.io/cmsis-toolbox/pyOCD-Debugger/#rtt)
for details. The `channel:` list must include an entry for the SDSIO channel
(`SDSIO_RTT_CHANNEL`, default: `1`) with `mode: server` and a free TCP port:

```yml
rtt:
  - control-block:
      auto-detect: true
    channel:
      - number: 1     # must match SDSIO_RTT_CHANNEL in sdsio_client_rtt_config.h
        mode: server
        port: 5100    # any free TCP port on localhost
```

Run the target with pyOCD (RTT is active during `pyocd run`):

```bash
pyocd run --cbuild-run out/<name>+<target-type>.cbuild-run.yml --eot
```

#### Start SDSIO-Server for pyOCD

```bash
python sdsio-server.py socket --ipaddr 127.0.0.1 --port 5100 --connect
```

#### SDSIO control file for pyOCD

```yml
sdsio:
  interface:
    socket:
      ipaddr: 127.0.0.1
      port: 5100       # must match the port configured in *.csolution.yml
      connect:

  workdir: ./SDS Recordings
```

Then start the server with:

```bash
python sdsio-server.py -c sdsio.yml
```

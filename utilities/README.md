# Utilities

This directory contains several Python scripts that process [SDS data files](https://arm-software.github.io/SDS-Framework/main/theory.html#sds-data-files).

Utility                                                                                       | Description
:---------------------------------------------------------------------------------------------|:-------------------------------
[SDSIO-Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server) | Tool for recording and playback of SDS data files.
[SDS-Check](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-check)       | Confidence checker for SDS data files.
[SDS-Convert](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-convert)   | Converter of SDS data recording into various formats.
[SDS-View](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-view)         | Graphical data viewer for SDS data files.

## Windows on ARM64

Use the native ARM64 standalone SDSIO-Server build. It includes Python, the server's
Python dependencies, and the ARM64 libusb DLL. No Python installation, `pip install`,
OpenCV installation, manual DLL copying, or system `PATH` changes are required.

1. Download the `sdsio-server-windows-arm64-<version>` artifact from a successful
   [Build sdsio-server workflow run](https://github.com/ARM-software/SDS-Framework/actions/workflows/build_sdsio-server.yml).
   Extract the artifact into a local folder. Keep the included `sdsio-server-libusb`
   directory with the executable; it contains third-party licenses, corresponding
   source archives, and rebuilding instructions, not additional runtime setup.

2. For USB transport, connect the board running the SDSIO-Client firmware. Its SDSIO
   interface must use the Windows **WinUSB** driver. If the board already uses WinUSB,
   no driver change is needed. Otherwise, follow the board vendor's ARM64-compatible
   driver instructions or use [Zadig](https://zadig.akeo.ie/) with administrator
   privileges: select **Options > List All Devices**, identify the SDSIO interface by
   its USB IDs, select **WinUSB**, and install or replace its driver.

   **Only change the SDSIO interface's driver**, not the debug probe, serial interface,
   composite parent device, or unrelated USB devices. The executable bundles the
   user-space USB library, not a device driver. Windows ARM64 requires ARM64-compatible
   device drivers, including when running an x64 application under emulation.

3. From the extracted folder, start the server using PowerShell:

   ```powershell
   .\sdsio-server.exe usb --workdir .\data
   ```

   The firmware must expose the USB product string `SDSIO-Client` or `SDSIO Client`,
   which the server uses to discover the device. If the server keeps waiting, check
   the firmware, USB connection, and WinUSB driver binding. Use an elevated shell
   if enabling `--high-priority`.

For TCP/IP transport, no USB driver setup is needed. Serial transport requires an
ARM64-compatible driver for the serial adapter. See the
[SDSIO-Server usage documentation](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server)
for the corresponding commands, using `.\sdsio-server.exe` instead of
`python sdsio-server.py`.

The Windows x64 artifact (`sdsio-server-windows-<version>`) similarly includes its
matching x64 DLL. Use the ARM64 artifact for native execution on Windows ARM;
there is no need to select or install a DLL yourself.

CI checks USB-library initialization using a separate temporary executable for each
Windows architecture. Recording/playback and the board's driver binding still require
testing with target hardware.

To build the executable yourself, see [DEVELOPMENT.md](../DEVELOPMENT.md).

# Utilities

This directory contains several Python scripts that process [SDS data files](https://arm-software.github.io/SDS-Framework/main/theory.html#sds-data-files).

Utility                                                                                       | Description
:---------------------------------------------------------------------------------------------|:-------------------------------
[SDSIO-Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server) | Tool for recording and playback of SDS data files.
[SDS-Check](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-check)       | Confidence checker for SDS data files.
[SDS-Convert](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-convert)   | Converter of SDS data recording into various formats.
[SDS-View](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-view)         | Graphical data viewer for SDS data files.

## Windows on ARM64

SDSIO-Server can use native ARM64 Python on Windows. It does not use OpenCV, so
installing `opencv-python` or the complete `requirements.txt` is not necessary
when using only the server. The following steps describe the native ARM64 setup;
they have not been validated end-to-end on Windows ARM64 hardware.

Run these PowerShell commands from this directory.

1. Install an ARM64 build of [CPython](https://www.python.org/downloads/windows/)
   (for example, Python 3.13). Ensure `python` selects that installation, then
   create an isolated environment and install the server dependencies:

   ```powershell
   python -c "import platform, struct; print(platform.machine(), struct.calcsize('P') * 8)"
   python -m venv .venv
   .\.venv\Scripts\python.exe -m pip install ifaddr pyserial libusb1 pyyaml
   ```

   The architecture check should report `ARM64 64`. The explicit virtual-environment
   executable avoids requiring PowerShell script activation.

2. For USB transport, download the Windows binary archive (`libusb-<version>.7z`)
   from the [official libusb releases](https://github.com/libusb/libusb/releases).
   Extract it and locate `libusb-1.0.dll` in an `ARM64\dll` directory under the
   Visual Studio build directory. Archive layouts vary by release. Use the DLL,
   not the `.lib` file, and do not use an x86 or x64 DLL with ARM64 Python.

   Copy that DLL into the environment's `usb1` package directory, which the Python
   wrapper searches explicitly. Replace the source path below with the extracted
   DLL's actual location:

   ```powershell
   Copy-Item 'C:\path\to\extracted\ARM64\dll\libusb-1.0.dll' '.\.venv\Lib\site-packages\usb1\libusb-1.0.dll'
   ```

   Installing the Python package `libusb1` alone does not guarantee that a matching
   native DLL is installed. Keep the DLL local to this environment; do not copy it
   into Windows system directories.

3. Verify that the wrapper can load and initialize the native USB library:

   ```powershell
   .\.venv\Scripts\python.exe -c "import usb1; ctx = usb1.USBContext(); ctx.open(); print('libusb initialized'); ctx.close()"
   ```

   A `cannot find a suitable libusb-1.0` error can indicate a missing DLL, an
   architecture mismatch, or a missing DLL dependency. This check does not verify
   the target device's driver or a complete recording/playback session.

4. For USB transport, connect the board running the SDSIO-Client firmware and
   ensure its SDSIO USB interface uses the Windows **WinUSB** driver. Prefer the
   board vendor's ARM64-compatible driver installation instructions. If necessary,
   use [Zadig](https://zadig.akeo.ie/) with administrator privileges: select
   **Options > List All Devices**, identify the SDSIO interface by its USB IDs,
   select **WinUSB**, and install or replace its driver.

   **Only change the SDSIO interface's driver.** Do not replace drivers for the
   board's debug probe, serial interface, or unrelated USB devices. On a composite
   device, select the SDSIO interface rather than the parent device. Windows ARM64
   requires an ARM64-compatible driver even when running x64 Python under emulation;
   installing the libusb DLL does not install a device driver.

5. Start the server in USB mode:

   ```powershell
   .\.venv\Scripts\python.exe .\sdsio-server.py usb --workdir .\data
   ```

   The firmware must expose the USB product string `SDSIO-Client` or `SDSIO Client`,
   which the server uses to discover the device. If it keeps waiting for a device,
   check the firmware, USB connection, and WinUSB driver binding. Use an elevated
   shell if enabling `--high-priority`.

For TCP/IP or serial transport, install the same Python dependencies but skip the
USB DLL and WinUSB setup steps. Serial transport still requires an ARM64-compatible
driver for the serial adapter. See the
[SDSIO-Server usage documentation](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server)
for the corresponding commands.

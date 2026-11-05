# SDSIO-Server Binary Build

This repository builds the SDSIO-Server Python utility as a standalone executable
with [PyInstaller](https://pyinstaller.org/).

## Local Build

Use Python 3.11 or a compatible Python 3 installation. From the repository root, install the build
and SDSIO-Server dependencies:

```bash
python -m pip install pyinstaller
python -m pip install -r .github/requirements-sdsio-server.txt
```

Build the one-file executable:

```bash
pyinstaller --clean --noconfirm --onefile --name sdsio-server utilities/sdsio-server.py
```

The executable is written to `dist/`. On Windows, the output file is `dist/sdsio-server.exe`.

### Windows x64 and ARM64 builds

Build each executable with matching Python and libusb architectures. Download the
Windows binary archive from the [libusb 1.0.30 release](https://github.com/libusb/libusb/releases/tag/v1.0.30)
and extract the appropriate DLL:

| Python / executable | DLL in the archive |
| --- | --- |
| Windows x64 | `VS2025\MS64\dll\libusb-1.0.dll` |
| Windows ARM64 | `VS2025\ARM64\dll\libusb-1.0.dll` |

Add the selected DLL to the existing build command (substitute its absolute path):

```powershell
python -m PyInstaller --clean --noconfirm --onefile --name sdsio-server --add-binary "C:\path\to\libusb-1.0.dll:usb1" utilities\sdsio-server.py
```

The DLL is extracted automatically and dynamically loaded by the Python wrapper.
There is no runtime architecture selection: each executable contains its own matching
Python runtime and DLL. Native ARM64 output requires native ARM64 Python; x64 Python
under Windows ARM emulation produces an x64 executable.

CI performs the download and selection automatically, verifies pinned archive SHA256
hashes, and includes `sdsio-server-libusb/` with licenses, corresponding libusb and
libusb1 sources, application source, and rebuilding instructions. Distribute this
folder with the executable. For local redistribution, include the same material;
see [the notice template](.github/libusb-notices.txt). When updating libusb, update the
version, archive hashes, archive paths in the workflow, and these documents together.

End users need neither Python nor manual DLL setup; USB transport still requires the
target's WinUSB driver binding. See the [Windows ARM64 usage instructions](utilities/README.md#windows-on-arm64).

## Continuous Integration

The workflow [`.github/workflows/build_sdsio-server.yml`](.github/workflows/build_sdsio-server.yml)
builds SDSIO-Server on these GitHub-hosted runners:

| Runner | Artifact |
| --- | --- |
| `windows-2025` | `sdsio-server-windows` |
| `windows-11-arm` | `sdsio-server-windows-arm64` |
| `ubuntu-24.04` | `sdsio-server-linux` |
| `macos-15` | `sdsio-server-macos` |
| `ubuntu-24.04-arm` | `sdsio-server-linux-arm64` |

Each artifact name includes the version read from `SDSIO_SERVER_VERSION` in `utilities/sdsio-server.py`.

The workflow also runs the generated executable with `--version` and compares the reported version with
`SDSIO_SERVER_VERSION` before uploading the artifact. Both Windows jobs also build and
run a temporary one-file USB smoke-test executable with the same DLL inclusion option.
It initializes and closes libusb without a target device, and is not distributed.
This does not verify a board's driver binding or recording/playback; those require
hardware testing. The server's runtime code and CLI are unchanged.

## Dependencies

SDSIO-Server uses the minimal dependency set in [`.github/requirements-sdsio-server.txt`](.github/requirements-sdsio-server.txt):

- `ifaddr`
- `pyyaml`
- `pyserial`
- `libusb1`

The image and data-processing dependencies used by SDS-View and SDS-Convert are kept separate
from the SDSIO-Server build.

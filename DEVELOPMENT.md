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
`SDSIO_SERVER_VERSION` before uploading the artifact.

## Dependencies

SDSIO-Server uses the minimal dependency set in [`.github/requirements-sdsio-server.txt`](.github/requirements-sdsio-server.txt):

- `ifaddr`
- `pyyaml`
- `pyserial`
- `libusb1`

The image and data-processing dependencies used by SDS-View and SDS-Convert are kept separate
from the SDSIO-Server build.

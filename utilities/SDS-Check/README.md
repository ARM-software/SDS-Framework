# SDS-Check

Checks SDS data files written with the **DataTest** example for correctness. It can be used
to check the SDS records to see if the storage format is consistent and without errors.

### Size Consistency Check

This check processes the SDS data records and calculates the total size of the SDS data.
It is the sum of all data records (header + data). This data size should match the size of
the SDS file in question. If the sizes do not match, an error is printed, e.g:

```txt
Error: File size mismatch. Expected 360 bytes, but file contains 363 bytes.
```

### Timestamp Consistency Check

This check processes the SDS records and ensures that the timestamps recorded in the records
are arranged in ascending order. If the utility detects that the timestamp of the subsequent
data record is lower than the current one, an error is printed, e.g:

```txt
Error: Timestamp not in ascending order in record 23.
```

### Jitter Check

This check processes the SDS data records and searches for a maximum deviation of the recorded
timestamps from the expected ones. If the deviation is found, the number of the data record
is also saved. This maximum deviation is then evaluated as **jitter** and printed out in
the utility's summary report, e.g:

```txt
python sds-check.py -s Temperature.0.sds
File    : Temperature.0.sds
Size    : 360 bytes
Records : 30
Interval: 1024ms
Jitter  : 56ms, record 19
Validation passed
```

## Setup and requirements

### Requirements

- Python 3.9 or later

## Usage
Print help (*common*) with:
```
python sds-check.py --help
```

```txt
usage: sds-check.py [-h] -s [<sds_file>]

SDS data validation

options:
  -h, --help       show this help message and exit

required:
  -s [<sds_file>]  SDS data recording file
```

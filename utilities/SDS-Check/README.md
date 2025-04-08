# SDS-Check

Checks SDS data files written with the **DataTest** example for correctness. It can be used
to check the SDS data records to ensure that the storage format is consistent and error-free.
The following checks are performed:

- Size consistency check
- Timestamp consistency check
- Jitter check
- Delta time check
- Duplicate timestamp check

### Size consistency check

This check processes the SDS data records and calculates the total size of the SDS data.
It is the sum of all data records (header + data). This data size should match the size of
the SDS file in question. If the sizes do not match, an error is printed, e.g:

```txt
Error: File size mismatch. Expected 360 bytes, but file contains 363 bytes.
```

### Timestamp consistency check

This check processes the SDS records and ensures that the timestamps recorded in the records
are arranged in ascending order. If the utility detects that the timestamp of the subsequent
data record is lower than the current one, an error is printed, e.g:

```txt
Error: Timestamp not in ascending order in record 23.
```

### Jitter check

This check processes the SDS data records and searches for a maximum deviation of the recorded
timestamps from the expected ones. If the deviation is found, the number of the data record
is also saved. This maximum deviation is then evaluated as **jitter** and printed out in
the utility's summary report, e.g:

```txt
File     : Gyroscope.0.sds
Size     : 153,334 bytes
Records  : 284
Interval : 50ms
Jitter   : 0ms
Validation passed
```

### Delta time check

This check processes the SDS records and tries to find the largest difference in timestamps
between two neighboring records, called **DeltaTime**.

For normally recorded files, the delta time and the sampling interval are identical, so no information
about the delta time status is printed. If the delta time and the recording interval are not identical,
i.e. a difference is detected, the record number is also saved and the **DeltaTime** is added in
the summary report, e.g:

```txt
File     : Temperature.1.sds
Size     : 36 bytes
Records  : 3
Interval : 1025ms
Jitter   : 25ms, record 1
DeltaTime: 1050ms, record 2
Validation passed
```

This is not an error, but a report of an anomaly. If the delta time is long compared to the sampling
interval, e.g. a few times longer, this could indicate that one or more data records are missing from
the recorded file.

### Duplicate timestamp check

This check processes the SDS records in search of reused timestamps, the so-called **Duplicate**
timestamps. This means that the same timestamp is used in several consecutive data records.

This may indicate that the recording loop in an embedded application is not set up correctly. It is also
possible that duplicate timestamps are caused by unexpected thread delays in the embedded application.

Duplicate timestamps are not found in normal recording files. If multiple timestamps with the same value
are found in the SDS file, **DupStamps** will be added in the summary report, e.g:

```txt
File     : DataInput.0.sds
Size     : 17,509,440 bytes
Records  : 47580
Interval : 5ms
Jitter   : 4ms, record 4
DupStamps: 4, record 1
Validation passed
```

This is not an error, but a report of an anomaly. The report contains the number of records with
the same timestamp and the position in the SDS file where the anomaly was detected (record number).
Please note that only the first occurrence of a duplicate timestamp is reported.

## Setup and requirements

### Requirements

- Python 3.9 or later

## Usage
Print help (*common*) with:

```txt
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

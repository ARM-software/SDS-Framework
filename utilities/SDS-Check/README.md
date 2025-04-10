# SDS-Check

Checks SDS data files written with the **DataTest** example for correctness. It can be used
to check the SDS data records to ensure that the storage format is consistent and error-free.
The following checks are performed:

- **Size consistency** check,
- **Timestamp consistency** check,
- **Jitter** check,
- **Delta time** check,
- **Duplicate timestamp** check.

After processing the SDS data file, the SDS-Check utility prints a summary with statistics:

- **DataSize**:  total size of the data in bytes,
- **Records**:   total number of records,
- **BlockSize**: average block size of a data record,
- **Largest**:   largest block size, if different from the average block size (optional),
- **Smallest**:  smallest block size, if different from the average block size (optional),
- **Interval**:  time interval of the recording in milliseconds,
- **DataRate**:  recorded data rate in bytes per second,
- **Jitter**:    deviation from the expected timestamps,
- **DeltaTime**: largest difference of the neighboring timestamps, if deviating from the recording interval (optional),
- **DupStamps**: number of reused timestamps, if found (optional).

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
DataSize : 153.334 bytes
Records  : 284
BlockSize: 532 bytes
Largest  : 606 bytes
Smallest : 444 bytes
Interval : 50 ms
DataRate : 10.640 byte/s
Jitter   : 0 ms
Validation passed
```

### Delta time check

This check processes the SDS records and tries to find the largest difference in timestamps
between two neighboring records, called **DeltaTime**.

For normally recorded files, the delta time and the recording interval are identical, so no information
about the delta time status is printed. If the delta time and the recording interval are not identical,
i.e. a difference is detected, the record number is also saved and the **DeltaTime** is added in
the summary report, e.g:

```txt
File     : Temperature.0.sds
DataSize : 360 bytes
Records  : 30
BlockSize: 4 bytes
Interval : 1.024 ms
DataRate : 4 byte/s
Jitter   : 59 ms, record 19
DeltaTime: 1.050 ms, record 2
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
DataSize : 17.509.440 bytes
Records  : 47.580
BlockSize: 360 bytes
Interval : 1 ms
DataRate : 360.000 byte/s
Jitter   : 4 ms, record 4
DeltaTime: 5 ms, record 5
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

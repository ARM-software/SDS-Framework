# SDS-Convert
Convert SDS data recordings to selected format, based on descriptions found in metadata (YAML) files.  
By default raw timestamps are used for the output file. User can override this behaviour using `--normalize` flag.
Output timestamps will then start with 0. User can also select start and stop tick for the data to be converter to the
specified format using `--start-tick <tick>` and `--stop-tick <tick>` respectively. Both parameters are based on 
timestamp format in the output file.

## Supported formats
- **Simple CSV**  
  Format: `simple_csv`

  Convert .sds to simple CSV format. It takes one sensor and converts data for each record. In case of sensor with
  multiple channels, each channel will be presented in its own column.

  Timestamps in the output file will be seconds in floating point format type. Start and stop tick arguments are also
  floating point numbers \[*s*\].

- **[Qeexo V2 CSV format](https://docs.qeexo.com/guides/userguides/data-management#2-1-Data-format-specification)**  
  Format: `qeexo_v2_csv`

  By default interval of 50 ms is used for timestamp increments. User can override this setting by
  passing number of ms after `--interval` flag. User can also define text in label column by passing a string
  after `--label` flag.

  Timestamps in the output file will be milliseconds in integer format type. Start and stop tick arguments are also
  integer numbers \[*ms*\].

- **Audio WAV**  
  Format: `audio_wav`

  Convert .sds to audio WAV format. It takes one sensor and appends required wave header, derived from the
  parameters in the metadata file.

## Set-up and requirements
### Requirements
- Python 3.9 or later with packages:
  - pyyaml
  - pandas

### Set-up
1. Open terminal in SDS-Convert root folder
2. Check installed Python version with:
   ```
   python --version
   ```
3. (Optional) Use Python environment
   1. Create Python environment:
      ```
      python -m venv <env_name>
      ```
      >Note: Usually **`env`** is used for `<env_name>`
   2. Activate created Python environment:
      ```
      <env_name>/Scripts/activate
      ```
4. Install required Python packages:
   ```
   pip install pyyaml pandas
   ```

## Usage
Print help (*common*) with:
```
python sds-convert.py --help
```

```
usage: sds-convert.py [-h] {audio_wav,simple_csv,qeexo_v2_csv} ...

Convert from or to SDS files using selected data conversion format

positional arguments:
  {audio_wav,simple_csv,qeexo_v2_csv}  Data conversion format

options:
  -h, --help                           show this help message and exit
```

### Audio WAV
Print help (*audio_wav*) with:
```
python sds-convert.py audio_wav --help
```

```
usage: sds-convert.py audio_wav [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
```
> Note that metadata and SDS file pairs must be passed as arguments in the same order to ensure recorded data
is decoded correctly.

### Simple CSV
Print help (*simple_csv*) with:
```
python sds-convert.py simple_csv --help
```

```
usage: sds-convert.py simple_csv [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]] [--normalize] [--start-tick <start-tick>] [--stop-tick <stop-tick>]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
  --normalize                         Normalize timestamps so they start with 0
  --start-tick <start-tick>           Exported data start tick (default: None)
  --stop-tick <stop-tick>             Exported data stop tick (default: None)
```
> Note that metadata and SDS file pairs must be passed as arguments in the same order to ensure recorded data
is decoded correctly.

### Qeexo V2 CSV
Print help (*qeexo_v2_csv*) with:
```
python sds-convert.py qeexo_v2_csv --help
```

```
usage: sds-convert.py qeexo_v2_csv [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]] [--normalize] [--start-tick <start-tick>] [--stop-tick <stop-tick>] [--label 'label'] [--interval <interval>] [--sds_index <sds_index>]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
  --normalize                         Normalize timestamps so they start with 0
  --start-tick <start-tick>           Exported data start tick (default: None)
  --stop-tick <stop-tick>             Exported data stop tick (default: None)
  --label 'label'                     Qeexo class label for sensor data (default: None)
  --interval <interval>               Qeexo timestamp interval in ms (default: 50)
  --sds_index <sds_index>             SDS file index to write (default: <sensor>.0.sds)
```
> Note that metadata and SDS file pairs must be passed as arguments in the same order to ensure recorded data
is decoded correctly.

### Examples
- Audio WAV
  ```
  python sds-convert.py audio_wav -i Microphone.0.sds -o microphone.wav -y Microphone.sds.yml
  ```

- Simple CSV
  ```
  python sds-convert.py simple_csv  -i Gyroscope.0.sds -o gyroscope_simple.csv -y Gyroscope.sds.yml --normalize --start-tick 0.2 --stop-tick 0.3
  ```

- Qeexo V2 CSV
  - SDS to CSV
    ```
    python sds-convert.py qeexo_v2_csv -i Gyroscope.0.sds Accelerometer.0.sds -o sensor_fusion.csv -y Gyroscope.sds.yaml Accelerometer.sds.yaml --normalize --start-tick 200 --stop-tick 300
    ```

  - CSV to SDS
    ```
    python sds-convert qeexo_v2_csv -i accelerometer_data.csv -o accelerometer.sds
    ```

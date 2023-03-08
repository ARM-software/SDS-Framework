# SDS-Convert
Convert SDS data recordings to selected format, based on descriptions found in metadata (YAML) files.  
By default raw timestamps are used for the output file. User can override this behaviour using `--normalize` flag.
Output timestamps will then start with 0. User can also select start and stop tick for the data to be converter to the
specified format using `--start-tick <tick>` and `--stop-tick <tick>` respectively. Both parameters are based on 
timestamp format in the output file.

## Supported formats
- **Simple CSV**  
   Format flag: `-f simple_csv`

   Convert .sds to simple CSV format. It takes one sensor and converts data for each record. In case of sensor with
   multiple channels, each channel will be presented in its own column.

   Timestamps in the output file will be seconds in floating point format type. Start and stop tick arguments are also
   floating point numbers \[*s*\].

- **[Qeexo V2 CSV format](https://docs.qeexo.com/guides/userguides/data-management#2-1-Data-format-specification)**  
   Format flag: `-f qeexo_v2_csv`

   By default interval of 50 ms is used for timestamp increments. User can override this setting by
   passing number of ms after `--interval` flag. User can also define text in label column by passing a string
   after `--label` flag.

   Timestamps in the output file will be milliseconds in integer format type. Start and stop tick arguments are also
   integer numbers \[*ms*\].

- **Audio WAV**  
   Format flag: `-f audio_wav`

   Convert .sds to audio WAV format. It takes one sensor and appends required wave header, derived from the
   parameters in the metadata file.

## Set-up and requirements
### Requirements
- Python 3.9 or later with packages:
  - pyyaml

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
   pip install pyyaml
   ```

## Usage
Print help with:
```
python sds-convert.py --help
```

```
usage: sds-convert.py [-h] -y <yaml_file> [<yaml_file> ...] -s <sds_file> [<sds_file> ...] -o <output_file> -f {simple_csv,qeexo_v2_csv,audio_wav}
                      [--normalize] [--start-tick <start-tick>] [--stop-tick <stop-tick>] [--label 'label'] [--interval <interval>]

Convert SDS data to selected format

options:
  -h, --help                              show this help message and exit

required:
  -y <yaml_file> [<yaml_file> ...]        YAML sensor description file
  -s <sds_file> [<sds_file> ...]          SDS data recording file
  -o <output_file>                        Output file
  -f {simple_csv,qeexo_v2_csv,audio_wav}  Output data format

optional:
  --normalize                             Normalize timestamps so they start with 0
  --start-tick <start-tick>               Exported data start tick (default: None)
  --stop-tick <stop-tick>                 Exported data stop tick (default: None)
  --label 'label'                         Qeexo class label for sensor data (default: None)
  --interval <interval>                   Qeexo timestamp interval in ms (default: 50)
```

### Run tool
To convert data into selected format run:
```
python sds-convert.py -y <description_filename>.yml [<description_filename2>.yml ...] -s <sds_data_filename>.sds [<sds_data_filename2>.sds ...] -o <output_file> -f <format> [--normalize] [--label "label'] [--interval <interval>]
```
Note that metadata and SDS file pairs must be passed as arguments in the same order to ensure recorded data
is decoded correctly.

### Examples
- Basic use case 
   - Simple CSV
      ```
      python sds-convert.py -y Gyroscope.sds.yml -s Gyroscope.0.sds -o gyroscope_simple.csv -f simple_csv
      ```

   - Qeexo V2 CSV
      ```
      python sds-convert.py -y Gyroscope.sds.yaml Accelerometer.sds.yaml -s Gyroscope.0.sds Accelerometer.0.sds -o sensor_fusion.csv -f qeexo_v2_csv
      ```

- Basic use case with normalized timestamps:  
   Timestamps in output file will start with 0.
   - Simple CSV
      ```
      python sds-convert.py -y Gyroscope.sds.yml -s Gyroscope.0.sds -o gyroscope_simple.csv -f simple_csv --normalize
      ```

   - Qeexo V2 CSV
      ```
      python sds-convert.py -y Gyroscope.sds.yaml Accelerometer.sds.yaml -s Gyroscope.0.sds Accelerometer.0.sds -o sensor_fusion.csv -f qeexo_v2_csv --normalize
      ```

- Use case with start and stop flags:  
   Only data with timestamps between start and stop tick arguments will be captured in the  output file.
   - Simple CSV (timestamps in s)
      ```
      python sds-convert.py -y Gyroscope.sds.yml -s Gyroscope.0.sds -o gyroscope_simple.csv -f simple_csv --start-tick 0.2 --stop-tick 0.3
      ```

   - Qeexo V2 CSV (timestamps in ms)
      ```
      python sds-convert.py -y Gyroscope.sds.yaml Accelerometer.sds.yaml -s Gyroscope.0.sds Accelerometer.0.sds -o sensor_fusion.csv -f qeexo_v2_csv --start-tick 200 --stop-tick 300
      ```

- Use case with normalized start and stop flags:  
   Only data with normalized timestamps between start and stop tick arguments will be captured in the  output file.  
   - Simple CSV (timestamps in s)
      ```
      python sds-convert.py -y Gyroscope.sds.yml -s Gyroscope.0.sds -o gyroscope_simple.csv -f simple_csv --normalize --start-tick 0.2 --stop-tick 0.3
      ```

   - Qeexo V2 CSV (timestamps in ms)
      ```
      python sds-convert.py -y Gyroscope.sds.yaml Accelerometer.sds.yaml -s Gyroscope.0.sds Accelerometer.0.sds -o sensor_fusion.csv --normalize -f qeexo_v2_csv --start-tick 200 --stop-tick 300
      ```

- Use case with audio format:
   - Audio WAV
      ```
      python sds-convert.py -y Microphone.sds.yml -s Microphone.0.sds -f audio_wav -o microphone.wav
      ```

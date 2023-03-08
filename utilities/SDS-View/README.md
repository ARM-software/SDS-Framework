# SDS-View
View time based plot of SDS data recording, based on the description found in metadata (YAML) file. 

Horizontal time scale is derived from the number of data points in a recording and frequency 
provided in the metadata description. All plots form a single recording will be displayed on the 
same figure (shared vertical scale).

If there are 3 values described in the metadata file, an additional 3D view will be displayed.  
The tool also supports plotting of multiple recordings at the same time, by listing their paths 
after the `-s` flag.  
Note that in this case all recordings will be processed and decoded based on the description in 
the metadata file listed after the `-y` flag.

## Limitations
- Data in recording must all be of the same type (float, uint32_t, uint16_t, ...)

## Set-up and requirements
### Requirements
- Python 3.9 or later with packages:
  - pyyaml
  - numpy
  - matplotlib

### Set-up
1. Open terminal in SDS-View root folder
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
   3. Install required Python packages using [requirements.txt](./requirements.txt):
      ```
      pip install -r requirements.txt
      ```
   4. Skip next step
4. Install required Python packages:
   ```
   pip install pyyaml numpy matplotlib
   ```

## Usage
Print help with:
```
python sds-view.py --help
```
```
usage: sds-view.py [-h] -y <yaml_file> -s <sds_file> [<sds_file> ...] [--3D]

View SDS data

options:
  -h, --help                      show this help message and exit

required:
  -y <yaml_file>                  YAML sensor description file
  -s <sds_file> [<sds_file> ...]  SDS data recording file

optional:
  --3D                            Plot 3D view in addition to normal 2D
```
### Run tool
To plot SDS data on run:
```
python sds-view.py -y <description_filename>.yml -s <sds_data_filename>.sds [<sds_data_filename2>.sds ...]
```

### Examples
- Gyroscope:
   ```
   python sds-view.py -y test/Gyroscope.sds.yml -s test/Gyroscope.0.sds
   ```
- Accelerometer:
   ```
   python sds-view.py -y test/Accelerometer.sds.yml -s test/Accelerometer.0.sds
   ```
- Temperature:
   ```
   python sds-view.py -y test/Temperature.sds.yml -s test/Temperature.0.sds
   ```

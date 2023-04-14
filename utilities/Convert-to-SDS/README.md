# Convert-to-SDS

Convert from other formats into SDS data recording.

## Supported formats

- **Simple CSV**  
   Format flag: `-f simple_csv`

   Convert from a simple sensor CSV format to a .sds file.

   _Note: Still to be implemented._

- **[Qeexo V2 CSV format](https://docs.qeexo.com/guides/userguides/data-management#2-1-Data-format-specification)**  
   Format flag: `-f qeexo_v2_csv`

   Convert from a CSV file in the Qeexo AutoML V2 format to .sds files.
   This will simply convert the data stream from Qeexo AutoML V2 CSV format to the binary .sds format.
   It is the responsibility of the user to create the `<sensor>.sds.yml` file to describe each sensor data format.

## Set-up and requirements

### Requirements

- Python 3.9 or later with packages:
  - pyyaml

### Set-up

1. Open terminal in Convert-to-SDS root folder
2. Check installed Python version with:

   ```bash
   python --version
   ```

3. (Optional) Use Python environment
   1. Create Python environment:

      ```bash
      python -m venv <env_name>
      ```

      >Note: Usually **`env`** is used for `<env_name>`
   2. Activate created Python environment:

      ```bash
      <env_name>/Scripts/activate
      ```

4. Install required Python packages:

   ```bash
   pip install pyyaml
   ```

## Usage

Print help with:

```bash
python convert-to-sds.py --help
```

```bash
```

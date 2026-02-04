# Synchronous Data Stream - File Format

The **SDS Framework** uses a binary data file format to store the individual data streams. It supports the recording and playback of multiple data streams that may have jitters.  Therefore each stream contains timestamp information that allows to correlate the data streams as it is for example required in a sensor fusion application.

The binary data format (stored in `*.<n>.sds` data files) has a record structure with a variable size. Each record has the following format:

1. **timestamp**: record timestamp in tick-frequency (32-bit unsigned integer, little endian)
2. **data size**: number of data bytes in the record (32-bit unsigned integer, little endian)
3. **binary data**: SDS stream (little endian, no padding) as described with the `*.sds.yml` file.

The content of each data stream is described in a [YAML](https://en.wikipedia.org/wiki/YAML) metadata file that is created by the user.

## YAML Format

The following section defines the YAML format of this metadata file. The file `sds.schema.json` is a schema description of the SDS Format Description.

`sds:`                               | Start of the SDS Format Description
:------------------------------------|---------------------------------------------------
&nbsp;&nbsp;&nbsp; `name:`           | Name of the Synchronous Data Stream (SDS)
&nbsp;&nbsp;&nbsp; `description:`    | Additional descriptive text (optional)
&nbsp;&nbsp;&nbsp; `frequency:`      | Capture frequency of the SDS
&nbsp;&nbsp;&nbsp; `tick-frequency:` | Tick frequency of the timestamp value (optional); default: 1000 for 1 milli-second interval
&nbsp;&nbsp;&nbsp; `content:`        | List of values captured (see below)

`content:`                           | List of values captured (in the order of the data file)
:------------------------------------|---------------------------------------------------
`- value:`                           | Name of the value
&nbsp;&nbsp;&nbsp; `type:`           | Data type of the value
&nbsp;&nbsp;&nbsp; `offset:`         | Offset of the value (optional); default: 0
&nbsp;&nbsp;&nbsp; `scale:`          | Scale factor of the value (optional); default: 1.0
&nbsp;&nbsp;&nbsp; `unit:`           | Physical unit of the value (optional); default: no units
&nbsp;&nbsp;&nbsp; `image:`          | Image format metadata (optional)

### Image Format Metadata Fields

The `image` fields provide metadata for image data captured in the SDS stream. When a content item represents image data, `image` describes the format, dimensions, and memory layout.

`image:`                             | Image stream metadata (all fields required except where noted)
:------------------------------------|---------------------------------------------------
&nbsp;&nbsp;&nbsp; `pixel_format:`   | Pixel format identifier (enum)
&nbsp;&nbsp;&nbsp; `width:`          | Number of pixels per row (integer, minimum: 1)
&nbsp;&nbsp;&nbsp; `height:`         | Number of rows (integer, minimum: 1)
&nbsp;&nbsp;&nbsp; `stride_bytes:`   | Bytes per row for single-plane formats (required for single-plane)
&nbsp;&nbsp;&nbsp; `planes:`         | Per-plane stride array for multi-plane formats (required for multi-plane)

The `pixel_format` field accepts the following identifiers:

- **Single-plane formats**: `RAW8`, `RAW10`, `RGB565`, `RGB888`, `YUYV`, `UYVY`
- **Multi-plane formats**: `NV12`, `NV21`, `I420`, `NV16`, `NV61`, `YUV422P`, `YUV444`, `YUV444P`

## Examples

### Sensor Data Stream

This example defines a data stream with the name "sensorX" that contains the values of a gyroscope, temperature sensor, and additional raw data (that are not further described).

![image](https://user-images.githubusercontent.com/8268058/208393980-ebe82918-625b-46d7-8f16-74590f8e1ea2.png)

The binary data that are coming form this sensors are stored in data files with the following file format: `<sensor-name>.<file-index>.sds`. In this example the files names could be:

```yml
   sensorX.0.sds   # capture 0
   sensorX.1.sds   # capture 1
```

The following `sensorX.sds.yml` provides the format description of the SDS `sensorX` binary data files and maybe used by data conversion utilities and data viewers.

```yml
sds:                   # describes a synchronous data stream
  name: sensorX        # user defined name
  description: Gyroscope stream with 1KHz, plus additional user data
  frequency: 1000
  content:
  - value: x           # Value name is 'x'
    type:  uint16_t    # stored using a 16-bit unsigned int
    scale: 0.2         # value is scaled by 0.2
    unit: dps          # base unit of the value
  - value: y
    type: uint16_t
    scale: 0.2
    unit: dps
  - value: z
    type: uint16_t
    unit: dps          # scale 1.0 is default
  - value: temp
    type: float
    unit: degree Celsius
  - value: raw
    type: uint16_t     # raw data, no scale or unit given
  - value: flag
    type: uint32_t:1   # a single bit stored in a 32-bit int
```

### Video Frame Stream

This example shows a video stream capturing RGB888 frames at 30 Hz. Each frame is 640x480 pixels with 3 bytes per pixel (RGB), requiring a stride of 1920 bytes per row.

```yml
sds:
  name: Camera stream
  description: RGB888 video capture at 30 fps
  frequency: 30
  content:
  - value: frame
    type: uint8_t
    image:
      pixel_format: RGB888
      width: 640
      height: 480
      stride_bytes: 1920   # 640 pixels * 3 bytes/pixel
```

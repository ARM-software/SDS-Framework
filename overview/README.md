# Overview

The Synchronous Data Stream (SDS) Framework manages data streams and provides tools for developing and optimizing embedded applications using DSP, ML, or Edge AI algorithms. It enables real-time capture of multiple data streams—such as sensor, audio, and video inputs—alongside algorithm outputs directly on target hardware. Using the [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil), these streams may be stored in files on a host computer or on memory cards in the embedded system.

![Data capturing and playback in Target System](./SDSIO.png)

The captured data streams are useful in various steps of the development cycle, for example to:

- Validate physical input signals from sensors or output of algorithms.
- Provide input data to Digital Signal Processing (DSP) development tools (such as filter designers) or MLOps systems (for AI model training).
- Provide input data for simulation using [Arm Virtual Hardware (AVH-FVP)](https://github.com/Arm-software/AVH) models for testing and validation, for example in CI systems.

![CI Workflow with Simulation](./Simulation.png)

With integration into MLOps systems, the SDS Framework can be used to provide input data to ML/AI development systems for model classification, training, and performance optimization.

![MLOps Integration](./MLOps.png)

## Features

- Implements a flexible data stream management for sensor, audio, and video interfaces that process data streams.
- Supports data streams from multiple interfaces (i.e. for sensor fusion) including provisions for time drifts.
- **Record real-world data** with original data sources of the target hardware for analysis and development.
- **Playback real-world data** for algorithm validation using target hardware or [Arm Virtual Hardware - FVP](https://github.com/arm-software/avh).

The captured data streams are stored in SDS data files. A [YAML metadata file](./schema/README.md) can be used to describe the content. The SDS data files have several use cases such as:

- Input to Digital Signal Processing (DSP) development tools such as filter designers
- Input to Machine Learning (ML) model classification, training, and performance optimization 
- Verify execution of DSP algorithm on Cortex-M targets with off-line tools

[Python-based utilities](./utilities/README.md) are provided for recording, playback, visualization, and data conversion

## Links

- [Documentation](https://arm-software.github.io/SDS-Framework/main/index.html)
- [Examples](https://github.com/Arm-Examples/SDS-Examples)
- [Repository](https://github.com/ARM-software/SDS-Framework)
- [Issues](https://github.com/ARM-software/SDS-Framework/issues)

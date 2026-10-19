# Parallel Cryptographic Hash & Merkle Tree Verification Engine

A C-based firmware integrity verification project for QNX Neutrino RTOS. The server receives a 64 KiB firmware image from a client, hashes sixteen 4 KiB blocks in parallel using four POSIX worker threads, builds a SHA-256 Merkle tree, compares the root with a predefined golden root, and returns an integrity flag and block mask.

## Features

- QNX client-server architecture using the native QNX message-passing API.
- Two-part IOV request containing a message header and the 64 KiB firmware payload.
- Four worker threads calculating sixteen SHA-256 leaf hashes.
- Merkle tree reduction from 16 leaf hashes to one root.
- QNX completion event/pulse and synchronous message reply.
- Tkinter ground station for fault injection, SSH deployment, JSON result parsing, and block-grid visualization.

## Architecture

The host GUI prepares the image and transfers it to the QNX target over SSH. The QNX client sends the header and payload to the named verification server. The server hashes the blocks in parallel, builds the Merkle root and sends the result back to the client.

See [System Architecture](docs/ARCHITECTURE.md) and [Project Report Summary](docs/PROJECT_REPORT.md).

## Test Results

The following illustrations are reconstructed from the results shown in the submitted project report. See [test documentation](tests/README.md) for the test cases and result-mask calculations.

### Intact firmware — VALID, mask 0

With no fault-injection blocks selected, the report records a valid image and a zero corruption mask.

![Intact firmware result](tests/valid_firmware_result.svg)

### Corrupted firmware — blocks 3, 6, 9 and 12, mask 4680

The report records a corrupted status when blocks 3, 6, 9 and 12 are marked.

![Corrupted firmware result](tests/corrupted_firmware_blocks_3_6_9_12.svg)

The report also records a mask of 33825 for the example with blocks 0, 5, 10 and 15 marked. These are values documented in the report, not fresh test runs performed during this repository update.

## Repository Layout

    qnx_source/
      client/
        .project
        .cproject
        Makefile
        src/client.c
        src/protocol.h
      server/
        .project
        .cproject
        Makefile
        src/server.c
        src/protocol.h
        src/sha256.c
        src/sha256.h
    ground_station/
      ground_station.py
      requirements.txt
      .env.example
    docs/
      ARCHITECTURE.md
      PROJECT_REPORT.md
    tests/
      README.md
      valid_firmware_result.svg
      corrupted_firmware_blocks_3_6_9_12.svg

## Build the QNX Programs

A QNX Software Development Platform (SDP) / Momentics environment with the QNX compiler is required. These programs are targeted by the supplied Makefiles at aarch64le.

From a QNX command shell, run:

    cd qnx_source/client
    make

Then build the server:

    cd qnx_source/server
    make

The expected output paths are under each project's build/aarch64le-debug directory. This repository setup has not been compiled in a fresh QNX environment, so use Momentics or your QNX build environment to confirm the build for your installed SDP version.

Copy the resulting merkle-client and merkle-server executables to the QNX target. Start merkle-server first so it can register the service name; then launch the ground station. The GUI expects the client executable to already be available at /tmp/merkle-client on the target.

## Run the Host Ground Station

Requirements: Python 3, Tkinter, an SSH-accessible QNX target, and Paramiko.

Install the dependency from the repository root:

    python -m pip install -r ground_station/requirements.txt

Set the connection variables in your shell before launching the GUI. For example, in PowerShell:

    $env:QNX_HOST = "10.0.0.1"
    $env:QNX_USER = "root"
    $env:QNX_PASSWORD = "your-target-password"
    python ground_station/ground_station.py

Change the host and username to match your target. The password is read from the environment and is not stored in the Python source. Do not commit real passwords, private keys, or populated environment files. Root SSH login and automatic host-key acceptance are suitable only for a controlled lab demonstration; production deployments should use a dedicated account and verified host keys.

The GUI generates a 64 KiB image filled with 0x01. Each selected 4 KiB block is marked by changing its first byte to 0xFF. The GUI streams the image to /tmp/update.bin over SSH and runs the client, then parses the returned JSON and displays the result.

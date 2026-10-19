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

## Repository layout

    qnx_source/
      client/
        Makefile
        .project
        src/client.c
        src/protocol.h
      server/
        Makefile
        .project
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

## Build the QNX programs

A QNX Software Development Platform (SDP) / Momentics environment with the QNX compiler is required. These programs are targeted by the supplied Makefiles at aarch64le.

From a QNX command shell, run:

    cd qnx_source/client
    make

Then build the server:

    cd qnx_source/server
    make

The expected output paths are under each project's build/aarch64le-debug directory. This repository setup has not been compiled in a fresh QNX environment, so use Momentics or your QNX build environment to confirm the build for your installed SDP version.

Copy the resulting merkle-client and merkle-server executables to the QNX target. Start merkle-server first so it can register the service name; then launch the ground station. The GUI expects the client executable to already be available at /tmp/merkle-client on the target.

## Run the host ground station

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

## Recorded test results

The submitted project report records these representative results:

| Scenario | Expected / recorded result |
|---|---|
| No blocks changed | VALID, mask 0 |
| Blocks 3, 6, 9, and 12 marked | CORRUPTED, mask 4680 |
| Blocks 0, 5, 10, and 15 marked | Mask 33825 |

These are report-recorded results, not tests executed by GitHub or this documentation update.

## Current implementation limitations

- The corruption mask identifies blocks by checking the first byte of each block against the 0x01 test pattern. Changes elsewhere in a block may cause a root mismatch without setting the corresponding mask bit.
- Header metadata fields are not fully populated or validated.
- Not every QNX API return value is checked, and the full 10-second connection retry behavior described in the requirements is not implemented.
- The server sends the completion event before the synchronous reply, so the client remains blocked while hashing is in progress.
- The golden root, payload size and block geometry are compile-time constants.

## Team

The submitted report lists the following contributors:

- K A Vignesvaran — CB.EN.U4ELC23021
- Kiruthik Pranav T A — CB.EN.U4ELC23023
- Vidhyarth S E — CB.EN.U4ELC23057

## License

See LICENSE. Confirm that all project contributors and any applicable course/institution rules permit public redistribution under the selected license.

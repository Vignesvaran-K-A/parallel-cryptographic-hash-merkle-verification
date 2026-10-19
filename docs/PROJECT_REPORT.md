# Project Report Summary

**Project:** Parallel Cryptographic Hash & Merkle Tree Verification Engine  
**Platform:** QNX Neutrino RTOS  
**Course:** 26ELC331 — Real Time Operating System using QNX  
**Report date:** October 2026

This document summarizes the submitted project report. The original 40-page report PDF is not uploaded in this commit; add it to this directory through GitHub's web upload interface when desired.

## Objective

Build a firmware-integrity verifier in which a QNX client sends a firmware image to a server using native QNX message passing. The server hashes the image in parallel, combines the leaf hashes into a Merkle tree, compares the computed root with a predefined reference root, and returns a validity flag plus a block-corruption mask.

## Design described in the report

- Firmware payload: 65,536 bytes (64 KiB).
- Block size: 4,096 bytes (4 KiB).
- Number of leaf blocks: 16.
- Digest algorithm: SHA-256, producing a 32-byte digest.
- Worker threads: 4; each hashes 4 blocks.
- IPC: QNX client-server message passing with a two-part IOV (request header and firmware payload).
- Result: a 32-bit word returned through a QNX pulse; bit 31 represents the validity flag and bits 0–15 represent the block mask.
- Host GUI: Tkinter dashboard; Paramiko is used for SSH deployment and remote command execution.

## Verification workflow

1. The ground station creates a 64 KiB image filled with byte 0x01.
2. Selected blocks are marked by changing the first byte of each selected 4 KiB block to 0xFF.
3. The payload is streamed to the target over SSH and the QNX client is launched.
4. The client sends the request header and payload to the named server service using MsgSendv.
5. Four worker threads calculate the SHA-256 leaf digests concurrently.
6. The server builds the Merkle tree and compares the resulting root to PREDEFINED_GOLDEN_ROOT.
7. The server creates the result word, delivers the completion event/pulse and replies to the client.
8. The client prints JSON; the ground station parses it and visualizes the block mask.

## Results recorded in the submitted report

| Test case | Reported result |
|---|---|
| Intact image | VALID, mask 0 |
| Blocks 3, 6, 9, and 12 marked as corrupted | CORRUPTED, mask 4680 |
| Blocks 0, 5, 10, and 15 marked as corrupted | Mask 33825 |

These are results recorded by the report, not new tests run by this repository setup.

## Known limitations reported

- The current corruption mask checks the first byte of each block for the test marker. It is not a general per-block comparison against reference leaf hashes.
- Request metadata fields for payload size, block count, and block size are not populated/validated in the current client-server exchange.
- Some QNX API return values are not checked, and the client does not implement the full 10-second retry requirement.
- The server sends the event before replying, meaning the client remains blocked during verification.
- Payload geometry and the golden root are compile-time constants.

## Team

The submitted report lists these team members:

- K A Vignesvaran — CB.EN.U4ELC23021
- Kiruthik Pranav T A — CB.EN.U4ELC23023
- Vidhyarth S E — CB.EN.U4ELC23057

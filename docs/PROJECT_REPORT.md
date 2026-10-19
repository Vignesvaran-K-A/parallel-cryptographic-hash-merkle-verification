# Project Report Summary

**Project:** Parallel Cryptographic Hash & Merkle Tree Verification Engine  
**Platform:** QNX Neutrino RTOS  
**Course:** 26ELC331 — Real Time Operating System using QNX  
**Report date:** October 2026

This summary is based on the submitted 40-page project report. The original PDF is a separate binary file and is not stored in the repository yet.

## Objective

Build a firmware-integrity verifier in which a QNX client sends a firmware image to a server using native QNX message passing. The server hashes the image in parallel, combines leaf hashes into a Merkle tree, compares the computed root with a predefined reference root, and returns an integrity flag plus a block-corruption mask.

## Design described in the report

- Firmware payload: 65,536 bytes (64 KiB).
- Block size: 4,096 bytes (4 KiB).
- Number of leaf blocks: 16.
- Digest algorithm: SHA-256, producing a 32-byte digest.
- Worker threads: 4; each hashes 4 blocks.
- IPC: QNX native message passing with a two-part IOV request.
- Result: a 32-bit word returned through a QNX pulse; bit 31 represents the validity flag and bits 0–15 represent the block mask.
- Host GUI: Tkinter dashboard; Paramiko is used for SSH deployment and remote command execution.

## Verification workflow

1. The ground station creates a 64 KiB image filled with byte `0x01`.
2. Each selected 4 KiB block is marked by changing its first byte to `0xFF`.
3. The payload is streamed to the target over SSH and the QNX client is launched.
4. The client sends the request header and payload to the named server service using `MsgSendv`.
5. Four worker threads calculate SHA-256 leaf digests concurrently.
6. The server builds the Merkle tree and compares the resulting root with `PREDEFINED_GOLDEN_ROOT`.
7. The server sends the result using the completion event/pulse and a synchronous reply.
8. The client prints JSON; the ground station parses it and visualizes the result.

## Results recorded in the submitted report

| Test case | Reported result |
|---|---|
| Intact firmware image | `VALID`, mask `0` |
| Blocks 3, 6, 9, and 12 marked | `CORRUPTED`, mask `4680` |
| Blocks 0, 5, 10, and 15 marked | Mask `33825` |

These values are transcribed from the report; they were not re-tested as part of repository documentation.

## Limitations reported

- The corruption mask checks the first byte of each block for the injected marker. This test-specific approach does not localize arbitrary corruption elsewhere in a block.
- Request metadata fields are not fully populated or validated.
- Some QNX API return values are not checked, and the full 10-second connection retry requirement is not implemented.
- The server delivers the event before replying, so the client remains blocked during verification.
- Payload geometry and the golden root are compile-time constants.

## Team

The submitted report lists:

- K A Vignesvaran — CB.EN.U4ELC23021
- Kiruthik Pranav T A — CB.EN.U4ELC23023
- Vidhyarth S E — CB.EN.U4ELC23057

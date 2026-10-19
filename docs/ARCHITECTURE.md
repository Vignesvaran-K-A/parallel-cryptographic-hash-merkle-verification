# System Architecture

## Main components

- **Host ground station:** generates the firmware payload, injects test markers, streams it to the target over SSH, runs the client, and visualizes the result.
- **QNX client:** reads the image and sends a header plus the 64 KiB payload as a two-part IOV message.
- **QNX Neutrino microkernel:** mediates process message passing and delivery of the reply and pulse.
- **QNX server:** receives the payload into a private buffer and performs hashing and verification with four worker threads.
- **SHA-256 module:** hashes each of the sixteen 4 KiB blocks into a 32-byte leaf digest.
- **Merkle tree:** recursively hashes pairs of child digests until one root remains.

## End-to-end workflow

~~~mermaid
flowchart LR
    A[Host GUI] -->|SSH: stream update.bin| B[QNX target RAM disk]
    A -->|SSH: launch client| C[merkle-client]
    C -->|MsgSendv: header + 64 KiB IOV| K[QNX Neutrino IPC]
    K --> D[merkle-server]
    D --> E[Four worker threads]
    E --> F[16 SHA-256 leaf hashes]
    F --> G[Merkle tree reduction]
    G --> H[Compare computed root with golden root]
    H --> I[Pack integrity bit and 16-bit block mask]
    I -->|MsgDeliverEvent pulse + MsgReply| K
    K --> C
    C -->|JSON result over SSH| A
~~~

## Hashing and block layout

The report describes a 64 KiB payload divided into sixteen 4 KiB blocks. Four worker threads each hash four blocks. The server then combines the sixteen 32-byte leaf digests level by level:

~~~text
16 leaf hashes -> 8 parent hashes -> 4 -> 2 -> 1 Merkle root
~~~

For each parent node:

~~~text
parent = SHA256(left_digest || right_digest)
~~~

## Result word

- Bit 31: integrity flag (1 when the computed root matches the golden root).
- Bits 15–0: corruption mask; bit i indicates that block i matches the test corruption-marker rule.
- The current source builds this mask by checking the first byte of each block. A root mismatch will detect a modified image, but the mask can miss corruption outside the first byte unless the marker rule is changed or per-leaf reference hashes are compared.

## IPC sequence

1. The server registers the name merkle_verification_service.
2. The client opens the service by name and creates a local channel for the result pulse.
3. The client sends the request header and the firmware as an IOV.
4. The server computes the hashes, builds the Merkle root and prepares the result word.
5. The server delivers the event and replies to release the client.
6. The client decodes the pulse and emits a JSON line for the host GUI.

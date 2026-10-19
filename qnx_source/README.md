# QNX Source Code

This directory contains the QNX Neutrino C implementation of the Parallel Cryptographic Hash and Merkle Tree Verification Engine.

The QNX client sends a 64 KB firmware payload to the verification server using native message passing and an I/O vector. The server computes SHA-256 leaf hashes with worker threads, builds a Merkle tree, and reports the verification status and block mask.

See the client and server project directories for the source code and Momentics build configuration.
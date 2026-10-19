/*
 * ===========================================================================
 * Project Title      : Parallel Cryptographic Hash & Merkle Tree
 *                      Verification Engine (QNX Neutrino RTOS)
 * File               : sha256.h (merkle-server)
 * Brief Note on Project :
 *     A zero-touch OTA firmware-integrity verifier for a Raspberry Pi 4
 *     running the QNX Neutrino microkernel. A 64 KB firmware image is split
 *     into sixteen 4 KB blocks, hashed with SHA-256 by four parallel worker
 *     threads, folded into a Merkle tree and compared with a stored golden
 *     root. The verdict and a 16-bit corruption mask are returned to the
 *     host laptop as JSON for display on a live dashboard.
 *     This file: public interface of the SHA-256 engine.
 * Team Members       : K A Vignesvaran   - CB.EN.U4ELC23021
 *                      Kiruthik Pranav T A - CB.EN.U4ELC23023
 *                      Vidhyarth S E     - CB.EN.U4ELC23057
 * Course             : 26ELC331 Real Time Operating System using QNX
 * Date of Evaluation : 17 October 2026
 * ===========================================================================
 */

#ifndef SHA256_H_
#define SHA256_H_
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    unsigned long long bitlen;
    uint32_t state[8];
} sha256_ctx;

void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const uint8_t data[], size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t hash[]);

#endif /* SHA256_H_ */

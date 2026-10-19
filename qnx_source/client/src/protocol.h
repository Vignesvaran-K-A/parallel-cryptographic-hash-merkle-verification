/*
 * ===========================================================================
 * Project Title      : Parallel Cryptographic Hash & Merkle Tree
 *                      Verification Engine (QNX Neutrino RTOS)
 * File               : protocol.h (shared by merkle-server and merkle-client)
 * Brief Note on Project :
 *     A zero-touch OTA firmware-integrity verifier for a Raspberry Pi 4
 *     running the QNX Neutrino microkernel. A 64 KB firmware image is split
 *     into sixteen 4 KB blocks, hashed with SHA-256 by four parallel worker
 *     threads, folded into a Merkle tree and compared with a stored golden
 *     root. The verdict and a 16-bit corruption mask are returned to the
 *     host laptop as JSON for display on a live dashboard.
 *     This file: shared contract - constants, message structures and result-word
 *                macros.
 * Team Members       : K A Vignesvaran   - CB.EN.U4ELC23021
 *                      Kiruthik Pranav T A - CB.EN.U4ELC23023
 *                      Vidhyarth S E     - CB.EN.U4ELC23057
 * Course             : 26ELC331 Real Time Operating System using QNX
 * Date of Evaluation : 17 October 2026
 * ===========================================================================
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>

#define ATTACH_POINT            "merkle_verification_service"
#define SUB_BLOCK_SIZE          4096
#define TOTAL_BLOCK_COUNT       16
#define FIRMWARE_PAYLOAD_SIZE   (SUB_BLOCK_SIZE * TOTAL_BLOCK_COUNT)
#define SHA256_DIGEST_LENGTH    32

#define MERKLE_PULSE_CODE       (_PULSE_CODE_MINAVAIL + 5)
#define MERKLE_MSG_TYPE_VERIFY  0xAA01

/*
 * Reference Merkle root for the known-good 64 KB image filled with 0x01.
 * Keep this value consistent with the image used for the project tests.
 */
static const uint8_t PREDEFINED_GOLDEN_ROOT[SHA256_DIGEST_LENGTH] = {
    0x57, 0x10, 0x67, 0x73, 0x3b, 0xe7, 0xa3, 0xbd,
    0x97, 0x60, 0x29, 0x6d, 0xe2, 0xe7, 0xfb, 0x2a,
    0xcc, 0x9a, 0x23, 0x34, 0x08, 0x3a, 0xfc, 0xbd,
    0x33, 0x29, 0x04, 0x1d, 0x1e, 0x1f, 0xd4, 0x56
};

typedef struct {
    uint32_t msg_type;
    uint32_t payload_size;
    uint32_t block_count;
    uint32_t block_size;
    struct sigevent event;
} merkle_msg_header_t;

typedef struct {
    int32_t  status;
    uint32_t result;
} merkle_reply_ack_t;

#define STATUS_VALID_BIT        (1U << 31)
#define GET_CORRUPT_MASK(val)   ((uint32_t)((val) & 0x0000FFFF))
#define IS_INTEGRITY_OK(val)    (((val) & STATUS_VALID_BIT) != 0)

#endif /* PROTOCOL_H_ */

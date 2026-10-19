/*
 * ===========================================================================
 * Project Title      : Parallel Cryptographic Hash & Merkle Tree Verification Engine (QNX Neutrino RTOS)
 * File               : client.c (merkle-client)
 * Brief Note on Project :
 *     A zero-touch OTA firmware-integrity verifier for a Raspberry Pi 4
 *     running the QNX Neutrino microkernel. A 64 KB firmware image is split
 *     into sixteen 4 KB blocks, hashed with SHA-256 by four parallel worker threads,
 *     folded into a Merkle tree and compared with a stored golden
 *     root. The verdict and a 16-bit corruption mask are returned to the
 *     host laptop as JSON for display on a live dashboard.
 *     This file: QNX client: loads update.bin, sends it to the server, waits for
 *                the pulse and prints the JSON verdict.
 * Team Members       : K A Vignesvaran   - CB.EN.U4ELC23021
 *                      Kiruthick Pranav  - CB.EN.U4ELC23023
 *                      Vidhyarth S E     - CB.EN.U4ELC23057
 * Course             : 26ELC331 Real Time Operating System using QNX
 * Date of Evaluation : 17 October 2026
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include "protocol.h"

static uint8_t firmware_data[FIRMWARE_PAYLOAD_SIZE];

int main(int argc, char *argv[]) {
    FILE *fp;
    int chid, coid_local, server_coid;
    merkle_msg_header_t header;
    struct sched_param param;
    int policy;
    iov_t send_iov[2];
    merkle_reply_ack_t reply;
    iov_t recv_iov[1];
    struct _pulse pulse;
    int rcvid;
    uint32_t result_val, corrupt_mask;
    int is_valid;

    if (argc < 2) {
        printf("{\"status\": \"ERROR\", \"message\": \"No firmware file provided\"}\n");
        return EXIT_FAILURE;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("{\"status\": \"ERROR\", \"message\": \"Failed to open file\"}\n");
        return EXIT_FAILURE;
    }
    fread(firmware_data, 1, FIRMWARE_PAYLOAD_SIZE, fp);
    fclose(fp);

    chid = ChannelCreate(0);
    coid_local = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
    server_coid = name_open(ATTACH_POINT, 0);

    if (server_coid == -1) {
        printf("{\"status\": \"ERROR\", \"message\": \"Server not running\"}\n");
        return EXIT_FAILURE;
    }

    memset(&header, 0, sizeof(header));
    header.msg_type = MERKLE_MSG_TYPE_VERIFY;

    pthread_getschedparam(pthread_self(), &policy, &param);

    SIGEV_PULSE_INIT(&header.event, coid_local, param.sched_priority, MERKLE_PULSE_CODE, 0);
    SIGEV_MAKE_UPDATEABLE(&header.event);
    MsgRegisterEvent(&header.event, server_coid);

    SETIOV(&send_iov[0], &header, sizeof(header));
    SETIOV(&send_iov[1], firmware_data, FIRMWARE_PAYLOAD_SIZE);

    SETIOV(&recv_iov[0], &reply, sizeof(reply));

    MsgSendv(server_coid, send_iov, 2, recv_iov, 1);

    rcvid = MsgReceivePulse(chid, &pulse, sizeof(pulse), NULL);

    if (rcvid == 0 && pulse.code == MERKLE_PULSE_CODE) {
        result_val = (uint32_t)pulse.value.sival_int;
        is_valid = IS_INTEGRITY_OK(result_val);
        corrupt_mask = GET_CORRUPT_MASK(result_val);

        printf("{\"status\": \"%s\", \"mask\": %u}\n",
               is_valid ? "VALID" : "CORRUPTED", corrupt_mask);
    }

    MsgUnregisterEvent(&header.event);
    name_close(server_coid);
    ConnectDetach(coid_local);
    ChannelDestroy(chid);
    return EXIT_SUCCESS;
}

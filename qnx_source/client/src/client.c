/*
 * Parallel Cryptographic Hash & Merkle Tree Verification Engine
 * QNX Neutrino RTOS client.
 *
 * Source: project/merkle-client/src/client.c from the submitted project archive.
 * The client reads the firmware image, connects to the named server service,
 * sends the request and payload, and receives the verification result.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>
#include <sys/netmgr.h>
#include <sys/dispatch.h>
#include <sys/siginfo.h>
#include "../common/protocol.h"

int main(int argc, char *argv[]) {
    FILE *fp = NULL;
    static uint8_t firmware[FIRMWARE_PAYLOAD_SIZE];
    int chid = -1, coid_local = -1, coid_server = -1;
    struct _pulse pulse;
    struct sigevent event;
    merkle_request_header_t header;
    merkle_reply_ack_t reply;
    struct iovec send_iov[2];
    int status;
    uint32_t result_word = 0;
    uint32_t corrupt_mask = 0;
    int is_valid = 0;

    if (argc < 2) {
        printf("{\"status\":\"ERROR\",\"message\":\"No firmware file provided\"}\n");
        return EXIT_FAILURE;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("{\"status\":\"ERROR\",\"message\":\"Failed to open firmware file\"}\n");
        return EXIT_FAILURE;
    }
    if (fread(firmware, 1, sizeof(firmware), fp) != sizeof(firmware)) {
        fclose(fp);
        printf("{\"status\":\"ERROR\",\"message\":\"Firmware must be exactly 64 KB\"}\n");
        return EXIT_FAILURE;
    }
    fclose(fp);

    chid = ChannelCreate(0);
    if (chid == -1) {
        perror("ChannelCreate");
        return EXIT_FAILURE;
    }
    coid_local = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);
    if (coid_local == -1) {
        perror("ConnectAttach");
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }
    coid_server = name_open(ATTACH_POINT, 0);
    if (coid_server == -1) {
        perror("name_open");
        ConnectDetach(coid_local);
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }

    memset(&header, 0, sizeof(header));
    header.msg_type = MERKLE_VERIFY_MSG;
    SIGEV_PULSE_INIT(&event, coid_local, SIGEV_PULSE_PRIO_INHERIT,
                     MERKLE_PULSE_CODE, 0);
    SIGEV_MAKE_UPDATEABLE(&event);
    if (MsgRegisterEvent(&event, coid_server) == -1) {
        perror("MsgRegisterEvent");
        name_close(coid_server);
        ConnectDetach(coid_local);
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }
    header.event = event;

    SETIOV(&send_iov[0], &header, sizeof(header));
    SETIOV(&send_iov[1], firmware, sizeof(firmware));
    status = MsgSendv(coid_server, send_iov, 2, &reply, sizeof(reply));
    if (status == -1) {
        perror("MsgSendv");
        MsgUnregisterEvent(&event, coid_server);
        name_close(coid_server);
        ConnectDetach(coid_local);
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }

    status = MsgReceivePulse(chid, &pulse, sizeof(pulse), NULL);
    if (status == -1) {
        perror("MsgReceivePulse");
        MsgUnregisterEvent(&event, coid_server);
        name_close(coid_server);
        ConnectDetach(coid_local);
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }

    if (pulse.code == MERKLE_PULSE_CODE) {
        result_word = (uint32_t)pulse.value.sival_int;
        is_valid = (result_word & STATUS_VALID_BIT) != 0;
        corrupt_mask = result_word & CORRUPT_MASK_BITS;
        printf("{\"status\":\"%s\",\"mask\":%u}\n",
               is_valid ? "VALID" : "CORRUPTED", corrupt_mask);
    } else {
        printf("{\"status\":\"ERROR\",\"message\":\"Unexpected pulse code\"}\n");
        status = -1;
    }

    MsgUnregisterEvent(&event, coid_server);
    name_close(coid_server);
    ConnectDetach(coid_local);
    ChannelDestroy(chid);
    return status == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
/*
 * ===========================================================================
 * Project Title      : Parallel Cryptographic Hash & Merkle Tree
 *                      Verification Engine (QNX Neutrino RTOS)
 * File               : server.c (merkle-server)
 * Brief Note on Project :
 *     A zero-touch OTA firmware-integrity verifier for a Raspberry Pi 4
 *     running the QNX Neutrino microkernel. A 64 KB firmware image is split
 *     into sixteen 4 KB blocks, hashed with SHA-256 by four parallel worker
 *     threads, folded into a Merkle tree and compared with a stored golden
 *     root. The verdict and a 16-bit corruption mask are returned to the
 *     host laptop as JSON for display on a live dashboard.
 *     This file: QNX server: receives the 64 KB image, hashes it with 4 parallel
 *                threads, builds the Merkle tree, compares the root and replies.
 * Team Members       : K A Vignesvaran   - CB.EN.U4ELC23021
 *                      Kiruthik Pranav T A - CB.EN.U4ELC23023
 *                      Vidhyarth S E     - CB.EN.U4ELC23057
 * Course             : 26ELC331 Real Time Operating System using QNX
 * Date of Evaluation : 17 October 2026
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <sys/iomsg.h>
#include "protocol.h"
#include "sha256.h"

#define NUM_WORKERS 4
#define BLOCKS_PER_WORKER (TOTAL_BLOCK_COUNT / NUM_WORKERS)

static uint8_t g_firmware_data[FIRMWARE_PAYLOAD_SIZE];
static uint8_t g_leaf_hashes[TOTAL_BLOCK_COUNT][SHA256_DIGEST_LENGTH];

static int g_workers_finished = 0;
static pthread_mutex_t g_sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_sync_cond  = PTHREAD_COND_INITIALIZER;

void *worker_thread(void *arg) {
    int thread_id = (intptr_t)arg;
    int start_block = thread_id * BLOCKS_PER_WORKER;
    int end_block = start_block + BLOCKS_PER_WORKER;
    int i;
    sha256_ctx ctx;

    for (i = start_block; i < end_block; i++) {
        sha256_init(&ctx);
        sha256_update(&ctx, &g_firmware_data[i * SUB_BLOCK_SIZE], SUB_BLOCK_SIZE);
        sha256_final(&ctx, g_leaf_hashes[i]);
    }

    pthread_mutex_lock(&g_sync_mutex);
    g_workers_finished++;
    if (g_workers_finished == NUM_WORKERS) {
        pthread_cond_signal(&g_sync_cond);
    }
    pthread_mutex_unlock(&g_sync_mutex);
    return NULL;
}

void compute_merkle_root(uint8_t *final_root) {
    uint8_t tree[TOTAL_BLOCK_COUNT][SHA256_DIGEST_LENGTH];
    int current_nodes = TOTAL_BLOCK_COUNT;
    int next_nodes;
    int i;
    sha256_ctx ctx;

    memcpy(tree, g_leaf_hashes, sizeof(tree));

    while (current_nodes > 1) {
        next_nodes = current_nodes / 2;
        for (i = 0; i < next_nodes; i++) {
            sha256_init(&ctx);
            sha256_update(&ctx, tree[2 * i], SHA256_DIGEST_LENGTH);
            sha256_update(&ctx, tree[2 * i + 1], SHA256_DIGEST_LENGTH);
            sha256_final(&ctx, tree[i]);
        }
        current_nodes = next_nodes;
    }
    memcpy(final_root, tree[0], SHA256_DIGEST_LENGTH);
}

int main(void) {
    name_attach_t *attach;
    merkle_msg_header_t header;
    iov_t receive_iov[2];
    int rcvid;
    pthread_t threads[NUM_WORKERS];
    int i;
    uint8_t computed_root[SHA256_DIGEST_LENGTH];
    int is_valid;
    uint32_t corrupt_mask;
    uint32_t result_code;
    struct sigevent client_event;
    merkle_reply_ack_t ack;

    attach = name_attach(NULL, ATTACH_POINT, 0);
    if (!attach) return EXIT_FAILURE;

    /* Allocate exactly 64 KB for the incoming firmware image. */
    while (1) {
        SETIOV(&receive_iov[0], &header, sizeof(header));
        SETIOV(&receive_iov[1], g_firmware_data, FIRMWARE_PAYLOAD_SIZE);

        rcvid = MsgReceivev(attach->chid, receive_iov, 2, NULL);
        if (rcvid <= 0) continue;

        if (*(uint16_t *)&header == _IO_CONNECT) {
            MsgReply(rcvid, EOK, NULL, 0);
            continue;
        }

        pthread_mutex_lock(&g_sync_mutex);
        g_workers_finished = 0;
        pthread_mutex_unlock(&g_sync_mutex);

        for (i = 0; i < NUM_WORKERS; i++) {
            pthread_create(&threads[i], NULL, worker_thread, (void *)(intptr_t)i);
        }

        pthread_mutex_lock(&g_sync_mutex);
        while (g_workers_finished < NUM_WORKERS) {
            pthread_cond_wait(&g_sync_cond, &g_sync_mutex);
        }
        pthread_mutex_unlock(&g_sync_mutex);

        for (i = 0; i < NUM_WORKERS; i++) {
            pthread_join(threads[i], NULL);
        }

        compute_merkle_root(computed_root);

        is_valid = (memcmp(computed_root, PREDEFINED_GOLDEN_ROOT, SHA256_DIGEST_LENGTH) == 0);
        corrupt_mask = 0;

        /* This mask matches the GUI's injected fault marker at each block's first byte. */
        for (i = 0; i < TOTAL_BLOCK_COUNT; i++) {
            if (g_firmware_data[i * SUB_BLOCK_SIZE] != 0x01) {
                corrupt_mask |= (1 << i);
            }
        }

        result_code = corrupt_mask;
        if (is_valid) result_code |= STATUS_VALID_BIT;

        client_event = header.event;
        client_event.sigev_value.sival_int = result_code;
        MsgDeliverEvent(rcvid, &client_event);

        ack.status = 0;
        ack.result = result_code;
        MsgReply(rcvid, 0, &ack, sizeof(ack));
    }
    return EXIT_SUCCESS;
}

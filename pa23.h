#ifndef PA23_H
#define PA23_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"


#define MAX_PROCESS_COUNT 11
#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>

char *const pipe_opened_log = "Pipe (proc %d) from %d to %d - descriptor %d (opened)\n";
char *const pipe_closed_log = "Pipe (proc %d) from %d to %d - descriptor %d (closed)\n";

typedef struct{
    int fd_r;
    int fd_w;
} PipeFileDisc;

typedef struct {
    local_id request_id;
    timestamp_t lamport_time;
} Request;
typedef struct{
    local_id id_now;
    local_id last_sent;
    int iterations;
    timestamp_t lamport_time;
    int q[MAX_PROCESS_ID - 1];
    PipeFileDisc *pipes_arr[11][11];
} WorkInfo;

timestamp_t lamport_time = 0;

timestamp_t get_lamport_time();

int pipe2(int pipefd[2], int flags);

int receive_message(void *self, local_id from, Message *msg);

int receive_multicast(void * self, int16_t type);

int send_message(void *self, local_id dst, const Message *msg);

void dec_work(local_id id);


void work_with_state();

void log_event(int num, local_id lid, local_id to, int balance_or_iteration);

void log_pipe(int type,  int fd_w, int from, int to, int desc);

void open_pipes();

void forc_procs();

void close_red_pipes();

void close_self_pipes();

MessageHeader create_message_header(uint16_t length, int16_t type, timestamp_t time);

void form_history_message(AllHistory* ah);

#endif

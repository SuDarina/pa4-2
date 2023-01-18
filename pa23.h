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

typedef struct{
    local_id id_now;
    balance_t balance_now;
    PipeFileDisc *s_pipes[MAX_PROCESS_COUNT][MAX_PROCESS_COUNT];
} WorkInfo;

timestamp_t lamport_time;

timestamp_t get_lamport_time();

void init_lamport_time();

void inc_lamport_time();

void set_lamport_time(timestamp_t local_time);

int pipe2(int pipefd[2], int flags);
int receive_multicast(void * self, int16_t type);

void dec_work(local_id id);


void work_with_state();

void log_event(int num, local_id lid, local_id to, balance_t balance);

void log_pipe(int type,  int fd_w, int from, int to, int desc);

void open_pipes();

void forc_procs();

void close_red_pipes();

void close_self_pipes();

MessageHeader create_message_header(uint16_t length, int16_t type, timestamp_t time);

void get_history_messages(AllHistory* ah);

#endif

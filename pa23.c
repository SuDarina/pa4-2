#include "pa23.h"
#include "ipc.h"
#include "banking.h"


int fd_pipes_log;
int fd_events_log;

int N;
WorkInfo *work_info;
int use_cs = 0;
char buffer[MAX_MESSAGE_LEN];
int done = 0;
int highest_prio = 0;

timestamp_t get_lamport_time() {
    return lamport_time;
}

void inc_lamport(timestamp_t old) {
    lamport_time = (old <= lamport_time) ? lamport_time : old;
    lamport_time++;
}


int request_cs(const void * self) {
    WorkInfo *wi = (WorkInfo*) self;
    Message cs_req;
    cs_req.s_header = create_message_header(0, CS_REQUEST, get_lamport_time());
    send_multicast(wi, &cs_req);

    Request req;
    req.request_id = wi->id_now;
    req.lamport_time = get_lamport_time();

//    QItem i;
//    i.processId = GetLocalProcessId();
//    i.time = get_lamport_time();
//    QueueInsert(&csQueue, i);

    wi->q[req.request_id] = req.lamport_time;

    // receive reply messages
    local_id replyCounter = 0;
    while (1) {
        local_id last_sender = receive_any(wi, &cs_req);
        switch (cs_req.s_header.s_type){
            case CS_REQUEST:
                // put the request into the queue
                wi->q[last_sender] = cs_req.s_header.s_local_time;
                printf("req cs proc %d\n", last_sender);

                // send reply message to sender
                cs_req.s_header.s_type = CS_REPLY;
                cs_req.s_header.s_payload_len = 0;
                send(wi, last_sender, &cs_req);
                break;
            case CS_REPLY:
                // check if the message's time is greater than this request
                if (cs_req.s_header.s_local_time > req.lamport_time) ++replyCounter;
                break;
            case CS_RELEASE:
                // remove the request from queue
                wi->q[last_sender] = INT16_MAX;
                printf("released %d\n", last_sender);
//                QueueRemoveByProcessId(&csQueue, last_sender);
                break;
            case DONE:
                done++;
                break;
        }

        for(int i = 1; i < N - 1; i++) {
            if (wi->q[i] < wi->q[i + 1]) highest_prio = i;
            else highest_prio = i + 1;
        }
        if(highest_prio == 0) {
            for(int i = N - 1; i > 0; i--) {
                if (wi->q[i] != INT16_MAX) highest_prio = i;
                break;
            }
        }
        // check if we can go into cs
        printf("lid %d wants to cs\n", wi->id_now);
        printf("replyCounter %d\n", replyCounter);
        printf("highest_prio = %d\n", highest_prio);
        for(int i = 0; i < N; i++){
            printf("q[%d]: %d\n", i, wi->q[i]);
        }
        if (replyCounter == (wi->id_now - 1) && highest_prio == req.request_id){
            break;
        }
    }

    return 0;
}

int release_cs(const void * self) {
    WorkInfo *wi = (WorkInfo*) self;

    wi->q[wi->id_now] = INT16_MAX;

    Message cs_rel;
    cs_rel.s_header.s_type =CS_RELEASE;
    cs_rel.s_header.s_payload_len = 0;
    send_multicast(wi, &cs_rel);

    return 0;
}

void dec_work(local_id lid) {
    work_info->id_now = lid;
    work_info->iterations = work_info->id_now * 5;
    close_red_pipes();
    log_event(0, lid, 0, work_info->iterations);
    char pld[MAX_PAYLOAD_LEN];
    int len = sprintf(pld, log_started_fmt,
                      get_lamport_time(), lid, getpid(), getppid(), work_info->iterations);
    Message msg;
    memcpy(msg.s_payload, pld, len);
    msg.s_header = create_message_header(len, STARTED, get_lamport_time());
    send_multicast(work_info, &msg);
    receive_multicast(work_info, msg.s_header.s_type);

    log_event(1, lid, 0, 0);
    for (int i = 1; i <= work_info->iterations; ++i){
        if (use_cs) request_cs(work_info);
        log_event(6, work_info->id_now, i, work_info->iterations);
        print(buffer);
        if (use_cs) release_cs(work_info);
    }

    work_with_state();
    close_self_pipes();
}

void work_with_state() {
    Message msg_state;
    int state_stop = 0;


    while (1) {
        int last_sender = receive_any(work_info, &msg_state);
        if (msg_state.s_header.s_type == DONE) {
            ++done;
            if (done == N - 2) {
                log_event(3, work_info->id_now, 0, 0);
                if (state_stop) {
                    return;
                }
            }
        }

        if(msg_state.s_header.s_type == CS_REQUEST) {
            Message cs_req;
            cs_req.s_header.s_payload_len = 0;
            cs_req.s_header.s_type =CS_REQUEST;
            send(work_info, last_sender, &cs_req);
        }

        if (msg_state.s_header.s_type == STOP) {
            char pld[MAX_PAYLOAD_LEN];
            int len = sprintf(pld, log_done_fmt, get_lamport_time(), work_info->id_now, work_info->iterations);
            Message stop;
            memcpy(&stop.s_payload, pld, len);
            stop.s_header = create_message_header(len, DONE, get_lamport_time());
            send_multicast(&(work_info->id_now), &stop);
            state_stop = 1;
        }
    }
}

int send_multicast(void *self, const Message *msg) {
    WorkInfo *wi = (WorkInfo *) self;
    inc_lamport(0);
    ((Message*)msg)->s_header.s_local_time = get_lamport_time();
    for (local_id i = 0; i < N; i++) {
        if (work_info->id_now != i) {
            send_message(wi, i, msg);
        }
    }
    return 0;
}

int send_message(void *self, local_id dst, const Message *msg) {
    WorkInfo *wi = (WorkInfo *) self;
    write(wi->pipes_arr[wi->id_now][dst]->fd_w, msg,
          msg->s_header.s_payload_len + sizeof(MessageHeader));
    return 0;
}

int send(void *self, local_id dst, const Message *msg) {
    WorkInfo *wi = (WorkInfo *) self;
    inc_lamport(0);
    ((Message*)msg)->s_header.s_local_time = get_lamport_time();
    return send_message(wi, dst, msg);
}

int receive_multicast(void *self, int16_t type) {
    WorkInfo *wi = (WorkInfo *) self;
    Message msg;
    for (local_id i = 1; i < N; i++)
        if (i != wi->id_now) {
            receive(wi, i, &msg);
        }
    return 0;
}

int receive_any(void *self, Message *msg) {
    WorkInfo *wi = (WorkInfo *) self;
    while (1) {
        for (local_id i = 0; i < N; i++)
            if (i != wi->id_now) {
                PipeFileDisc *pipe = wi->pipes_arr[wi->id_now][i];
                int bytes_count = read(pipe->fd_r, &(msg->s_header), sizeof(MessageHeader));
                if (bytes_count > 0) {
                    read(pipe->fd_r, &msg->s_payload, msg->s_header.s_payload_len);
                    inc_lamport(msg->s_header.s_local_time);
                    return i;
                }
            }
    }
}


int receive(void *self, local_id from, Message *msg) {
    WorkInfo *wi = (WorkInfo *) self;
    receive_message(wi, from, msg);
    inc_lamport(msg->s_header.s_local_time);
    return 0;
}

int receive_message(void *self, local_id from, Message *msg) {
    WorkInfo *wi = (WorkInfo *) self;
    local_id id = wi->id_now;
    PipeFileDisc *pipe = wi->pipes_arr[id][from];
    while (1) {
        if (read(pipe->fd_r, &(msg->s_header), sizeof(MessageHeader)) != -1) {
            read(pipe->fd_r, &(msg->s_payload), msg->s_header.s_payload_len);
            return 0;
        }
    }
}

void open_pipes() {
    int fds[2];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i != j) {
                pipe2(fds, O_NONBLOCK);
                work_info->pipes_arr[j][i]->fd_r = fds[0];
                log_pipe(0, 0, j, i, work_info->pipes_arr[j][i]->fd_r);

                work_info->pipes_arr[i][j]->fd_w = fds[1];
                log_pipe(0, 0, i, j, work_info->pipes_arr[i][j]->fd_w);
            }
        }
    }
}

void forc_procs() {
    pid_t pids[N];
    pids[0] = getpid();
    for (local_id i = 1; i < N; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            dec_work(i);
            exit(0);
        }
    }
}

void close_red_pipes() {
    PipeFileDisc *pipe;
    local_id id = work_info->id_now;
    for (local_id i = 0; i < N; i++) {
        if (i == id) continue;
        for (local_id j = 0; j < N; j++) {
            if (i != j) {
                pipe = work_info->pipes_arr[i][j];
                log_pipe(1, id, i, j, pipe->fd_w);
                close(pipe->fd_w);
                log_pipe(1, id, i, j, pipe->fd_r);

                close(pipe->fd_r);
            }
        }
    }
}

void close_self_pipes() {
    PipeFileDisc *pipe;
    local_id id = work_info->id_now;
    for (local_id i = 0; i < N; i++) {
        if (i != id) {
            pipe = work_info->pipes_arr[id][i];
            log_pipe(1, id, id, i, pipe->fd_w);
            close(pipe->fd_w);
            log_pipe(1, id, id, i, pipe->fd_r);

            close(pipe->fd_r);
        }
    }
}

MessageHeader create_message_header(uint16_t len, int16_t type, timestamp_t time) {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_payload_len = len;
    header.s_type = type;
    header.s_local_time = time;
    return header;
}

void log_event(int num, local_id lid, local_id to, int balance_or_iteration) {

    switch (num) {
        case 0:
            sprintf(buffer, log_started_fmt,
                    get_lamport_time(), lid, getpid(), getppid(), balance_or_iteration);
            break;
        case 1:
            sprintf(buffer, log_received_all_started_fmt,
                    get_lamport_time(), lid);
            break;
        case 2:
            sprintf(buffer, log_done_fmt,
                    get_lamport_time(), lid, balance_or_iteration);
            break;
        case 3:
            sprintf(buffer, log_received_all_done_fmt,
                    get_lamport_time(), lid);
            break;
        case 4:
            sprintf(buffer, log_transfer_out_fmt,
                    get_lamport_time(), lid, balance_or_iteration, to);
            break;
        case 5:
            sprintf(buffer, log_transfer_in_fmt,
                    get_lamport_time(), lid, balance_or_iteration, to);
            break;
        case 6:
            sprintf(buffer, log_loop_operation_fmt, lid, to, balance_or_iteration);
            break;
        default:
            break;
    }
    write(fd_events_log, buffer, strlen(buffer));
}

void log_pipe(int type, int fd_w, int from, int to, int desc) {
    char *buf = malloc(sizeof(char) * 100);
    if (type == 0) {
        sprintf(buf, pipe_opened_log, fd_w, from, to, desc);
    }
    if (type == 1) {
        sprintf(buf, pipe_closed_log, fd_w, from, to, desc);
    }
    write(fd_pipes_log, buf, strlen(buf));
}

void root_work() {
    work_info->id_now = PARENT_ID;
    close_red_pipes();
    receive_multicast(work_info, STARTED);
    Message *msg = malloc(sizeof(Message));
    msg->s_header = create_message_header(0, STOP, get_lamport_time());
    send_multicast(work_info, msg);
    free(msg);
    receive_multicast(work_info, DONE);
    while (wait(NULL) > 0) {}
    close_self_pipes();
}

int main(int argc, char *argv[]) {
    for(int i = 2; i < argc; i++) {
        if(argv[i] != "-p" & argv[i] != "--mutexl") {
            if(argv[i - 1] != "-p") {
                N = atoi(argv[i]) + 1;
            }
            else return 0;
        }
        if(argv[i] == "--mutexl") {
            use_cs = 1;
        }
        use_cs = 1;
    }
    fd_pipes_log = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    fd_events_log = open(events_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    work_info = (WorkInfo *) malloc(sizeof(WorkInfo));
    for (int i = 0; i < N; i++) {
        work_info->q[i] = INT16_MAX;
        for (int j = 0; j < N; j++) {
            if (i != j) {
                PipeFileDisc *pipe = (PipeFileDisc *) malloc(sizeof(PipeFileDisc));
                work_info->pipes_arr[i][j] = pipe;
            }
        }
    }
    open_pipes();
    forc_procs();
    root_work();
    close(fd_pipes_log);
    close(fd_events_log);
    free(work_info);
    return 0;
}

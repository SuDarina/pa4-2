#include "pa23.h"
#include "ipc.h"
#include "banking.h"


int fd_pipes_log;
int fd_events_log;

int N;
WorkInfo *work_info;
int *balance_arr;
BalanceHistory history;


timestamp_t get_lamport_time() {
    return lamport_time;
}

void inc_lamport(timestamp_t old) {
    lamport_time = (old <= lamport_time) ? lamport_time : old;
    lamport_time++;
}

void update_balance_history(){
    timestamp_t t = get_lamport_time();
    BalanceState* bs = malloc (sizeof (BalanceState));
    for (int i = history.s_history_len; i <= t; i++){
        bs = &(history.s_history[i]);
        bs->s_balance = history.s_history[i-1].s_balance;
        bs->s_time = i;
        bs->s_balance_pending_in = 0;
    }
    history.s_history_len = t + 1;
    bs->s_time = t;
    bs = &(history.s_history[t]);
    bs->s_balance = work_info->balance_now;
    bs->s_balance_pending_in = 0;
}


void dec_work(local_id lid) {
    work_info->id_now = lid;
    work_info->balance_now = balance_arr[lid - 1];
    close_red_pipes();
    log_event(0, lid, 0, work_info->balance_now);
    char pld[MAX_PAYLOAD_LEN];
    int len = sprintf(pld, log_started_fmt,
                      get_lamport_time(), lid, getpid(), getppid(), work_info->balance_now);
    Message msg;
    memcpy(msg.s_payload, pld, len);
    msg.s_header = create_message_header(len, STARTED, get_lamport_time());
    send_multicast(work_info, &msg);
    receive_multicast(work_info, msg.s_header.s_type);

    log_event(1, lid, 0, 0);

    work_with_state();
    close_self_pipes();
}

void work_with_state() {
    history.s_id = work_info->id_now;
    history.s_history_len = 1;
    history.s_history[0].s_balance = work_info->balance_now;
    update_balance_history();
    Message msg_state;
    int done = 0;
    int state_stop = 0;


    while (1) {
        receive_any(work_info, &msg_state);
        if (msg_state.s_header.s_type == DONE) {
            ++done;
            if (done == N - 2) {
                log_event(3, work_info->id_now, 0, 0);
                update_balance_history();
                if (state_stop) {
                    char pld[MAX_PAYLOAD_LEN];
                    memcpy(&pld, &history, sizeof(BalanceHistory));
                    Message msg_history;
                    memcpy(&msg_history.s_payload, pld, sizeof(BalanceHistory));
                    msg_history.s_header = create_message_header(sizeof(BalanceHistory), BALANCE_HISTORY,get_lamport_time());
                    send(work_info, PARENT_ID, &msg_history);
                    return;
                }
            }
        }

        if (msg_state.s_header.s_type == TRANSFER) {
            TransferOrder transfer_order;
            local_id id = work_info->id_now;
            memcpy(&transfer_order, msg_state.s_payload, sizeof(TransferOrder));
            if (transfer_order.s_dst == id) {
                work_info->balance_now += transfer_order.s_amount;
                Message transfer;
                update_balance_history();
                for (timestamp_t i = msg_state.s_header.s_local_time; i < get_lamport_time(); i++) {
                    history.s_history[i].s_balance_pending_in += transfer_order.s_amount;
                }
                transfer.s_header = create_message_header(0, ACK, get_lamport_time());
                send(work_info, PARENT_ID, &transfer);
                log_event(5, id, transfer_order.s_src, transfer_order.s_amount);
            } else if (transfer_order.s_src == id) {
                work_info->balance_now -= transfer_order.s_amount;
                msg_state.s_header.s_local_time = get_lamport_time();
                send(work_info, transfer_order.s_dst, &msg_state);
                log_event(4, id, transfer_order.s_dst, transfer_order.s_amount);
                update_balance_history();
            }
        }
        if (msg_state.s_header.s_type == STOP) {
            char pld[MAX_PAYLOAD_LEN];
            int len = sprintf(pld, log_done_fmt, get_lamport_time(), work_info->id_now, work_info->balance_now);
            Message stop;
            memcpy(&stop.s_payload, pld, len);
            stop.s_header = create_message_header(len, DONE, get_lamport_time());
            send_multicast(&(work_info->id_now), &stop);
            state_stop = 1;
        }
    }
}

void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
    WorkInfo *wi = (WorkInfo *) parent_data;
    TransferOrder to;
    to.s_src = src;
    to.s_dst = dst;
    to.s_amount = amount;
    char pld[MAX_PAYLOAD_LEN];
    Message transfer_out;
    memcpy(pld, &transfer_out, sizeof (TransferOrder));
    transfer_out.s_header = create_message_header(sizeof(TransferOrder), TRANSFER, get_lamport_time());

    memcpy(transfer_out.s_payload, &to, sizeof(TransferOrder));
    send(wi, src, &transfer_out);

    Message *transfer_in = malloc(sizeof(Message));
    receive(wi, dst, transfer_in);
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
                    return 0;
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

void form_history_message(AllHistory *ah) {
    Message question;
    Message answer;
    int max_len = 0;
    question.s_header = create_message_header(0, BALANCE_HISTORY, get_lamport_time());
    ah->s_history_len = N - 1;
    for (local_id i = 1; i < N; i++) {
        question.s_header.s_local_time = get_lamport_time();
        receive(work_info, i, &answer);
        memcpy(&ah->s_history[i - 1], answer.s_payload, sizeof(answer.s_payload));
        if (((BalanceHistory*)answer.s_payload)->s_history_len > max_len) {
            max_len = ((BalanceHistory*)answer.s_payload)->s_history_len;
        }
    }
    for (local_id i = 1; i < N; i++) {
        int h_len = ah->s_history[i - 1].s_history_len;
        if (h_len < max_len) {
            BalanceState bs = ah->s_history[i - 1].s_history[h_len - 2];
            for (int j = h_len; j < max_len; j++) {
                bs.s_time = j;
                ah->s_history[i - 1].s_history[j] = bs;
            }
            ah->s_history[i - 1].s_history_len = max_len;
        }
    }
}

void log_event(int num, local_id lid, local_id to, balance_t balance_now) {
    char buffer[MAX_MESSAGE_LEN];

    switch (num) {
        case 0:
            sprintf(buffer, log_started_fmt,
                    get_lamport_time(), lid, getpid(), getppid(), balance_now);
            break;
        case 1:
            sprintf(buffer, log_received_all_started_fmt,
                    get_lamport_time(), lid);
            break;
        case 2:
            sprintf(buffer, log_done_fmt,
                    get_lamport_time(), lid, balance_now);
            break;
        case 3:
            sprintf(buffer, log_received_all_done_fmt,
                    get_lamport_time(), lid);
            break;
        case 4:
            sprintf(buffer, log_transfer_out_fmt,
                    get_lamport_time(), lid, balance_now, to);
            break;
        case 5:
            sprintf(buffer, log_transfer_in_fmt,
                    get_lamport_time(), lid, balance_now, to);
            break;
        default:
            break;
    }
    write(fd_events_log, buffer, strlen(buffer));
}

void log_pipe(int type, int fd_w, int from, int to, int desc) {
    char *buffer = malloc(sizeof(char) * 100);
    if (type == 0) {
        sprintf(buffer, pipe_opened_log, fd_w, from, to, desc);
    }
    if (type == 1) {
        sprintf(buffer, pipe_closed_log, fd_w, from, to, desc);
    }
    write(fd_pipes_log, buffer, strlen(buffer));
}

void root_work() {
    work_info->id_now = PARENT_ID;
    close_red_pipes();
    receive_multicast(work_info, STARTED);
    bank_robbery(work_info, N - 1);
    Message *msg = malloc(sizeof(Message));
    msg->s_header = create_message_header(0, STOP, get_lamport_time());
    send_multicast(work_info, msg);
    receive_multicast(work_info, DONE);
    AllHistory all_history;
    form_history_message(&all_history);
    while (wait(NULL) > 0) {}
    close_self_pipes();
    print_history(&all_history);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-p") != 0) {
        return 0;
    }
    N = atoi(argv[2]) + 1;

    balance_arr = (int *) malloc(sizeof(balance_t) * (N - 1));
    for (int i = 0; i < N - 1; i++) {
        balance_arr[i] = atoi(argv[i + 3]);
    }
    fd_pipes_log = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    fd_events_log = open(events_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    work_info = (WorkInfo *) malloc(sizeof(WorkInfo));
    for (int i = 0; i < N; i++) {
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
    free(balance_arr);
    return 0;
}

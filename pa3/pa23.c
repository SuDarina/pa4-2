#include "pa23.h"
#include "ipc.h"
#include "banking.h"


int fd_pipes_log;
int fd_events_log;

int N;
WorkInfo *work_info;
int* balance_arr;


timestamp_t get_lamport_time() {
    return lamport_time;
}

void init_lamport_time() {
    lamport_time = 0;
}

void inc_lamport_time() {
    lamport_time++;
}

void set_lamport_time(timestamp_t local_time) {
    if (local_time > lamport_time) {
        lamport_time = local_time + 1;
    } else {
        lamport_time = lamport_time + 1;
    }
}

void dec_work(local_id lid) {
    work_info->id_now = lid;
    work_info->balance_now = balance_arr[lid - 1];
    init_lamport_time();
    close_red_pipes();
    log_event(0, lid, 0, work_info->balance_now);
    inc_lamport_time();
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
    BalanceHistory history;
    BalanceState state;
    Message msg_state;
    timestamp_t last_time = 0;
    int done = 0;
    int balance = work_info->balance_now;
    int state_stop = 0;
    history.s_id = work_info->id_now;
    state.s_balance = balance;
    state.s_time = 0;
    state.s_balance_pending_in = 0;
    history.s_history[0] = state;
    while (1) {
        receive_any(work_info, &msg_state);
        set_lamport_time(msg_state.s_header.s_local_time);
        if (msg_state.s_header.s_type == DONE) {
            ++done;
            set_lamport_time(msg_state.s_header.s_local_time);
            if (done == N - 2) {
                log_event(3, work_info->id_now, 0, 0);
                if (state_stop) {
                    int counter;
                    int time = get_lamport_time();
                    for (counter = history.s_history_len; counter <= time; counter++) {
                        history.s_history[counter].s_balance = history.s_history[history.s_history_len - 1].s_balance;
                        history.s_history[counter].s_balance_pending_in = history.s_history[history.s_history_len -
                                                                                            1].s_balance_pending_in;
                        history.s_history[counter].s_time = counter;
                    }
                    history.s_history_len = last_time + 1;
                    inc_lamport_time();
                    char pld[MAX_PAYLOAD_LEN];
                    memcpy(&pld, &history, sizeof(BalanceHistory));
                    Message msg_history;
                    memcpy(&msg_history.s_payload, pld, sizeof(BalanceHistory));
                    msg_history.s_header = create_message_header(sizeof(BalanceHistory), BALANCE_HISTORY,
                                                                 get_lamport_time());

                    send(work_info, PARENT_ID, &msg_history);
                    return;
                }
            }
        }

        if (msg_state.s_header.s_type == TRANSFER) {
            inc_lamport_time();
            timestamp_t tmp_last_time = last_time;
            TransferOrder transfer_order;
            local_id id = work_info->id_now;
            timestamp_t new_time = get_lamport_time();
            memcpy(&transfer_order, msg_state.s_payload, sizeof(TransferOrder));
            int pend = 0;

            if (transfer_order.s_dst == id) {
                balance += transfer_order.s_amount;
                Message transfer;
                int old_balance = history.s_history[history.s_history_len - 1].s_balance_pending_in;
                pend = old_balance - transfer_order.s_amount;
                transfer.s_header = create_message_header(0, ACK, get_lamport_time());
                send(work_info, PARENT_ID, &transfer);
                log_event(5, id, transfer_order.s_src, transfer_order.s_amount);
            } else if (transfer_order.s_src == id) {
                balance -= transfer_order.s_amount;
                int old_balance = history.s_history[history.s_history_len - 1].s_balance_pending_in;
                pend = old_balance + transfer_order.s_amount;
                msg_state.s_header.s_local_time = get_lamport_time();
                send(work_info, transfer_order.s_dst, &msg_state);
                log_event(4, id, transfer_order.s_dst, transfer_order.s_amount);
            }

            state.s_balance = balance;
            state.s_time = new_time;
            state.s_balance_pending_in = pend;
            history.s_history[new_time] = state;
            BalanceState tmp_state = history.s_history[tmp_last_time];
            for (tmp_last_time++; tmp_last_time < new_time; tmp_last_time++) {
                tmp_state.s_time = tmp_last_time;
                history.s_history[tmp_last_time] = tmp_state;
            }
            last_time = tmp_last_time;

        }
        if (msg_state.s_header.s_type == STOP) {
            char pld[MAX_PAYLOAD_LEN];
            int len = sprintf(pld, log_done_fmt, get_lamport_time(), work_info->id_now, balance);

            inc_lamport_time();
            Message stop;

            memcpy(&stop.s_payload, pld, len);
            stop.s_header = create_message_header(len, DONE, get_lamport_time());
            send_multicast(&(work_info->id_now), &stop);
            state_stop = 1;
        }
    }
}

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
    WorkInfo *wi = (WorkInfo*)parent_data;
    TransferOrder to;
    to.s_src = src;
    to.s_dst = dst;
    to.s_amount = amount;
    Message transfer_out;
    transfer_out.s_header.s_magic = MESSAGE_MAGIC;
    transfer_out.s_header.s_type = TRANSFER;
    transfer_out.s_header.s_local_time = get_lamport_time();
    transfer_out.s_header.s_payload_len = sizeof(TransferOrder);

    memcpy(transfer_out.s_payload, &to, sizeof(TransferOrder));
    send(wi, src, &transfer_out);

    Message *transfer_in = malloc(sizeof (Message));
    receive(wi, dst, transfer_in);
}

int send_multicast(void * self, const Message * msg){
    WorkInfo *wi = (WorkInfo*)self;
    for (local_id i = 0; i < N; i++){
        if(work_info->id_now != i){
            send(wi, i, msg);
        }
    }
    return 0;
}

int send(void * self, local_id dst, const Message * msg){
    WorkInfo *wi = (WorkInfo*)self;
    write(wi->s_pipes[wi->id_now][dst]->fd_w, msg,
          msg->s_header.s_payload_len + sizeof(MessageHeader));
    return 0;
}

int receive_multicast(void * self, int16_t type) {
    WorkInfo* wi = (WorkInfo*)self;
    for (local_id i = 1; i < N; i++)
        if (i != wi->id_now) {
            Message msg;
            if (receive(wi, i, &msg) != 0 || msg.s_header.s_type != type) {
                return -1;
            }
            if (msg.s_header.s_type != DONE)
                set_lamport_time(msg.s_header.s_local_time);
        }

    return 0;
}

int receive_any(void * self, Message * msg){
    WorkInfo* wi = (WorkInfo*)self;
    while(1){
        for (local_id i = 0; i < N; i++)
            if (i != wi->id_now){
                PipeFileDisc *pipe = wi->s_pipes[wi->id_now][i];
                int bytes_count = read(pipe->fd_r, &(msg->s_header), sizeof(MessageHeader));
                if(bytes_count<=0)
                    continue;
                bytes_count = read(pipe->fd_r, &msg->s_payload, msg->s_header.s_payload_len);
                return (int) i;
            }
    }
}


int receive(void * self, local_id from, Message * msg){
    WorkInfo *wi = (WorkInfo*)self;
    local_id id = wi->id_now;
    PipeFileDisc *pipe = wi->s_pipes[id][from];

    while (1) {
        int bytes_count = read(pipe->fd_r, &(msg->s_header), sizeof(MessageHeader));
        if(bytes_count==-1)
            continue;

        bytes_count = read(pipe->fd_r, &(msg->s_payload), msg->s_header.s_payload_len);
        return 0;
    }
}

void open_pipes(){
    int fds[2];
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            if (i != j) {
                pipe2(fds, O_NONBLOCK);
                work_info->s_pipes[j][i]->fd_r = fds[0];
                log_pipe(0, 0, j, i, work_info->s_pipes[j][i]->fd_r);

                work_info->s_pipes[i][j]->fd_w = fds[1];
                log_pipe(0, 0, i, j, work_info->s_pipes[i][j]->fd_w);
            }
        }
    }
}

void forc_procs(){
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

void close_red_pipes(){
    PipeFileDisc* pipe;
    local_id id = work_info->id_now;
    for (local_id i = 0; i < N; i++){
        if (i == id) continue;
        for (local_id j = 0; j < N; j++){
            if (i != j){
                pipe = work_info->s_pipes[i][j];
                log_pipe(1,id, i, j, pipe->fd_w);
                close(pipe->fd_w);
                log_pipe(1, id, i, j, pipe->fd_r);

                close(pipe->fd_r);
            }
        }
    }
}

void close_self_pipes(){
    PipeFileDisc* pipe;
    local_id id = work_info->id_now;
    for (local_id i = 0; i < N; i++){
        if (i != id){
            pipe = work_info->s_pipes[id][i];
            log_pipe(1,id, id, i, pipe->fd_w);
            close(pipe->fd_w);
            log_pipe(1, id, id, i, pipe->fd_r);

            close(pipe->fd_r);
        }
    }
}

Message create_message(uint16_t magic, char* payload, uint16_t len, int16_t type, timestamp_t time){
    Message msg;
    if(payload!=NULL)
        memcpy(&msg.s_payload, payload, len);
    msg.s_header = create_message_header(len, type, time);
    return msg;
}

MessageHeader create_message_header(uint16_t len, int16_t type, timestamp_t time) {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_payload_len = len;
    header.s_type = type;
    header.s_local_time = time;
    return header;
}

void get_history_messages(AllHistory* ah) {
    Message question;
    Message answer;
    int max_len = 0;
    question.s_header.s_magic = MESSAGE_MAGIC;
    question.s_header.s_type = BALANCE_HISTORY;
    question.s_header.s_payload_len = 0;
    ah->s_history_len = N - 1;
    for (local_id i = 1; i < N; i++) {
        question.s_header.s_local_time = get_lamport_time();
        receive(work_info, i, &answer);
        BalanceHistory* history = (BalanceHistory*) answer.s_payload;
        memcpy(&ah->s_history[i - 1], history, sizeof(BalanceHistory));
        if (history->s_history_len > max_len) {
            max_len = history->s_history_len;
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

    switch(num) {
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

void log_pipe(int type,  int fd_w, int from, int to, int desc) {
    char* buffer = malloc(sizeof(char) * 100);
    if(type ==  0) {
        sprintf(buffer, pipe_opened_log, fd_w, from, to, desc);
    }
    if(type == 1) {
        sprintf(buffer, pipe_closed_log, fd_w, from, to, desc);
    }
    write(fd_pipes_log, buffer, strlen(buffer));
}

void root_work() {
    local_id lid = PARENT_ID;
    work_info->id_now = lid;
    init_lamport_time();
    close_red_pipes();
    receive_multicast(work_info, STARTED);
    bank_robbery(work_info, N - 1);
    Message msg = create_message(MESSAGE_MAGIC, NULL, 0, STOP, get_lamport_time());
    send_multicast(work_info, &msg);
    receive_multicast(work_info, DONE);
    AllHistory all_history;
    get_history_messages(&all_history);

    while(wait(NULL) > 0){}
    close_self_pipes();
    print_history(&all_history);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-p" ) != 0) {
        return 0;
    }
    N = atoi(argv[2]) + 1;

    balance_arr = (int*)malloc(sizeof(balance_t) * (N - 1));
    for (int i = 0; i < N - 1; i++) {
        balance_arr[i] = atoi(argv[i + 3]);
    }
    fd_pipes_log = open(pipes_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    fd_events_log = open(events_log, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0777);
    work_info = (WorkInfo *)malloc(sizeof(WorkInfo));
    for(int i = 0; i < N; i++) {
        for (int j = 0 ; j < N; j++){
            if(i != j){
                PipeFileDisc * pipe = (PipeFileDisc*)malloc(sizeof(PipeFileDisc));
                work_info->s_pipes[i][j] = pipe;
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

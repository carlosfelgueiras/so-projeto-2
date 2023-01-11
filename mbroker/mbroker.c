#include "mbroker.h"
#include "logging.h"
#include "operations.h"
#include "protocol.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int register_pipe_fd;
p_box_info *box_info;
pthread_mutex_t *box_info_mutex;
box_usage_state_t *box_usage;
pthread_mutex_t box_usage_mutex = PTHREAD_MUTEX_INITIALIZER;
long unsigned int box_max_number;

int box_alloc(void) {
    pthread_mutex_lock(&box_usage_mutex);

    for (int i = 0; i < box_max_number; i++) {
        if (box_usage[i] == FREE) {
            box_usage[i] = TAKEN;

            pthread_mutex_unlock(&box_usage_mutex);

            return i;
        }
    }
    pthread_mutex_unlock(&box_usage_mutex);
    return -1;
}

void box_delete(int i) {
    pthread_mutex_lock(&box_usage_mutex);
    box_usage[i] = FREE;
    pthread_mutex_unlock(&box_usage_mutex);
}

int box_info_lookup(char *box_name) {
    for (int i = 0; i < box_max_number; i++) {
        if (box_usage[i] == TAKEN && !strcmp(box_name, box_info[i].box_name))
            return i;
    }
    return -1;
}

void send_message_to_client(char *message) {
    char message_code[P_SUB_MESSAGE_SIZE] = {0};
    message_code[0] = 10;
    memcpy(message_code + 1, message, 1024);
    // Send to message pipe
}

void publisher(char *pipe_name, char *box_name) {
    (void)pipe_name;
    (void)box_name;

    // TODO: implement
}

void subscriber(char *pipe_name, char *box_name) {
    (void)pipe_name;
    (void)box_name;

    // TODO: implement
}

void manager_box_creation() {
    char pipe[P_PIPE_NAME_SIZE];
    char box_name[P_BOX_NAME_SIZE];
    if (read(register_pipe_fd, pipe, P_PIPE_NAME_SIZE) != P_PIPE_NAME_SIZE) {
        exit(-1);
    }

    if (read(register_pipe_fd, box_name, P_BOX_NAME_SIZE) != P_BOX_NAME_SIZE) {
        exit(-1);
    }

    char tmp_pipe_name[P_PIPE_NAME_SIZE + 5] = {0};
    sprintf(tmp_pipe_name, "/tmp/%s", pipe);

    int pipe_fd = open(tmp_pipe_name, O_WRONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    int fd = tfs_open(box_name, TFS_O_CREAT);
    tfs_close(fd);

    p_response response_struct;

    if (write(pipe_fd, &response_struct, P_BOX_CREATION_RESPONSE_SIZE) !=
        P_BOX_CREATION_RESPONSE_SIZE) {
        exit(-1);
    }

    if (close(pipe_fd) < 0) {
        exit(-1);
    }

    printf("code: 3 %s %s\n", pipe, box_name);
}

void manager_box_removal() {
    char pipe[P_PIPE_NAME_SIZE];
    char box_name[P_BOX_NAME_SIZE];
    if (read(register_pipe_fd, pipe, P_PIPE_NAME_SIZE) != P_PIPE_NAME_SIZE) {
        exit(-1);
    }

    if (read(register_pipe_fd, box_name, P_BOX_NAME_SIZE) != P_BOX_NAME_SIZE) {
        exit(-1);
    }

    char tmp_pipe_name[P_PIPE_NAME_SIZE + 5] = {0};
    sprintf(tmp_pipe_name, "/tmp/%s", pipe);

    int pipe_fd = open(tmp_pipe_name, O_WRONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    p_response response_struct;
    response_struct.protocol_code = P_BOX_REMOVAL_RESPONSE_CODE;
    response_struct.return_code = -1;
    strcpy(response_struct.error_message, "ola :)");

    if (write(pipe_fd, &response_struct, P_BOX_CREATION_RESPONSE_SIZE) !=
        P_BOX_CREATION_RESPONSE_SIZE) {
        exit(-1);
    }

    if (close(pipe_fd) < 0) {
        exit(-1);
    }

    printf("code: 5 %s %s\n", pipe, box_name);
}

void manager_box_listing() {
    char pipe[P_PIPE_NAME_SIZE];

    if (read(register_pipe_fd, pipe, P_PIPE_NAME_SIZE) != P_PIPE_NAME_SIZE) {
        exit(-1);
    }

    printf("code: 7 %s\n", pipe);
}

int main(int argc, char **argv) {
    char register_pipe[P_PIPE_NAME_SIZE + 5];
    int max_sessions = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
        exit(-1);
    }

    if (strlen(argv[1]) > P_PIPE_NAME_SIZE - 1) {
        fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
        exit(-1);
    }

    sprintf(register_pipe, "/tmp/%s", argv[1]);
    sscanf(argv[2], "%d", &max_sessions);

    if (max_sessions <= 0) {
        fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
        exit(-1);
    }

    tfs_params params = tfs_default_params();
    params.max_open_files_count = (size_t)max_sessions;

    if (tfs_init(&params) == -1) {
        exit(-1);
    }

    box_max_number = params.max_inode_count;

    box_info = (p_box_info *)malloc(sizeof(p_box_info) * box_max_number);
    if (box_info == NULL) {
        exit(-1);
    }
    box_info_mutex =
        (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * box_max_number);
    if (box_info_mutex == NULL) {
        exit(-1);
    }
    box_usage =
        (box_usage_state_t *)malloc(sizeof(box_usage_state_t) * box_max_number);
    if (box_usage == NULL) {
        exit(-1);
    }

    for (int i = 0; i < params.max_inode_count; i++) {
        if (pthread_mutex_init(&box_info_mutex[i], NULL) != 0) {
            exit(-1);
        }
        box_usage[i] = FREE;
    }

    if (unlink(register_pipe) != 0 &&
        errno != ENOENT) { // To prevent the case where the pipe already exists
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", register_pipe,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Create pipe
    if (mkfifo(register_pipe, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    register_pipe_fd = open(register_pipe, O_RDONLY);
    if (register_pipe_fd < 0) {
        exit(-1);
    }

    int write_fd = open(register_pipe, O_WRONLY);
    if (write_fd < 0) {
        exit(-1);
    }

    while (1) {
        char code;
        if (read(register_pipe_fd, &code, 1) != 1) {
            code = 0;
        }
        if (code == P_PUB_REGISTER_CODE) {
            char pub_pipe_name[P_PIPE_NAME_SIZE];
            char pub_box_name[P_BOX_NAME_SIZE];

            if (read(register_pipe_fd, pub_pipe_name, P_PIPE_NAME_SIZE) !=
                P_PIPE_NAME_SIZE) {
                exit(-1);
            }

            if (read(register_pipe_fd, pub_box_name, P_BOX_NAME_SIZE) !=
                P_BOX_NAME_SIZE) {
                exit(-1);
            }

            publisher(pub_pipe_name, pub_box_name);
        } else if (code == P_SUB_REGISTER_CODE) {
            char sub_pipe_name[P_PIPE_NAME_SIZE];
            char sub_box_name[P_BOX_NAME_SIZE];

            if (read(register_pipe_fd, sub_pipe_name, P_PIPE_NAME_SIZE) !=
                P_PIPE_NAME_SIZE) {
                exit(-1);
            }

            if (read(register_pipe_fd, sub_box_name, P_BOX_NAME_SIZE) !=
                P_BOX_NAME_SIZE) {
                exit(-1);
            }

            subscriber(sub_pipe_name, sub_box_name);
        } else if (code == P_BOX_CREATION_CODE) {

        } else if (code == P_BOX_REMOVAL_CODE) {
        } else if (code == P_BOX_LISTING_CODE) {
        } else {
            if (close(register_pipe_fd) < 0) {
                exit(-1);
            }
            if (close(write_fd) < 0) {
                exit(-1);
            }
        }

        switch (code) {
        case P_PUB_REGISTER_CODE:
            break;
        case P_SUB_REGISTER_CODE:
            break;
        case P_BOX_CREATION_CODE:
            manager_box_creation();
            break;
        case P_BOX_REMOVAL_CODE:
            manager_box_removal();
            break;
        case P_BOX_LISTING_CODE:
            manager_box_listing();
            break;
        case 0:
            break;
        default:
            if (close(register_pipe_fd) < 0) {
                exit(-1);
            }
            if (close(write_fd) < 0) {
                exit(-1);
            }
            break;
        }
    }
}

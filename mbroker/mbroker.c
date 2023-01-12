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

int box_alloc() {
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
    char box_name_slash[P_BOX_NAME_SIZE + 1];
    sprintf(box_name_slash, "/%s", box_name);
    for (int i = 0; i < box_max_number; i++) {
        if (box_usage[i] == TAKEN &&
            !strcmp(box_name_slash, box_info[i].box_name))
            return i;
    }
    return -1;
}

void send_response_client_manager(int fd, char *message, u_int8_t choice) {
    p_response response;
    strcpy(response.error_message, message);

    if (strlen(message) == 0) {
        response.return_code = 0;
    } else {
        response.return_code = -1;
    }

    response.protocol_code = choice;

    if (write(fd, &response, sizeof(response)) != sizeof(response)) {
        exit(-1);
    }

    if (close(fd) < 0) {
        exit(-1);
    }
}

void send_message_to_client(char *message) {
    char message_code[P_SUB_MESSAGE_SIZE] = {0};
    message_code[0] = 10;
    memcpy(message_code + 1, message, 1024);
    // Send to message pipe
}

void publisher(char *pipe_name, char *box_name) {
    char tmp_pipe_name[P_PIPE_NAME_SIZE + 5] = {0};
    sprintf(tmp_pipe_name, "/tmp/%s", pipe_name);

    int pipe_fd = open(tmp_pipe_name, O_RDONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    int box_id = box_info_lookup(box_name);

    if (box_id < 0 || box_info[box_id].n_publishers >= 1) {
        if (close(pipe_fd) < 0) {
            exit(-1);
        }

        return;
    }

    int box_fd = tfs_open(box_info[box_id].box_name, O_APPEND);

    if (box_fd < 0) {
        exit(-1);
    }

    char buffer[P_PUB_MESSAGE_SIZE];
    while (1) {
        ssize_t code = read(pipe_fd, &buffer, P_PUB_MESSAGE_SIZE);
        if (code != P_PUB_MESSAGE_SIZE) {
            if (code == 0) {
                if (close(pipe_fd) < 0) {
                    exit(-1);
                }

                return;
            }
            
            exit(-1);
        }

        if (buffer[0] != P_PUB_MESSAGE_CODE) {
            exit(-1);
        }

        size_t size = strlen(buffer + 1) + 1; // To include a /0d

        size_t bytes_writen = (size_t) tfs_write(box_fd, buffer+1, size);

        pthread_mutex_lock(&box_info_mutex[box_id]);
        box_info[box_id].box_size += bytes_writen;
        pthread_mutex_unlock(&box_info_mutex[box_id]);

        // DAR SIGNAL
    }
}

void subscriber(char *pipe_name, char *box_name) {
    (void)pipe_name;
    (void)box_name;

    // TODO: implement
}

void manager_box_creation(char *pipe_name, char *box_name) {
    char box_name_slash[P_BOX_NAME_SIZE + 1];
    sprintf(box_name_slash, "/%s", box_name);

    char tmp_pipe_name[P_PIPE_NAME_SIZE + 5] = {0};
    sprintf(tmp_pipe_name, "/tmp/%s", pipe_name);

    int pipe_fd = open(tmp_pipe_name, O_WRONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    if (box_info_lookup(box_name) != -1) {
        send_response_client_manager(pipe_fd, "Box already exits",
                                     P_BOX_CREATION_RESPONSE_CODE);
        return;
    }

    int box_id = box_alloc();

    if (box_id == -1) {
        send_response_client_manager(pipe_fd,
                                     "No more space to create new boxes",
                                     P_BOX_CREATION_RESPONSE_CODE);
        return;
    }

    int fd = tfs_open(box_name_slash, TFS_O_CREAT);
    if (fd == -1) {
        send_response_client_manager(pipe_fd,
                                     "No more space to create new boxes",
                                     P_BOX_CREATION_RESPONSE_CODE);
        return;
    }

    if (tfs_close(fd) == -1) {
        exit(-1);
    }

    pthread_mutex_lock(&box_info_mutex[box_id]);
    strcpy(box_info[box_id].box_name, box_name_slash);
    box_info[box_id].box_size = 0;
    box_info[box_id].n_publishers = 0;
    box_info[box_id].n_subscribers = 0;
    pthread_mutex_unlock(&box_info_mutex[box_id]);

    send_response_client_manager(pipe_fd, "", P_BOX_CREATION_RESPONSE_CODE);
}

void manager_box_removal(char *pipe_name, char *box_name) {
    char tmp_pipe_name[P_PIPE_NAME_SIZE + 5];
    sprintf(tmp_pipe_name, "/tmp/%s", pipe_name);

    int pipe_fd = open(tmp_pipe_name, O_WRONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    int box_id = box_info_lookup(box_name);
    if (box_id == -1) {
        send_response_client_manager(pipe_fd, "Such box does not exist",
                                     P_BOX_REMOVAL_RESPONSE_CODE);
        return;
    }

    if (tfs_unlink(box_info[box_id].box_name) == -1) {
        exit(-1);
    }

    box_delete(box_id);
    /*TODO??: se subscritores estiverem bloqueados e fecharmos a caixa, e
     * preciso dar broadcast aqui ou deixamos?*/
    send_response_client_manager(pipe_fd, "", P_BOX_REMOVAL_RESPONSE_CODE);
}

void manager_box_listing(char *pipe_name) {
    char tmp_pipe_name[P_PIPE_NAME_SIZE + 5];
    sprintf(tmp_pipe_name, "/tmp/%s", pipe_name);

    int pipe_fd = open(tmp_pipe_name, O_WRONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    int last = -1;
    for (int i = 0; i < box_max_number; i++) {
        if (box_usage[i] == TAKEN) {
            last = i;
        }
    }

    if (last == -1) {
        p_box_response response;

        response.protocol_code = P_BOX_LISTING_RESPONSE_CODE;
        response.last = 1;
        memset(response.box_name, 0, P_BOX_NAME_SIZE);
        response.box_size = 0;
        response.n_publishers = 0;
        response.n_subscribers = 0;

        if (write(pipe_fd, &response, sizeof(p_box_response)) !=
            sizeof(p_box_response)) {
            exit(-1);
        }

        if (close(pipe_fd) < 0) {
            exit(-1);
        }

        return;
    }

    for (int i = 0; i < box_max_number; i++) {
        // Lock???
        uint8_t last_info = 0;

        if (i == last) {
            last_info = 1;
        }

        if (box_usage[i] == TAKEN) {
            p_box_response response =
                p_build_box_listing_response(last_info, box_info[i]);
            if (write(pipe_fd, &response, sizeof(p_box_response)) !=
                sizeof(p_box_response)) {
                exit(-1);
            }
        }

        if (i == last) {
            break;
        }
    }

    if (close(pipe_fd) < 0) {
        exit(-1);
    }
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

    if (tfs_init(NULL) == -1) {
        exit(-1);
    }

    tfs_params params = tfs_default_params();
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

    for (int i = 0; i < box_max_number; i++) {
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
            char pipe_name[P_PIPE_NAME_SIZE];
            char box_name[P_BOX_NAME_SIZE];
            if (read(register_pipe_fd, pipe_name, P_PIPE_NAME_SIZE) !=
                P_PIPE_NAME_SIZE) {
                exit(-1);
            }

            if (read(register_pipe_fd, box_name, P_BOX_NAME_SIZE) !=
                P_BOX_NAME_SIZE) {
                exit(-1);
            }

            manager_box_creation(pipe_name, box_name);
        } else if (code == P_BOX_REMOVAL_CODE) {
            char pipe_name[P_PIPE_NAME_SIZE];
            char box_name[P_BOX_NAME_SIZE];

            if (read(register_pipe_fd, pipe_name, P_PIPE_NAME_SIZE) !=
                P_PIPE_NAME_SIZE) {
                exit(-1);
            }

            if (read(register_pipe_fd, box_name, P_BOX_NAME_SIZE) !=
                P_BOX_NAME_SIZE) {
                exit(-1);
            }

            manager_box_removal(pipe_name, box_name);
        } else if (code == P_BOX_LISTING_CODE) {
            char pipe_name[P_PIPE_NAME_SIZE];

            if (read(register_pipe_fd, pipe_name, P_PIPE_NAME_SIZE) !=
                P_PIPE_NAME_SIZE) {
                exit(-1);
            }

            manager_box_listing(pipe_name);
        } else {
            if (close(register_pipe_fd) < 0) {
                exit(-1);
            }
            if (close(write_fd) < 0) {
                exit(-1);
            }
        }
    }
}

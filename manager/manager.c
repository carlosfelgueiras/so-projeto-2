#include "logging.h"
#include "protocol.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char tmp_pipe_name[P_PIPE_NAME_SIZE + 5]; // Full name of the pipe

int compare_box(const void *a, const void *b) {
    return strcmp(((p_box_info *)a)->box_name, ((p_box_info *)b)->box_name);
}

int open_register_pipe(char *register_pipename) {
    char register_pn[P_PIPE_NAME_SIZE + 5];
    sprintf(register_pn, "/tmp/%s", register_pipename);

    int register_pipe_fd = open(register_pn, O_WRONLY);
    if (register_pipe_fd < 0) {
        exit(-1);
    }
    return register_pipe_fd;
}

void close_register_pipe(int register_pipe_fd) {
    if (close(register_pipe_fd) < 0) { // Closing the register pipe
        exit(-1);
    }
}

int open_pipe() {
    int pipe_fd = open(tmp_pipe_name, O_RDONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    return pipe_fd;
}

void close_pipe(int fd) {
    if (close(fd) < 0) {
        exit(-1);
    }
}

void request_box_creation(char *register_pipe_name, char *pipe_name,
                          char *box_name) {
    char register_code[P_BOX_CREATION_SIZE];

    p_build_box_creation(register_code, pipe_name, box_name);

    int register_pipe_fd = open_register_pipe(register_pipe_name);

    if (write(register_pipe_fd, register_code, P_BOX_CREATION_SIZE) !=
        P_BOX_CREATION_SIZE) {
        exit(-1);
    }

    close_register_pipe(register_pipe_fd);

    int pipe_fd = open_pipe();

    p_response response;

    if (read(pipe_fd, &response, sizeof(p_response)) != sizeof(p_response)) {
        exit(-1);
    }

    if (response.protocol_code != P_BOX_CREATION_RESPONSE_CODE) {
        exit(-1);
    }

    if (response.return_code == 0) {
        fprintf(stdout, "OK\n");
    } else if (response.return_code == -1) {
        fprintf(stdout, "ERROR %s\n", response.error_message);
    } else {
        exit(-1);
    }

    close_pipe(pipe_fd);
}

void request_box_removal(char *register_pipe_name, char *pipe_name,
                         char *box_name) {
    char register_code[P_BOX_REMOVAL_SIZE];

    p_build_box_removal(register_code, pipe_name, box_name);

    int register_pipe_fd = open_register_pipe(register_pipe_name);

    if (write(register_pipe_fd, register_code, P_BOX_REMOVAL_SIZE) !=
        P_BOX_REMOVAL_SIZE) {
        exit(-1);
    }

    close_register_pipe(register_pipe_fd);

    int pipe_fd = open_pipe();

    p_response response;

    if (read(pipe_fd, &response, P_BOX_REMOVAL_RESPONSE_SIZE) !=
        P_BOX_REMOVAL_RESPONSE_SIZE) {
        exit(-1);
    }

    if (response.protocol_code != P_BOX_REMOVAL_RESPONSE_CODE) {
        exit(-1);
    }

    if (response.return_code == 0) {
        fprintf(stdout, "OK\n");
    } else if (response.return_code == -1) {
        fprintf(stdout, "ERROR %s\n", response.error_message);
    } else {
        exit(-1);
    }

    close_pipe(pipe_fd);
}

void request_box_list(char *pipe_name) {
    char register_code[P_BOX_LISTING_SIZE];

    p_build_box_listing(register_code, pipe_name);

    int register_pipe_fd = open_register_pipe(pipe_name);

    if (write(register_pipe_fd, register_code, P_BOX_LISTING_SIZE) !=
        P_BOX_LISTING_SIZE) {
        exit(-1);
    }

    close_register_pipe(register_pipe_fd);

    int pipe_fd = open_pipe();

    p_box_response *array = (p_box_response *)malloc(sizeof(p_box_response));
    unsigned int size = 0;
    unsigned int cap = 1;

    p_box_response response;
    while (1) {
        if (read(pipe_fd, &response, sizeof(p_box_response)) !=
            sizeof(p_box_response)) {
            exit(-1);
        }

        if (response.protocol_code != P_BOX_LISTING_RESPONSE_CODE) {
            exit(-1);
        }

        if (size >= cap) {
            cap *= 2;
            array =
                (p_box_response *)realloc(array, cap * sizeof(p_box_response));
        }

        array[size++] = response;

        if (response.last == 1)
            break;
    }

    close_pipe(pipe_fd);

    if (size == 1 && strlen(array[0].box_name) == 0) {
        fprintf(stdout, "NO BOXES FOUND\n");
        free(array);
        return;
    }

    qsort(array, (size_t)size, sizeof(p_box_response), compare_box);

    for (int i = 0; i < size; i++) {
        fprintf(stdout, "%s %zu %zu %zu\n", array[i].box_name,
                (size_t)array[i].box_size, (size_t)array[i].n_publishers,
                (size_t)array[i].n_subscribers);
    }

    free(array);
}

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    char pipe_name[P_PIPE_NAME_SIZE] = {0};
    pid_t pid;

    if (argc < 4 || argc > 5) {
        print_usage();
        exit(-1);
    }

    if ((strlen(argv[1]) > P_PIPE_NAME_SIZE - 1) ||
        (strlen(argv[2]) > P_PIPE_NAME_SIZE - 6)) {
        print_usage();
        exit(-1);
    }

    pid = getpid(); // maximum legnth of pid is 5 digits

    sprintf(pipe_name, "%s%05d", argv[2],
            pid); // Associating the pid to the pipe_name given

    sprintf(tmp_pipe_name, "/tmp/%s",
            pipe_name); // To create the pipe in tmp directory

    if (unlink(tmp_pipe_name) != 0 &&
        errno != ENOENT) { // To prevent the case where the pipe already exists
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", tmp_pipe_name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Create pipe
    if (mkfifo(tmp_pipe_name, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    switch (argc) {
    case 4:
        if (!strcmp(argv[3], "list")) {
            request_box_list(argv[1]);
        } else {
            print_usage();
            exit(-1);
        }
        break;
    case 5:
        if (strlen(argv[4]) > P_BOX_NAME_SIZE - 1) {
            print_usage();
            exit(-1);
        }

        if (!strcmp(argv[3], "create")) {
            request_box_creation(argv[1], pipe_name, argv[4]);
        } else if (!strcmp(argv[3], "remove")) {
            request_box_removal(argv[1], pipe_name, argv[4]);
        } else {
            print_usage();
            exit(-1);
        }
        break;
    default:
        print_usage();
        exit(-1);
    }

    return 0;
}

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

char tmp_pipe_name[P_PIPE_NAME_SIZE + 6];   //Full name of the pipe

int open_register_pipe(char *register_pipename) {

    char register_pn[P_PIPE_NAME_SIZE + 6] = {0};
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

void request_box_creation(char *pipe_name, char *box_name) {
    char register_code[P_BOX_CREATION_SIZE] = {0};
    register_code[0] = P_BOX_CREATION_CODE;
    memcpy(register_code + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(register_code + P_PIPE_NAME_SIZE + 1, box_name, P_BOX_NAME_SIZE);
    int register_pipe_fd=open_register_pipe(pipe_name);
    // Send to register pipe
    close_register_pipe(register_pipe_fd);
}

void request_box_removal(char *pipe_name, char *box_name) {
    char register_code[P_BOX_REMOVAL_SIZE] = {0};
    register_code[0] = P_BOX_REMOVAL_CODE;
    memcpy(register_code + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(register_code + P_PIPE_NAME_SIZE + 1, box_name, P_BOX_NAME_SIZE);
    int register_pipe_fd=open_register_pipe(pipe_name);
    // Send to register pipe
    close_register_pipe(register_pipe_fd);
}

void request_box_list(char *pipe_name) {
    char register_code[P_BOX_LISTING_SIZE] = {0};
    register_code[0] = P_BOX_LISTING_CODE;
    memcpy(register_code + 1, pipe_name, P_PIPE_NAME_SIZE);
    int register_pipe_fd=open_register_pipe(pipe_name);
    // Send to register pipe
    close_register_pipe(register_pipe_fd);
}

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    char pipe_name[P_PIPE_NAME_SIZE + 1] = {0};
    pid_t pid;

    if (argc < 4 || argc > 5) {
        print_usage();
        exit(-1);
    }

    if ((strlen(argv[1]) > P_PIPE_NAME_SIZE) || (strlen(argv[2]) > P_PIPE_NAME_SIZE-5)) {
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
        if (strlen(argv[4]) > P_BOX_NAME_SIZE) {
            print_usage();
            exit(-1);
        }

        if (!strcmp(argv[3], "create")) {
            request_box_creation(argv[1],argv[4]);
        } else if (!strcmp(argv[3], "remove")) {
            request_box_removal(argv[1],argv[4]);
        } else {
            print_usage();
            exit(-1);
        }
        break;
    default:
        print_usage();
        exit(-1);
    }

    return -1;
}

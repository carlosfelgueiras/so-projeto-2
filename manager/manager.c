#include "logging.h"
#include <string.h>

#define MANAGER_REGISTER_CODE_SIZE 289
#define MANAGER_LIST_CODE_SIZE 257

void request_box_creation(char *pipe_name, char *box_name) {
    char register_code[MANAGER_REGISTER_CODE_SIZE] = {0};
    register_code[0] = 3;
    memcpy(register_code + 1, pipe_name, 256);
    memcpy(register_code + 257, box_name, 32);
    // Send to register pipe
}

void request_box_removal(char *pipe_name, char *box_name) {
    char register_code[MANAGER_REGISTER_CODE_SIZE] = {0};
    register_code[0] = 5;
    memcpy(register_code + 1, pipe_name, 256);
    memcpy(register_code + 257, box_name, 32);
    // Send to register pipe
}

void request_box_list(char *pipe_name) {
    char register_code[MANAGER_LIST_CODE_SIZE] = {0};
    register_code[0] = 7;
    memcpy(register_code + 1, pipe_name, 256);
    // Send to register pipe
}

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    char register_pipe[256];
    char pipe_name[256];
    char box_name[32];

    if (argc < 4 || argc > 5) {
        print_usage();
        exit(-1);
    }

    if ((strlen(argv[1]) > 255) || (strlen(argv[2]) > 255)) {
        print_usage();
        exit(-1);
    }

    strcpy(register_pipe, argv[1]);
    strcpy(pipe_name, argv[2]);

    switch (argc) {
    case 4:
        if (!strcmp(argv[3], "list")) {
            // TODO: call function that handles list command
        } else {
            print_usage();
            exit(-1);
        }
        break;
    case 5:
        if (strlen(argv[4]) > 31) {
            print_usage();
            exit(-1);
        }

        if (!strcmp(argv[3], "create")) {
            strcpy(box_name, argv[4]);
            // TODO: function that handles create command
        } else if (!strcmp(argv[3], "remove")) {
            strcpy(box_name, argv[4]);
            // TODO: function that handles remove command
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

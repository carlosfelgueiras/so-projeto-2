#include "logging.h"
#include <string.h>

#define SUBSCRIBER_REGISTER_CODE_SIZE 289

void register_in_mbroker(char *pipe_name, char *box_name) {
    char register_code[SUBSCRIBER_REGISTER_CODE_SIZE] = {0};
    register_code[0] = 2;
    memcpy(register_code + 1, pipe_name, 256);
    memcpy(register_code + 257, box_name, 32);
    // Send to register pipe
}

int main(int argc, char **argv) {
    char register_pipe[256];
    char pipe_name[256];
    char box_name[32];

    if (argc != 4) {
        fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    if ((strlen(argv[1]) > 255) || (strlen(argv[2]) > 255) ||
        (strlen(argv[3]) > 31)) {
        fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    strcpy(register_pipe, argv[1]);
    strcpy(pipe_name, argv[2]);
    strcpy(box_name, argv[3]);

    register_in_mbroker(pipe_name, box_name);

    return 0;
}

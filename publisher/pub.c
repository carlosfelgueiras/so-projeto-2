#include "logging.h"
#include <string.h>

#define PUBLISHER_REGISTER_CODE_SIZE 289
#define PUBLISHER_MESSAGE_CODE_SIZE 1025

void register_in_mbroker(char *pipe_name, char *box_name) {
    char register_code[PUBLISHER_REGISTER_CODE_SIZE] = {0};
    register_code[0] = 1;
    memcpy(register_code + 1, pipe_name, 256);
    memcpy(register_code + 257, box_name, 32);
    // Send to register pipe
}

void send_message_to_mbroker(char *message) {
    char message_code[PUBLISHER_MESSAGE_CODE_SIZE] = {0};
    message_code[0] = 9;
    memcpy(message_code + 1, message, 1024);
    // Send to message pipe
}

int main(int argc, char **argv) {
    char register_pipe[256];
    char pipe_name[257] = {0};
    char box_name[33] = {0};

    if (argc != 4) {
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    if ((strlen(argv[1]) > 256) || (strlen(argv[2]) > 256) ||
        (strlen(argv[3]) > 32)) {
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    strcpy(register_pipe, argv[1]);
    strcpy(pipe_name, argv[2]);
    strcpy(box_name, argv[3]);

    register_in_mbroker(pipe_name, box_name);

    char message[1025];
    while (1) {
        memset(message, 0, 1025);
        fgets(message, 1025, stdin);
        send_message_to_mbroker(message);
    }

    return 0;
}

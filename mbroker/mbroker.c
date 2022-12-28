#include "logging.h"
#include <string.h>

#define MBROKER_MESSAGE_CODE_SIZE 1025

void send_message_to_client(char *message) {
    char message_code[MBROKER_MESSAGE_CODE_SIZE] = {0};
    message_code[0] = 10;
    memcpy(message_code + 1, message, 1024);
    // Send to message pipe
}

int main(int argc, char **argv) {
    char register_pipe[256];
    int max_sessions = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
        exit(-1);
    }

    if (strlen(argv[1]) > 255) {
        fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
        exit(-1);
    }

    strcpy(register_pipe, argv[1]);
    sscanf(argv[2], "%d", &max_sessions);

    if (max_sessions <= 0) {
        fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
        exit(-1);
    }

    return 0;
}

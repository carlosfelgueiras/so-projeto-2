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

int register_pipe_fd;

void send_message_to_client(char *message) {
    char message_code[P_SUB_MESSAGE_SIZE] = {0};
    message_code[0] = 10;
    memcpy(message_code + 1, message, 1024);
    // Send to message pipe
}

void publisher(){
    char pipe[P_PIPE_NAME_SIZE];
    char box_name[P_BOX_NAME_SIZE];
    if(read(register_pipe_fd,pipe,P_PIPE_NAME_SIZE)!=P_PIPE_NAME_SIZE){
        exit(-1);
    }

    if(read(register_pipe_fd,box_name,P_BOX_NAME_SIZE)!=P_BOX_NAME_SIZE){
        exit(-1);
    }
    printf("code: 1 %s %s",pipe,box_name);
}

void subscriber(){
    char pipe[P_PIPE_NAME_SIZE];
    char box_name[P_BOX_NAME_SIZE];
    if(read(register_pipe_fd,pipe,P_PIPE_NAME_SIZE)!=P_PIPE_NAME_SIZE){
        exit(-1);
    }

    if(read(register_pipe_fd,box_name,P_BOX_NAME_SIZE)!=P_BOX_NAME_SIZE){
        exit(-1);
    }
    printf("code: 2 %s %s",pipe,box_name);
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

    register_pipe_fd=open(register_pipe,O_RDONLY);
    if (register_pipe_fd<0){
        exit(-1);
    }

    while(1){
        char code;
        if(read(register_pipe_fd,&code,1)!=1) {
            exit(-1);
        }
        switch (code){
            case P_PUB_REGISTER_CODE:
                publisher();
                break;
            case P_SUB_REGISTER_CODE:
                subscriber();
                break;
            case P_BOX_CREATION_CODE:
                break;
            case P_BOX_REMOVAL_CODE:
                break;
            case P_BOX_LISTING_CODE:
                break; 
            default:
                break; 
        }
    }

    if(close(register_pipe_fd)<0) {
        exit(-1);
    }

    return 0;
}

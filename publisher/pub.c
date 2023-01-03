//logging.h????????????
#include "betterassert.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#define PUBLISHER_REGISTER_CODE_SIZE 289
#define PUBLISHER_MESSAGE_CODE_SIZE 1025
#define PIPE_SIZE 257
#define MESSAGE_SIZE 1025
#define BOX_SIZE 33

void register_in_mbroker(char *register_pipename,char *pipe_name, char *box_name) {
    char register_code[PUBLISHER_REGISTER_CODE_SIZE] = {0};
    char register_pn[PIPE_SIZE+5]={0};

    //Creating the code according to the protocol
    register_code[0] = 1;
    memcpy(register_code + 1, pipe_name, PIPE_SIZE-1);
    memcpy(register_code + PIPE_SIZE, box_name, BOX_SIZE-1);
    sprintf(register_pn,"/tmp/%s",register_pipename);

    //Open register pipe
    int pipe_fd = open(register_pn, O_WRONLY);
    if(pipe_fd<0) {
        exit(-1);
    }
    //Writing the code to the register pipe
    ssize_t bytes_wr=write(pipe_fd,register_code,PUBLISHER_REGISTER_CODE_SIZE);
    ALWAYS_ASSERT(bytes_wr==PUBLISHER_REGISTER_CODE_SIZE,"pub: error sending register code");
    if(close(pipe_fd)<0) { //Closing the register pipe
        exit(-1);
    }
}

void send_message_to_mbroker(int pipe_fd,char *message) {
    char message_code[PUBLISHER_MESSAGE_CODE_SIZE] = {0};
    //Creating the code according to the protocol
    message_code[0] = 9;
    memcpy(message_code + 1, message, MESSAGE_SIZE-1);
    //Writing the code to the associated pipe
    ssize_t bytes_wr=write(pipe_fd,message_code,PUBLISHER_MESSAGE_CODE_SIZE);
    ALWAYS_ASSERT(bytes_wr==PUBLISHER_MESSAGE_CODE_SIZE,"pub: error sendind message");
}

int main(int argc, char **argv) {
    char pipe_name[PIPE_SIZE] = {0};
    char tmp_pipe_name[PIPE_SIZE+5]={0};
    pid_t pid;

    if (argc != 4) { //Verifying the correct usage of arguments
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    if ((strlen(argv[1]) > PIPE_SIZE-1) || (strlen(argv[2]) > PIPE_SIZE-6) ||
        (strlen(argv[3]) > BOX_SIZE-1)) { //Verifying the correct usage of arguments
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    pid=getpid(); //maximum legnth of pid is 5 digits

    sprintf(pipe_name,"%s%05d",argv[2],pid); //Associating the pid to the pipe_name given

    sprintf(tmp_pipe_name,"/tmp/%s",pipe_name); //To create the pipe in tmp directory

    if (unlink(tmp_pipe_name) != 0 && errno != ENOENT) { //To prevent the case where the pipe already exists
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipe_name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Create pipe
    if (mkfifo(tmp_pipe_name, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    register_in_mbroker(argv[1],pipe_name, argv[3]);
    /*como tratar se o broker rejeitar o broker*/

    //Opening the associated pipe
    int pipe_fd = open(tmp_pipe_name, O_WRONLY);

    if(pipe_fd<0) {
        exit(-1);
    }

    char message[MESSAGE_SIZE];
    while (1) {
        //Read the message from stdin
        memset(message, 0, MESSAGE_SIZE);
        if(fgets(message, MESSAGE_SIZE, stdin)==NULL){
            break;
        }
        send_message_to_mbroker(pipe_fd,message);
    }

    if(close(pipe_fd)<0){ //closing the associated pipe
        exit(-1);
    }

    return 0;
}

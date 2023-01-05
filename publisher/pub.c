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

#define BOX_SIZE 33

char tmp_pipe_name[P_PIPE_NAME_SIZE + 6];
int pipe_status = 0;
int pipe_fd;

void register_in_mbroker(char *register_pipename, char *pipe_name,
                         char *box_name) {
    char register_code[P_PUB_REGISTER_SIZE] = {0};
    char register_pn[P_PIPE_NAME_SIZE + 6] = {0};

    // Creating the code according to the protocol
    register_code[0] = P_PUB_REGISTER_CODE;
    memcpy(register_code + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(register_code + P_PIPE_NAME_SIZE + 1, box_name, BOX_SIZE - 1);
    sprintf(register_pn, "/tmp/%s", register_pipename);

    // Open register pipe
    int register_pipe_fd = open(register_pn, O_WRONLY);
    if (register_pipe_fd < 0) {
        exit(-1);
    }
    // Writing the code to the register pipe
    ssize_t bytes_wr =
        write(register_pipe_fd, register_code, P_PUB_REGISTER_SIZE);

    if (bytes_wr != P_PUB_REGISTER_SIZE) {
        exit(-1);
    }

    if (close(pipe_fd) < 0) { // Closing the register pipe
        exit(-1);
    }
}

void send_message_to_mbroker(char *message) {
    char message_code[P_PUB_MESSAGE_SIZE] = {0};
    // Creating the code according to the protocol
    message_code[0] = 9;
    memcpy(message_code + 1, message, P_MESSAGE_SIZE);
    // Writing the code to the associated pipe
    ssize_t bytes_wr = write(pipe_fd, message_code, P_PUB_MESSAGE_SIZE);
    if (bytes_wr != P_PUB_MESSAGE_SIZE) {
        exit(-1);
    };
}

static void signal_handler(int sig) {
    if (pipe_status == 1) { // if the pipe is open, closes it
        close(pipe_fd);
    }

    unlink(tmp_pipe_name);

    switch (sig) { // since both signals cause simmilar effects, we use the same
                   // handler
    case SIGPIPE:
        write(1, "[ERR]: unable to register on mbroker\n", 37);
        exit(EXIT_FAILURE);
        break;
    case SIGINT:
        write(1, "pipes closed and program terminated\n", 37);
        exit(0);
        break;
    default:
        break;
    }
}

int main(int argc, char **argv) {
    char pipe_name[P_PIPE_NAME_SIZE + 1] = {0};
    pid_t pid;

    if (signal(SIGPIPE, signal_handler) ==
        SIG_ERR) { // swaps the signal handler
        fprintf(stderr, "signal\n");
        exit(-1);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR) { // swaps the signal handler
        fprintf(stderr, "signal\n");
        exit(-1);
    }

    if (argc != 4) { // Verifying the correct usage of arguments
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    if ((strlen(argv[1]) > P_PIPE_NAME_SIZE) ||
        (strlen(argv[2]) > P_PIPE_NAME_SIZE - 5) ||
        (strlen(argv[3]) >
         BOX_SIZE - 1)) { // Verifying the correct usage of arguments
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
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

    register_in_mbroker(argv[1], pipe_name, argv[3]);
    /*como tratar se o broker rejeitar o broker*/

    // Opening the associated pipe
    pipe_fd = open(tmp_pipe_name, O_WRONLY);

    pipe_status = 1;

    if (pipe_fd < 0) {
        exit(-1);
    }

    char message[P_MESSAGE_SIZE + 1];
    while (1) {
        // Read the message from stdin
        memset(message, 0, P_MESSAGE_SIZE + 1);
        if (fgets(message, P_MESSAGE_SIZE + 1, stdin) == NULL) {
            break;
        }
        send_message_to_mbroker(message);
    }

    raise(SIGINT);

    return 0;
}

// logging.h????????????
#include "betterassert.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define PUBLISHER_REGISTER_CODE_SIZE 289
#define PUBLISHER_MESSAGE_CODE_SIZE 1025
#define PIPE_SIZE 257
#define MESSAGE_SIZE 1025
#define BOX_SIZE 33

char tmp_pipe_name[PIPE_SIZE + 5];
int pipe_status = 0;
int pipe_fd;
int msg_count = 0;

void register_in_mbroker(char *register_pipename, char *pipe_name,
                         char *box_name) {
    char register_code[PUBLISHER_REGISTER_CODE_SIZE] = {0};
    char register_pn[PIPE_SIZE + 5] = {0};

    // Creating the code according to the protocol
    register_code[0] = 2;
    memcpy(register_code + 1, pipe_name, PIPE_SIZE - 1);
    memcpy(register_code + PIPE_SIZE, box_name, BOX_SIZE - 1);
    sprintf(register_pn, "/tmp/%s", register_pipename);

    // Open register pipe
    int register_pipe_fd = open(register_pn, O_WRONLY);
    if (register_pipe_fd < 0) {
        exit(-1);
    }
    // Writing the code to the register pipe
    ssize_t bytes_wr =
        write(register_pipe_fd, register_code, PUBLISHER_REGISTER_CODE_SIZE);
    ALWAYS_ASSERT(bytes_wr == PUBLISHER_REGISTER_CODE_SIZE,
                  "pub: error sending register code");
    if (close(pipe_fd) < 0) { // Closing the register pipe
        exit(-1);
    }
}

static void signal_handler(int sig) {
    if (pipe_status == 1) { // if the pipe is open, closes it
        close(pipe_fd);
    }

    unlink(tmp_pipe_name);

    //char value[20];

    switch (sig) { // since both signals cause simmilar effects, we use the same handler
    case SIGPIPE:
        write(1, "[ERR]: unable to register on mbroker\n", 37);
        exit(EXIT_FAILURE);
        break;
    case SIGINT:

        write(1, "pipes closed and program terminated\nnumber of messages: ", 37);
        write(1, "\n", 1);
        exit(0);
        break;
    default:
        break;
    }
}

int main(int argc, char **argv) {
    char pipe_name[PIPE_SIZE] = {0};
    pid_t pid;

    if (signal(SIGPIPE, signal_handler) == SIG_ERR) {
        fprintf(stderr, "signal\n");
        exit(-1);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "signal\n");
        exit(-1);
    }

    if (argc != 4) { // Verifying the correct usage of arguments
        fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    if ((strlen(argv[1]) > PIPE_SIZE - 1) ||
        (strlen(argv[2]) > PIPE_SIZE - 6) ||
        (strlen(argv[3]) >
         BOX_SIZE - 1)) { // Verifying the correct usage of arguments
        fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
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
    pipe_fd = open(tmp_pipe_name, O_RDONLY);
    pipe_status = 1;

    if (pipe_fd < 0) {
        exit(-1);
    }

    char message[MESSAGE_SIZE];
    memset(message, 0, MESSAGE_SIZE);
    while (1) {
        int code;

        // if the pipe is closed exits the program
        if (read(pipe_fd, &code, 1) < 0) {
            exit(-1);
        }

        // checks if the code is not corrupt
        if (code != 10) {
            exit(-1);
        }
        if (read(pipe_fd, message, 1024) != 1024) {
            exit(-1);
        }

        printf("%s\n", message);
        msg_count++;
    }

    return 0;
}

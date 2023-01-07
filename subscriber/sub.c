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

char tmp_pipe_name[P_PIPE_NAME_SIZE + 6]; // Full name of the pipe
char number_of_messages[20] = "0"; // String with the total number of messages
int pipe_status = 0;               // If the pipe is open or not
int pipe_fd;                       // File descriptor of the pipe

void register_in_mbroker(char *register_pipename, char *pipe_name,
                         char *box_name) {
    /*Funtion that register the subscriber in mbroker*/

    char register_code[P_SUB_REGISTER_SIZE] = {0};
    char register_pn[P_PIPE_NAME_SIZE + 6] = {0};

    // Creating the code according to the protocol
    p_build_sub_register(register_code, pipe_name, box_name);
    sprintf(register_pn, "/tmp/%s", register_pipename);

    // Open register pipe
    int register_pipe_fd = open(register_pn, O_WRONLY);
    if (register_pipe_fd < 0) {
        exit(-1);
    }
    // Writing the code to the register pipe
    ssize_t bytes_wr =
        write(register_pipe_fd, register_code, P_SUB_REGISTER_SIZE);

    if (bytes_wr != P_SUB_REGISTER_SIZE) { // Verifying if the code was sent
                                           // completely to the pipe
        exit(-1);
    }

    if (close(register_pipe_fd) < 0) { // Closing the register pipe
        exit(-1);
    }
}

static void signal_handler(int sig) {
    if (pipe_status == 1) { // if the pipe is open, closes it
        close(pipe_fd);
    }

    unlink(tmp_pipe_name); // Delete the pipe im tmp directory

    ssize_t bytes_wr;
    (void)bytes_wr;
    switch (sig) { // since both signals cause simmilar effects, we use the same
                   // handler
    case SIGPIPE:
        bytes_wr = write(1, "[ERR]: unable to register on mbroker\n", 37);
        _exit(EXIT_FAILURE);
        break;
    case SIGINT:

        bytes_wr = write(
            1, "pipes closed and program terminated\nnumber of messages: ", 37);
        bytes_wr = write(1, number_of_messages, 20);
        bytes_wr = write(1, "\n", 1);
        signal(sig, SIG_DFL);
        raise(sig);
        break;
    default:
        break;
    }
}

int main(int argc, char **argv) {
    char pipe_name[P_PIPE_NAME_SIZE + 1] = {0};
    pid_t pid;
    // Changing the default handler to signal_handler for SIGPIPE and SIGINT
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

    if ((strlen(argv[1]) > P_PIPE_NAME_SIZE) ||
        (strlen(argv[2]) > P_PIPE_NAME_SIZE - 5) ||
        (strlen(argv[3]) >
         P_BOX_NAME_SIZE)) { // Verifying the correct usage of arguments
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

    // Opening the associated pipe
    pipe_fd = open(tmp_pipe_name, O_RDONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    pipe_status = 1; // Changes to one if the pipe is opened

    char message[P_MESSAGE_SIZE + 1]; // Array that will hold the messages
    memset(message, 0, P_MESSAGE_SIZE + 1);
    while (1) {
        int code;
        int msg_count;
        // if the pipe is closed exits the program
        if (read(pipe_fd, &code, 1) < 0) {
            exit(-1);
        }

        // checks if the code is not corrupt
        if (code != P_SUB_MESSAGE_CODE) {
            exit(-1);
        }

        // reads the message
        if (read(pipe_fd, message, P_MESSAGE_SIZE) != P_MESSAGE_SIZE) {
            exit(-1);
        }

        printf("%s\n", message);
        // Augments by one the number of messages read
        sscanf(number_of_messages, "%d", &msg_count);
        sprintf(number_of_messages, "%d", ++msg_count);
    }

    return 0;
}

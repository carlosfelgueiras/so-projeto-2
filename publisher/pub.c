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

char tmp_pipe_name[P_PIPE_NAME_SIZE + 5]; // Full name of the pipe
int pipe_status = 0;                      // If the pipe is open or not
int pipe_fd;                              // File descriptor of the pipe

/*Function that register the publisher in mbroker*/
void register_in_mbroker(char *register_pipename, char *pipe_name,
                         char *box_name) {
    char register_code[P_PUB_REGISTER_SIZE];
    char register_pn[P_PIPE_NAME_SIZE + 5] = {0}; //Pipe name of the register pipe

    // Creating the code according to the protocol
    p_build_pub_register(register_code, pipe_name, box_name);
    //To open the pipe in tmp directory
    sprintf(register_pn, "/tmp/%s", register_pipename);

    // Open register pipe
    int register_pipe_fd = open(register_pn, O_WRONLY);
    if (register_pipe_fd < 0) {
        exit(-1);
    }
    // Writing the code to the register pipe
    ssize_t bytes_wr =
        write(register_pipe_fd, register_code, P_PUB_REGISTER_SIZE);

    if (bytes_wr != P_PUB_REGISTER_SIZE) { // Chekcks if the code was fully sent
        exit(-1);
    }

    if (close(register_pipe_fd) < 0) { // Closing the register pipe
        exit(-1);
    }
}
/*Function that creates the code to send the message*/
void send_message_to_mbroker(char *message) {
    char message_code[P_PUB_MESSAGE_SIZE] = {0};

    // Creating the code
    p_build_pub_message(message_code, message);
    
    // Writing the code to the associated pipe
    ssize_t bytes_wr = write(pipe_fd, message_code, P_PUB_MESSAGE_SIZE);
    if (bytes_wr != P_PUB_MESSAGE_SIZE) {
        exit(-1);
    };
}

/*Signal handler to handle the signals SIGPIPE AND SIGINT
(SIGINT was not specified to be handled in publisher, 
but we did for simplicity)*/
static void signal_handler(int sig) {
    if (pipe_status == 1) { // if the pipe is open, closes it
        close(pipe_fd);
    }

    unlink(tmp_pipe_name); // Erases the pipe from tmp directory
    ssize_t bytes_wr;
    (void)bytes_wr;
    switch (sig) { // since both signals cause simmilar effects, we use the same
                   // handler
    case SIGPIPE:
        bytes_wr = write(1, "[ERR]: unable to register on mbroker\n", 37);
        _exit(EXIT_FAILURE);
        break;
    case SIGINT:
        bytes_wr = write(1, "pipes closed and program terminated\n", 37);
        signal(sig, SIG_DFL);
        raise(sig);
        break;
    default:
        break;
    }
}

int main(int argc, char **argv) {
    char pipe_name[P_PIPE_NAME_SIZE];
    pid_t pid;

    if (signal(SIGPIPE, signal_handler) ==
        SIG_ERR) { // swaps the default signal handler for SIGPIPE
        fprintf(stderr, "signal\n");
        exit(-1);
    }

    if (signal(SIGINT, signal_handler) ==
        SIG_ERR) { // swaps the default signal handler for SIGINT
        fprintf(stderr, "signal\n");
        exit(-1);
    }

    if (argc != 4) { // Verifying the correct usage of arguments
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    if ((strlen(argv[1]) > P_PIPE_NAME_SIZE - 1) ||
        (strlen(argv[2]) > P_PIPE_NAME_SIZE - 6) ||
        (strlen(argv[3]) >
         P_BOX_NAME_SIZE - 1)) { // Verifying the correct usage of arguments
        fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
        exit(-1);
    }

    pid = getpid(); // Maximum legnth of pid is 5 digits

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
    pipe_fd = open(tmp_pipe_name, O_WRONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    pipe_status = 1; // Changes to 1 if the pipe is open

    char message[P_MESSAGE_SIZE];
    while (1) {
        // Read the message from stdin
        memset(message, 0, P_MESSAGE_SIZE);
        // Fails if it reads EOF
        if (fgets(message, P_MESSAGE_SIZE, stdin) == NULL) {
            break;
        }
        size_t new_line = strlen(message);
        message[new_line - 1] = '\0';
        send_message_to_mbroker(message);
    }

    raise(SIGINT); //To close the publisher session in the manner we want

    return 0;
}

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
int pipe_existance = 0; //If the pipe associated to the client exists

/*Comparator to sort the boxes through their names in alphabetical order*/
int compare_box(const void *a, const void *b) {
    return strcmp(((p_box_response *)a)->box_name,
                  ((p_box_response *)b)->box_name);
}

/*function to open the register pipe*/
int open_register_pipe(char *register_pipename) {
    char register_pn[P_PIPE_NAME_SIZE + 5];
    sprintf(register_pn, "/tmp/%s", register_pipename); //To acess the tmp directory where the pipe is

    int register_pipe_fd = open(register_pn, O_WRONLY);
    if (register_pipe_fd < 0) {
        exit(-1);
    }
    return register_pipe_fd; //returns the file descriptor for the pipe
}

/*Function to close the register pipe*/
void close_register_pipe(int register_pipe_fd) {
    if (close(register_pipe_fd) < 0) {
        exit(-1);
    }
}

/*Function to open the pipe associated to the client*/
int open_pipe() {
    int pipe_fd = open(tmp_pipe_name, O_RDONLY);

    if (pipe_fd < 0) {
        exit(-1);
    }

    return pipe_fd; //Returns the file descriptor for the pipe
}

/*Function to close the pipe associated to the client*/
void close_pipe(int fd) {
    if (close(fd) < 0) {
        exit(-1);
    }
}

/*Function that sends the request to mbroker for the creation of a box*/
void request_box_creation(char *register_pipe_name, char *pipe_name,
                          char *box_name) {
    char register_code[P_BOX_CREATION_SIZE];

    //Creating the protocol message to send to mbroker
    p_build_box_creation(register_code, pipe_name, box_name);

    int register_pipe_fd = open_register_pipe(register_pipe_name); //Opening the register pipe

    if (write(register_pipe_fd, register_code, P_BOX_CREATION_SIZE) !=
        P_BOX_CREATION_SIZE) { //Sending the message and verifying if it was sent correctly
        exit(-1);
    }

    close_register_pipe(register_pipe_fd); //Closing the register pipe

    int pipe_fd = open_pipe(); //Opening the pipe associated to the client

    p_response response; //Struct for the response from the mbroker
    //Reading the response from the pipe associated to the client
    if (read(pipe_fd, &response, sizeof(p_response)) != sizeof(p_response)) {
        exit(-1);
    }
    //Veryfing if the code sent is not corrupted
    if (response.protocol_code != P_BOX_CREATION_RESPONSE_CODE) {
        exit(-1);
    }
    /*Veryfing the status of the request, is it is 0, 
    it was done sucessfully, if it is -1, an error was caused*/
    if (response.return_code == 0) { 
        fprintf(stdout, "OK\n");
    } else if (response.return_code == -1) {
        fprintf(stdout, "ERROR %s\n", response.error_message);
    } else {
        exit(-1);
    }

    close_pipe(pipe_fd); //Closing the pipe associated to the client
}

/*Function that sends the request to mbroker for the removal of a box*/
void request_box_removal(char *register_pipe_name, char *pipe_name,
                         char *box_name) {
    char register_code[P_BOX_REMOVAL_SIZE];

    //Creating the protocol message to send to mbroker
    p_build_box_removal(register_code, pipe_name, box_name);

    int register_pipe_fd = open_register_pipe(register_pipe_name); //Opening the register pipe

    if (write(register_pipe_fd, register_code, P_BOX_REMOVAL_SIZE) !=
        P_BOX_REMOVAL_SIZE) { //Sending the message and verifying if it was sent correctly
        exit(-1);
    }

    close_register_pipe(register_pipe_fd); //Closing the register pipe

    int pipe_fd = open_pipe(); //Opening the pipe associated to the client

    p_response response; //Struct for the response from the mbroker
    //Reading the response from the pipe associated to the client
    if (read(pipe_fd, &response, P_BOX_REMOVAL_RESPONSE_SIZE) !=
        P_BOX_REMOVAL_RESPONSE_SIZE) {
        exit(-1);
    }
    //Veryfing if the code sent is not corrupted
    if (response.protocol_code != P_BOX_REMOVAL_RESPONSE_CODE) {
        exit(-1);
    }
    /*Veryfing the status of the request, is it is 0, 
    it was done sucessfully, if it is -1, an error was caused*/
    if (response.return_code == 0) {
        fprintf(stdout, "OK\n");
    } else if (response.return_code == -1) {
        fprintf(stdout, "ERROR %s\n", response.error_message);
    } else {
        exit(-1);
    }

    close_pipe(pipe_fd); //Closing the pipe associated to the client
}

/*Function that sends the request to mbroker for the listing of all the boxes*/
void request_box_list(char *pipe_name, char *register_pipe_name) {
    char register_code[P_BOX_LISTING_SIZE];

    //Creating the protocol message to send to mbroker
    p_build_box_listing(register_code, pipe_name);

    int register_pipe_fd = open_register_pipe(register_pipe_name); //Opening the register pipe

    if (write(register_pipe_fd, register_code, P_BOX_LISTING_SIZE) !=
        P_BOX_LISTING_SIZE) { //Sending the message and verifying if it was sent correctly
        exit(-1);
    }

    close_register_pipe(register_pipe_fd); //Closing the register pipe

    int pipe_fd = open_pipe(); //Opening the pipe associated to the client
    
    //An array wich will hold all boxes for listing 
    p_box_response *array = (p_box_response *)malloc(sizeof(p_box_response));
    unsigned int size = 0; //Current capacity occupied in the array
    unsigned int cap = 1;  //Total capacity of the array

    p_box_response response; //Struct which will hold the response from the mbroker
    while (1) {
        if (read(pipe_fd, &response, sizeof(p_box_response)) !=
            sizeof(p_box_response)) { //Reading the response from the pipe associated to the client
            exit(-1);
        }
        //Veryfing if the code sent is not corrupted
        if (response.protocol_code != P_BOX_LISTING_RESPONSE_CODE) {
            exit(-1);
        }
        //If the array fills up, we double its size
        if (size >= cap) {
            cap *= 2;
            array =
                (p_box_response *)realloc(array, cap * sizeof(p_box_response));
        }
        //Adding the response to the array and increasing its size
        array[size++] = response;

        if (response.last == 1) //Verifies if the the box sent is the last one
            break;
    }

    close_pipe(pipe_fd); //Closing the pipe associated to the client

    if (size == 1 && strlen(array[0].box_name) == 0) { //Verifies if there were no boxes sent
        fprintf(stdout, "NO BOXES FOUND\n");
        free(array);
        return;
    }

    //Sort the boxes through their names in alphabetical order
    qsort(array, (size_t)size, sizeof(p_box_response), compare_box);

    for (int i = 0; i < size; i++) { //Print to stdout the boxes and their attributes
        fprintf(stdout, "%s %zu %zu %zu\n", array[i].box_name,
                (size_t)array[i].box_size, (size_t)array[i].n_publishers,
                (size_t)array[i].n_subscribers);
    }

    free(array);
}

/*Hanler to finalise this session, destroying the pipe associated to the client*/
void signal_handler(int sig) {
    (void)sig;
    ssize_t bytes;
    (void)bytes;
    if (pipe_existance == 1) { //If the pipe was created
        if (unlink(tmp_pipe_name) != 0 &&
            errno !=
                ENOENT) { // To prevent the case where the pipe already exists
            bytes = write(2, "unlink fail\n", 13);
            _exit(EXIT_FAILURE);
        }
        bytes = write(1, "pipe destroyed\n", 16);
    }

    _exit(EXIT_SUCCESS);
}

/*Function that in case of incorrect usage, shows the various usages otf the client*/
static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    char pipe_name[P_PIPE_NAME_SIZE] = {0};
    pid_t pid;

    if (argc < 4 || argc > 5) { //Checks if the number of arguments are incorrect
        print_usage();
        exit(-1);
    }

    if ((strlen(argv[1]) > P_PIPE_NAME_SIZE - 1) ||
        (strlen(argv[2]) > P_PIPE_NAME_SIZE - 6)) { //Checks if the pipenames received are correct
        print_usage();
        exit(-1);
    }

    if (signal(SIGINT, signal_handler) ==
        SIG_ERR) { // swaps the signal handler for SIGINT
        fprintf(stderr, "signal\n");
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

    pipe_existance = 1; //Shows that the pipe already exists

    switch (argc) {
    case 4: //In case the number of arguments is 4
        if (!strcmp(argv[3], "list")) { //Checks if it is a request to list the boxes
            request_box_list(pipe_name, argv[1]);
        } else {
            print_usage();
            exit(-1);
        }
        break;
    case 5: //In case the number of arguments is 5
        if (strlen(argv[4]) > P_BOX_NAME_SIZE - 1) { //Checks if the box name sent is correct
            print_usage();
            exit(-1);
        }

        if (!strcmp(argv[3], "create")) { //Checks if it is a request to create a box
            request_box_creation(argv[1], pipe_name, argv[4]);
        } else if (!strcmp(argv[3], "remove")) { //Checks if it is a resquest to remove a box
            request_box_removal(argv[1], pipe_name, argv[4]);
        } else {
            print_usage();
            exit(-1);
        }
        break;
    default:
        print_usage();
        exit(-1);
    }

    return 0;
}

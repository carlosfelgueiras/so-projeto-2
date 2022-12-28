#include "logging.h"
#include <string.h>

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

    return 0;
}

#include <protocol.h>
#include <string.h>

void p_build_pub_register(char dest[P_PUB_REGISTER_SIZE],
                          char pipe_name[P_PIPE_NAME_SIZE],
                          char box_name[P_BOX_NAME_SIZE]) {
    memset(dest, 0, P_PUB_REGISTER_SIZE);

    dest[0] = P_PUB_REGISTER_CODE;
    memcpy(dest + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(dest + P_PIPE_NAME_SIZE + 1, box_name, P_BOX_NAME_SIZE);
}

void p_build_sub_register(char dest[P_SUB_REGISTER_SIZE],
                          char pipe_name[P_PIPE_NAME_SIZE],
                          char box_name[P_BOX_NAME_SIZE]) {
    memset(dest, 0, P_SUB_REGISTER_SIZE);

    dest[0] = P_SUB_REGISTER_CODE;
    memcpy(dest + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(dest + P_PIPE_NAME_SIZE + 1, box_name, P_BOX_NAME_SIZE);
}

void p_build_box_creation(char dest[P_BOX_CREATION_SIZE],
                          char pipe_name[P_PIPE_NAME_SIZE],
                          char box_name[P_BOX_NAME_SIZE]) {
    memset(dest, 0, P_BOX_CREATION_SIZE);

    dest[0] = P_BOX_CREATION_CODE;
    memcpy(dest + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(dest + P_PIPE_NAME_SIZE + 1, box_name, P_BOX_NAME_SIZE);
}

void p_build_box_creation_response(char dest[P_BOX_CREATION_RESPONSE_SIZE],
                                   p_response response) {
    memset(dest, 0, P_BOX_CREATION_RESPONSE_SIZE);
    dest[0] = P_BOX_CREATION_RESPONSE_CODE;

    memcpy(dest + 1, &response, sizeof(response));
}

void p_build_box_removal(char dest[P_BOX_REMOVAL_SIZE],
                         char pipe_name[P_PIPE_NAME_SIZE],
                         char box_name[P_BOX_NAME_SIZE]) {
    memset(dest, 0, P_BOX_REMOVAL_SIZE);

    dest[0] = P_BOX_REMOVAL_CODE;
    memcpy(dest + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(dest + P_PIPE_NAME_SIZE + 1, box_name, P_BOX_NAME_SIZE);
}

void p_build_box_removal_response(char dest[P_BOX_REMOVAL_RESPONSE_SIZE],
                                  p_response response) {
    memset(dest, 0, P_BOX_REMOVAL_RESPONSE_SIZE);
    dest[0] = P_BOX_REMOVAL_RESPONSE_CODE;

    memcpy(dest + 1, &response, sizeof(response));
}

void p_build_box_listing(char dest[P_BOX_LISTING_SIZE],
                         char pipe_name[P_PIPE_NAME_SIZE]) {
    memset(dest, 0, P_BOX_LISTING_SIZE);

    dest[0] = P_BOX_LISTING_CODE;
    memcpy(dest + 1, pipe_name, P_PIPE_NAME_SIZE);
}

void p_build_box_listing_response(char dest[P_BOX_LISTING_RESPONSE_SIZE],
                                  uint8_t last, p_box_info info) {
    memset(dest, 0, P_BOX_LISTING_RESPONSE_SIZE);

    dest[0] = P_BOX_LISTING_RESPONSE_CODE;
    dest[1] = (char)last;
    memcpy(dest + 2, &info, sizeof(p_box_info));
}
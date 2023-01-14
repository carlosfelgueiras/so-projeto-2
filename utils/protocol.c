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

p_response p_build_box_creation_response(int32_t return_code,
                                         char error_message[P_MESSAGE_SIZE]) {
    p_response response;

    response.protocol_code = P_BOX_CREATION_RESPONSE_CODE;
    response.return_code = return_code;
    strcpy(response.error_message, error_message);

    return response;
}

void p_build_box_removal(char dest[P_BOX_REMOVAL_SIZE],
                         char pipe_name[P_PIPE_NAME_SIZE],
                         char box_name[P_BOX_NAME_SIZE]) {
    memset(dest, 0, P_BOX_REMOVAL_SIZE);

    dest[0] = P_BOX_REMOVAL_CODE;
    memcpy(dest + 1, pipe_name, P_PIPE_NAME_SIZE);
    memcpy(dest + P_PIPE_NAME_SIZE + 1, box_name, P_BOX_NAME_SIZE);
}

p_response p_build_box_removal_response(int32_t return_code,
                                        char error_message[P_MESSAGE_SIZE]) {
    p_response response;

    response.protocol_code = P_BOX_REMOVAL_RESPONSE_CODE;
    response.return_code = return_code;
    strcpy(response.error_message, error_message);

    return response;
}

void p_build_box_listing(char dest[P_BOX_LISTING_SIZE],
                         char pipe_name[P_PIPE_NAME_SIZE]) {
    memset(dest, 0, P_BOX_LISTING_SIZE);

    dest[0] = P_BOX_LISTING_CODE;
    memcpy(dest + 1, pipe_name, P_PIPE_NAME_SIZE);
}

p_box_response p_build_box_listing_response(uint8_t last, p_box_info info) {
    p_box_response response;

    response.protocol_code = P_BOX_LISTING_RESPONSE_CODE;
    response.last = last;
    strcpy(response.box_name, info.box_name + 1);
    response.box_size = info.box_size;
    response.n_publishers = info.n_publishers;
    response.n_subscribers = info.n_subscribers;

    return response;
}

void p_build_pub_message(char dest[P_PUB_MESSAGE_SIZE],
                         char message[P_MESSAGE_SIZE]) {
    memset(dest, 0, P_PUB_MESSAGE_SIZE);

    dest[0] = P_PUB_MESSAGE_CODE;
    memcpy(dest + 1, message, P_MESSAGE_SIZE);
}

void p_build_sub_message(char dest[P_SUB_MESSAGE_SIZE],
                         char message[P_MESSAGE_SIZE]) {
    memset(dest, 0, P_SUB_MESSAGE_SIZE);

    dest[0] = P_SUB_MESSAGE_CODE;
    memcpy(dest + 1, message, P_MESSAGE_SIZE);
}
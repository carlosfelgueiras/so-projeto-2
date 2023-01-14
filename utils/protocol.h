#pragma once

#include <stdint.h>

#define P_PUB_REGISTER_CODE 1
#define P_SUB_REGISTER_CODE 2
#define P_BOX_CREATION_CODE 3
#define P_BOX_CREATION_RESPONSE_CODE 4
#define P_BOX_REMOVAL_CODE 5
#define P_BOX_REMOVAL_RESPONSE_CODE 6
#define P_BOX_LISTING_CODE 7
#define P_BOX_LISTING_RESPONSE_CODE 8
#define P_PUB_MESSAGE_CODE 9
#define P_SUB_MESSAGE_CODE 10

#define P_PUB_REGISTER_SIZE 289
#define P_SUB_REGISTER_SIZE 289
#define P_BOX_CREATION_SIZE 289
#define P_BOX_CREATION_RESPONSE_SIZE 1029
#define P_BOX_REMOVAL_SIZE 289
#define P_BOX_REMOVAL_RESPONSE_SIZE 1029
#define P_BOX_LISTING_SIZE 257
#define P_BOX_LISTING_RESPONSE_SIZE 58
#define P_PUB_MESSAGE_SIZE 1025
#define P_SUB_MESSAGE_SIZE 1025

#define P_PIPE_NAME_SIZE 256
#define P_BOX_NAME_SIZE 32
#define P_MESSAGE_SIZE 1024

#define P_UINT8_SIZE 1
#define P_UINT32_SIZE 4
#define P_UINT64_SIZE 8

typedef struct __attribute__((__packed__)) { //Struct that holds the info of the boxes in the program
    char box_name[P_BOX_NAME_SIZE + 1];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
} p_box_info;

typedef struct __attribute__((__packed__)) {//Structure that is the response to the list request
    uint8_t protocol_code;
    uint8_t last;
    char box_name[P_BOX_NAME_SIZE];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
} p_box_response;

typedef struct __attribute__((__packed__)) { //Structure tht is the response to create and remove requests
    uint8_t protocol_code;
    int32_t return_code;
    char error_message[P_MESSAGE_SIZE];
} p_response;

//Builds the protocol register message for the publisher
void p_build_pub_register(char dest[P_PUB_REGISTER_SIZE],
                          char pipe_name[P_PIPE_NAME_SIZE],
                          char box_name[P_BOX_NAME_SIZE]);

//Builds the protocol register message for the publisher
void p_build_sub_register(char dest[P_SUB_REGISTER_SIZE],
                          char pipe_name[P_PIPE_NAME_SIZE],
                          char box_name[P_BOX_NAME_SIZE]);

//Builds the protocol request message for the creation of a box
void p_build_box_creation(char dest[P_BOX_CREATION_SIZE],
                          char pipe_name[P_PIPE_NAME_SIZE],
                          char box_name[P_BOX_NAME_SIZE]);

//Builds the structure from the response to the creation request
p_response p_build_box_creation_response(int32_t return_code,
                                         char error_message[P_MESSAGE_SIZE]);

//Builds the protocol request message for the removal of a box
void p_build_box_removal(char dest[P_BOX_REMOVAL_SIZE],
                         char pipe_name[P_PIPE_NAME_SIZE],
                         char box_name[P_BOX_NAME_SIZE]);

//Builds the structure from the response to the removal request
p_response p_build_box_removal_response(int32_t return_code,
                                        char error_message[P_MESSAGE_SIZE]);

//Builds the protocol request message for the listing of all boxes
void p_build_box_listing(char dest[P_BOX_LISTING_SIZE],
                         char pipe_name[P_PIPE_NAME_SIZE]);

//Builds the strucutre from the response to the list request
p_box_response p_build_box_listing_response(uint8_t last, p_box_info info);

//Builds the protocol send message for the publisher
void p_build_pub_message(char dest[P_PUB_MESSAGE_SIZE], char message[P_MESSAGE_SIZE]);

//Builds the protocol receive message for the subscriber
void p_build_sub_message(char dest[P_SUB_MESSAGE_SIZE], char message[P_MESSAGE_SIZE]);
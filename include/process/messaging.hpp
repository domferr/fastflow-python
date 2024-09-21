#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include <Python.h>
#include "debugging.hpp"
#include <ff/distributed/ff_network.hpp> // import writen and readn

#define handleError(msg, then) do { perror(msg); then; } while(0)

enum message_type {
    MESSAGE_TYPE_RESPONSE = 1,
    MESSAGE_TYPE_REMOTE_PROCEDURE_CALL,
    MESSAGE_TYPE_GO_ON,
    MESSAGE_TYPE_EOS,
    MESSAGE_TYPE_ACK,
    MESSAGE_TYPE_END_OF_LIFE
};

struct Message {
    message_type type;
    //std::string data;
    char *data;
    long data_len;
    std::string f_name;
};

int sendMessage(int read_fd, int send_fd, const Message& message) {
    // send type
    if (write(send_fd, &message.type, sizeof(message.type)) == -1) {
        handleError("write type", return -1);
    }

    // send data
    if (write(send_fd, &message.data_len, sizeof(message.data_len)) == -1) {
        handleError("write data size", return -1);
    }
    if (message.data_len > 0 && writen(send_fd, message.data, message.data_len) == -1) {
        handleError("write data", return -1);
    }

    // send f_name
    uint32_t fnameSize = message.f_name.length();
    if (write(send_fd, &fnameSize, sizeof(fnameSize)) == -1) {
        handleError("write f_name size", return -1);
    }

    if (fnameSize > 0 && writen(send_fd, message.f_name.c_str(), fnameSize) == -1) {
        handleError("write f_name", return -1);
    }

    int ack; 
    int err = read(read_fd, &ack, sizeof(ack));
    if (err <= 0) handleError("error receiving ACK", return err);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int receiveMessage(int read_fd, int send_fd, Message& message) {
    // recv type
    int res = read(read_fd, &message.type, sizeof(message.type));
    if (res <= 0) handleError("read type", return res);

    // recv data
    res = read(read_fd, &message.data_len, sizeof(message.data_len));
    if (res <= 0) handleError("read data size", return res);

    message.data = new char[message.data_len];
    if (message.data_len > 0) {
        res = readn(read_fd, message.data, message.data_len);
        if (res <= 0) handleError("read data", return res);
    }
    
    // recv f_name
    uint32_t fnameSize;
    res = read(read_fd, &fnameSize, sizeof(fnameSize));
    if (res <= 0) handleError("read fname size", return res);

    char* bufferFname = new char[fnameSize + 1];
    if (fnameSize > 0) {
        res = readn(read_fd, bufferFname, fnameSize);
        if (res <= 0) handleError("read fname", return res);
    }
    
    bufferFname[fnameSize] = '\0';
    message.f_name = std::string(bufferFname);
    delete[] bufferFname;

    int ack = 17; 
    res = write(send_fd, &ack, sizeof(ack)); 
    if (res == -1) handleError("error sending ACK", return res);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int remote_procedure_call(int send_fd, int read_fd, char *data, long data_len, const char *f_name, Message &response) {
    // they are -1 if an error occurred during svc_init or svc
    if (send_fd == -1 || read_fd == -1) return -1;

    int err = sendMessage(read_fd, send_fd, { 
        .type = MESSAGE_TYPE_REMOTE_PROCEDURE_CALL, 
        .data = data,
        .data_len = data_len,
        .f_name = f_name 
    });
    if (err <= 0) return err;

    return receiveMessage(read_fd, send_fd, response);
}

void create_message_ff_send_out_to(Message &message, int index, void* &constant, char *data, long data_len) {
    if (constant != NULL && constant != ff::FF_EOS 
        && constant != ff::FF_GO_ON) {
            throw "fastflow constant not supported";
    }

    message.type = MESSAGE_TYPE_REMOTE_PROCEDURE_CALL;
    message.f_name = "ff_send_out_to";
    std::string index_str = std::to_string(index);
    
    std::string str;
    str.reserve(index_str.length() + 8 + data_len); // 8 is just to be sure there is enough space for the constant
    str.append(index_str);
    str.append("~");
    str.append(constant != NULL ? "t":"f");
    str.append(constant != NULL ? std::to_string(constant == ff::FF_EOS ? MESSAGE_TYPE_EOS:MESSAGE_TYPE_GO_ON):data);
    message.data = new char[str.length() + 1];
    memcpy(message.data, str.c_str(), str.length());
    message.data[str.length()] = '\0';
    message.data_len = str.length();
}

void parse_message_ff_send_out_to(Message &message, void **constant, int *index, std::string *data) {
    std::string str;
    str.assign(message.data, message.data_len);
    int dividerPos = str.find('~');
    *index = std::stoi(str.substr(0, dividerPos));
    if (str.at(dividerPos+1) == 't') {
        *data = "";
        std::string inner_data = str.substr(dividerPos+2, str.length() - dividerPos - 1);
        if (inner_data.compare(std::to_string(MESSAGE_TYPE_EOS)) == 0) {
            *constant = ff::FF_EOS;
        }
        if (inner_data.compare(std::to_string(MESSAGE_TYPE_GO_ON)) == 0) {
            *constant = ff::FF_GO_ON;
        }
    } else {
        *constant = NULL;
        *data = str.substr(dividerPos+2, str.length() - dividerPos - 1);
    }
}

#endif  //MESSAGING_HPP
#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include <Python.h>
#include "debugging.hpp"
#include <ff/distributed/ff_network.hpp> // import writen and readn

#define handleError(msg, then) do { if (errno < 0) { perror(msg); } else { LOG(msg": errno == 0, fd closed" << std::endl); } then; } while(0)

enum message_type {
    MESSAGE_TYPE_RESPONSE = 1,
    MESSAGE_TYPE_REMOTE_PROCEDURE_CALL,
    MESSAGE_TYPE_RESPONSE_GO_ON,
    MESSAGE_TYPE_RESPONSE_EOS,
    MESSAGE_TYPE_ACK
};

struct Message {
    message_type type;
    std::string data;
    std::string f_name;
};

int sendMessage(int read_fd, int send_fd, const Message& message) {
    // send type
    if (write(send_fd, &message.type, sizeof(message.type)) == -1) {
        handleError("write type", return -1);
    }

    // send data
    uint32_t dataSize = message.data.length();
    if (write(send_fd, &dataSize, sizeof(dataSize)) == -1) {
        handleError("write data size", return -1);
    }
    if (dataSize > 0 && writen(send_fd, message.data.c_str(), dataSize) == -1) {
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

    int dummy; 
    int err = read(read_fd, &dummy, sizeof(dummy)); 
    if (err <= 0) handleError("error receiving ACK", return err);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int receiveMessage(int read_fd, int send_fd, Message& message) {
    // recv type
    int res = read(read_fd, &message.type, sizeof(message.type));
    if (res <= 0) handleError("read type", return res);

    // recv data
    uint32_t dataSize;
    res = read(read_fd, &dataSize, sizeof(dataSize));
    if (res <= 0) handleError("read data size", return res);
    char* bufferData = new char[dataSize + 1];
    if (dataSize > 0) {
        res = readn(read_fd, bufferData, dataSize);
        if (res <= 0) handleError("read data", return res);
    }
    
    bufferData[dataSize] = '\0';
    message.data = std::string(bufferData);
    message.data.assign(bufferData, dataSize);
    delete[] bufferData;
    
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

    int dummy = 17; 
    res = write(send_fd, &dummy, sizeof(dummy)); 
    if (res == -1) handleError("error sending ACK", return res);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int remote_procedure_call(int send_fd, int read_fd, std::string &data, const char *f_name, Message &response) {
    if (send_fd == -1 || read_fd == -1) {
        // they are -1 if an error occurred during svc_init or svc
        return -1;
    }

    int err = sendMessage(read_fd, send_fd, { 
        .type = MESSAGE_TYPE_REMOTE_PROCEDURE_CALL, 
        .data = data, 
        .f_name = f_name });
    if (err <= 0) return err;

    return receiveMessage(read_fd, send_fd, response);
}

void create_message_ff_send_out_to(Message &message, int index, void* &constant, std::string &data) {
    if (constant != NULL && constant != ff::FF_EOS 
        && constant != ff::FF_GO_ON) {
            throw "fastflow constant not supported";
    }

    message.type = MESSAGE_TYPE_REMOTE_PROCEDURE_CALL;
    message.f_name = "ff_send_out_to";
    std::string index_str = std::to_string(index);
    message.data.erase();
    message.data.reserve(index_str.length() + 8 + data.length()); // 8 is just to be sure there is enough space for the constant
    message.data.append(index_str);
    message.data.append("~");
    message.data.append(constant != NULL ? "t":"f");
    message.data.append(constant != NULL ? std::to_string(constant == ff::FF_EOS ? MESSAGE_TYPE_RESPONSE_EOS:MESSAGE_TYPE_RESPONSE_GO_ON):data);
}

void parse_message_ff_send_out_to(Message &message, void **constant, int *index, std::string *data) {
    int dividerPos = message.data.find('~');
    *index = std::stoi(message.data.substr(0, dividerPos));
    if (message.data.at(dividerPos+1) == 't') {
        *data = "";
        std::string inner_data = message.data.substr(dividerPos+2, message.data.length() - dividerPos - 1);
        if (inner_data.compare(std::to_string(MESSAGE_TYPE_RESPONSE_EOS)) == 0) {
            *constant = ff::FF_EOS;
        }
        if (inner_data.compare(std::to_string(MESSAGE_TYPE_RESPONSE_GO_ON)) == 0) {
            *constant = ff::FF_GO_ON;
        }
    } else {
        *constant = NULL;
        *data = message.data.substr(dividerPos+2, message.data.length() - dividerPos - 1);
    }
}

#endif  //MESSAGING_HPP
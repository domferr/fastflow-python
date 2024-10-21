#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include <Python.h>
#include "debugging.hpp"
#include <ff/distributed/ff_network.hpp> // import writen and readn
#include <iostream>
#include <sstream>

enum message_type {
    MESSAGE_TYPE_RAW_DATA = 1,
    MESSAGE_TYPE_REMOTE_PROCEDURE_CALL,
    MESSAGE_TYPE_RESPONSE,
    MESSAGE_TYPE_END_OF_LIFE,
};

struct Message {
    message_type type;
    std::vector<std::string> data;
    std::string f_name = "";
};


// Serialize any argument
template<typename T>
std::string serialize(const T& arg) {
    std::ostringstream oss;
    oss << arg;
    return oss.str();
}

template <>
// Serialize a string argument by returning the string
inline std::string serialize<std::string>(const std::string& arg) {
    return arg;
}

// Deserialize any argument
template<typename T>
T deserialize(const std::string& data) {
    std::istringstream iss(data);
    T arg;
    iss >> arg;
    return arg;
}

template <>
// Deserialize a string argument by returning the string
inline std::string deserialize<std::string>(const std::string& data) {
    return data;
}

class Messaging {
public:
    Messaging(int send_fd, int read_fd) : send_fd(send_fd), read_fd(read_fd) {}

    inline int send_data(std::string &data) {
        return send_message(MESSAGE_TYPE_RAW_DATA, "", data);
    }

    template<typename... Args>
    inline int send_response(Args&... args) {
        return send_message(MESSAGE_TYPE_RESPONSE, "", args...);
    }

    template<typename... Args>
    int call_remote(Message &response, const char* fname, Args&... args) {
        start_call_remote(fname, args...);
        return recv_message(response);
    }

    template<typename... Args>
    inline int start_call_remote(const char* fname, Args&... args) {
        return send_message(MESSAGE_TYPE_REMOTE_PROCEDURE_CALL, fname, args...);
    }

    // Function to parse the remote call string and deserialize arguments
    template<typename... Args>
    std::tuple<Args...> parse_data(const std::vector<std::string>& data) {        
        // Deserialize each token back to its respective type and return as a tuple
        return call_remote_to_tuple<Args...>(std::index_sequence_for<Args...>{}, data);
    }

    inline int eol() {
        return send_message(MESSAGE_TYPE_END_OF_LIFE, "");
    }

    template<typename... Args>
    std::tuple<Args...> recv_deserialized_message(Message& message, int *err) {
        *err = recv_message(message);
        if (*err <= 0) return {};
        return parse_data<Args...>(message.data);
    }

    int recv_message(Message& message) {
        // recv type
        int res = read(read_fd, &message.type, sizeof(message.type));
        if (res <= 0) return res;

        size_t data_vector_size; 
        res = read(read_fd, &data_vector_size, sizeof(data_vector_size));
        if (res <= 0) return res;
        message.data.resize(data_vector_size);
        
        // recv the vector of data
        for (size_t i = 0; i < data_vector_size; i++) {
            // recv data i
            uint32_t dataSize;
            res = read(read_fd, &dataSize, sizeof(dataSize));
            if (res <= 0) return res;

            char* bufferData = new char[dataSize + 1];
            if (dataSize > 0) {
                res = readn(read_fd, bufferData, dataSize);
                if (res <= 0) return res;
            }
            
            bufferData[dataSize] = '\0';
            message.data[i].assign(bufferData, dataSize);
            delete[] bufferData;
        }
        
        // recv f_name
        uint32_t fnameSize;
        res = read(read_fd, &fnameSize, sizeof(fnameSize));
        if (res <= 0) return res;

        char* bufferFname = new char[fnameSize + 1];
        if (fnameSize > 0) {
            res = readn(read_fd, bufferFname, fnameSize);
            if (res <= 0) return res;
        }
        
        bufferFname[fnameSize] = '\0';
        message.f_name = std::string(bufferFname);
        delete[] bufferFname;

        int ack = 17; 
        return write(send_fd, &ack, sizeof(ack)); // 0 = EOF, -1 = ERROR, >= 1 = SUCCESS
    }

    inline bool is_closed() {
        return send_fd == -1 || read_fd == -1;
    }

    void closefds() {
        if (send_fd > 0) close(send_fd);
        if (read_fd > 0) close(read_fd);
        send_fd = -1;
        send_fd = -1;
    }

private:
    int send_fd;
    int read_fd;

    // Function to create a tuple of deserialized arguments
    template<typename... Args, std::size_t... Is>
    std::tuple<Args...> call_remote_to_tuple(std::index_sequence<Is...>, const std::vector<std::string> &data) {
        // idea from https://stackoverflow.com/a/59195736
        return std::make_tuple(deserialize<Args>(data[Is])...);
    }

    template<typename... Args>
    int send_message(message_type type, std::string f_name, Args&... args) {
        std::vector<std::string> serialized_args = { serialize(args)... };
        
        // send type
        if (write(send_fd, &type, sizeof(type)) == -1) return -1;

        // send vector data size
        size_t data_vector_size = serialized_args.size();
        if (write(send_fd, &data_vector_size, sizeof(data_vector_size)) == -1) return -1;

        // send vector of data
        for (size_t i = 0; i < serialized_args.size(); i++) {
            // send data
            auto data = serialized_args[i];
            uint32_t dataSize = data.size();
            if (write(send_fd, &dataSize, sizeof(dataSize)) == -1) return -1;
            if (dataSize > 0 && writen(send_fd, data.c_str(), dataSize) == -1) return -1;
        }

        // send f_name
        uint32_t fnameSize = f_name.length();
        if (write(send_fd, &fnameSize, sizeof(fnameSize)) == -1) return -1;
        if (fnameSize > 0 && writen(send_fd, f_name.c_str(), fnameSize) == -1) return -1;

        int ack; 
        return read(read_fd, &ack, sizeof(ack)); // 0 = EOF, -1 = ERROR, >= 1 = SUCCESS
    }
};

#endif  //MESSAGING_HPP
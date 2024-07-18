#ifndef PY_FF_NODE_PROCESS
#define PY_FF_NODE_PROCESS

#include <Python.h>
#include <ff/ff.hpp>
#include <ff/distributed/ff_network.hpp>
#include <iostream>
#include <sys/wait.h>
#include "error_macros.hpp"
#include "pickle.hpp"
#include "log.hpp"

#define handleError(msg, then) do { if (errno < 0) { perror(msg); } else { LOG(msg": errno == 0, fd closed\n"); } then; } while(0)

int sendMessage(int read_fd, int send_fd, std::string &data) {
    // send data
    uint32_t dataSize = data.length();
    if (write(send_fd, &dataSize, sizeof(dataSize)) == -1) {
        handleError("write data size", return -1);
    }
    if (dataSize > 0 && writen(send_fd, data.c_str(), dataSize) == -1) {
        handleError("write data", return -1);
    }

    char ack; 
    int err = read(read_fd, &ack, sizeof(ack)); 
    if (err <= 0) handleError("error receiving ACK", return err);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int sendMessage(int read_fd, int send_fd, std::string &data, std::string &optional) {
    // send data
    uint32_t dataSize = data.length();
    if (write(send_fd, &dataSize, sizeof(dataSize)) == -1) {
        handleError("write data size", return -1);
    }
    if (dataSize > 0 && writen(send_fd, data.c_str(), dataSize) == -1) {
        handleError("write data", return -1);
    }

    // send optional
    uint32_t optionalSize = data.length();
    if (write(send_fd, &optionalSize, sizeof(optionalSize)) == -1) {
        handleError("write optional data size", return -1);
    }
    if (optionalSize > 0 && writen(send_fd, optional.c_str(), optionalSize) == -1) {
        handleError("write optional data", return -1);
    }

    char ack; 
    int err = read(read_fd, &ack, sizeof(ack)); 
    if (err <= 0) handleError("error receiving ACK", return err);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int receiveMessage(int read_fd, int send_fd, std::string *data) {
    // recv data
    uint32_t dataSize;
    int res = read(read_fd, &dataSize, sizeof(dataSize));
    if (res <= 0) handleError("read data size", return res);
    char* bufferData = new char[dataSize + 1];
    if (dataSize > 0) {
        res = readn(read_fd, bufferData, dataSize);
        if (res <= 0) handleError("read data", return res);
    }
    
    bufferData[dataSize] = '\0';
    data->assign(bufferData, dataSize);
    delete[] bufferData;

    char ack = '#'; 
    res = write(send_fd, &ack, sizeof(ack)); 
    if (res == -1) handleError("error sending ACK", return res);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int receiveMessage(int read_fd, int send_fd, std::string &data) {
    // recv data
    uint32_t dataSize;
    int res = read(read_fd, &dataSize, sizeof(dataSize));
    if (res <= 0) handleError("read data size", return res);
    char* bufferData = new char[dataSize + 1];
    if (dataSize > 0) {
        res = readn(read_fd, bufferData, dataSize);
        if (res <= 0) handleError("read data", return res);
    }
    
    bufferData[dataSize] = '\0';
    data.assign(bufferData, dataSize);
    delete[] bufferData;

    // send ack
    char ack = '#'; 
    res = write(send_fd, &ack, sizeof(ack)); 
    if (res == -1) handleError("error sending ACK", return res);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int receiveMessage(int read_fd, int send_fd, std::string &data, std::string &optional) {
    // recv data
    uint32_t dataSize;
    int res = read(read_fd, &dataSize, sizeof(dataSize));
    if (res <= 0) handleError("read data size", return res);
    char* bufferData = new char[dataSize + 1];
    if (dataSize > 0) {
        res = readn(read_fd, bufferData, dataSize);
        if (res <= 0) handleError("read data", return res);
    }
    
    bufferData[dataSize] = '\0';
    data.assign(bufferData, dataSize);
    delete[] bufferData;

    // recv optional
    uint32_t optionalSize;
    res = read(read_fd, &optionalSize, sizeof(optionalSize));
    if (res <= 0) handleError("read optional data size", return res);
    char* bufferOptData = new char[optionalSize + 1];
    if (optionalSize > 0) {
        res = readn(read_fd, bufferOptData, optionalSize);
        if (res <= 0) handleError("read optional data", return res);
    }
    
    bufferOptData[optionalSize] = '\0';
    optional.assign(bufferOptData, optionalSize);
    delete[] bufferOptData;

    // send ack
    char ack = '#'; 
    res = write(send_fd, &ack, sizeof(ack)); 
    if (res == -1) handleError("error sending ACK", return res);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

void process_body(int read_fd, int send_fd) {
    // Load pickling/unpickling functions
    pickling pickl;

    auto cleanup_exit = [&](){
        LOG("[child] cleanup");
        // Cleanup of objects created
        close(send_fd);
        close(read_fd);
        pickl.~pickling();
        Py_Finalize();
        exit(0);
    };

    CHECK_ERROR_THEN("[child] load pickle/unpickle failure: ", cleanup_exit();)

    std::string data;
    std::string f_name;

    // receive serialized node
    int err = receiveMessage(read_fd, send_fd, data);
    if (err <= 0) handleError("[child] read serialized node", cleanup_exit());
    LOG("[child] read serialized node");

    // deserialize Python node
    auto node = pickl.unpickle(data);
    CHECK_ERROR_THEN("[child] unpickle node failure: ", cleanup_exit();)
    LOG("[child] deserialized node");

    while(err > 0) {
        err = receiveMessage(read_fd, send_fd, data, f_name);
        if (err < 0) handleError("[child] read next", cleanup_exit());

        if (err > 0) {
            // deserialize data
            auto py_args_tuple = pickl.unpickle(data);
            CHECK_ERROR_THEN("[child] deserialize data failure: ", cleanup_exit();)
            // call function
            PyObject *py_func = PyObject_GetAttrString(node, f_name.c_str());
            CHECK_ERROR_THEN("[child] get node function: ", cleanup_exit();)

            if (py_func) {
                // finally call the function
                PyObject* py_result = PyTuple_Check(py_args_tuple) == 1 ? PyObject_CallObject(py_func, py_args_tuple):PyObject_CallFunctionObjArgs(py_func, py_args_tuple, nullptr);
                CHECK_ERROR_THEN("[child] call function failure: ", cleanup_exit();)

                // serialize response
                auto result_str = pickl.pickle(py_result);
                CHECK_ERROR_THEN("[child] pickle result failure: ", cleanup_exit();)

                // send response
                err = sendMessage(read_fd, send_fd, result_str);
                if (err <= 0) handleError("[child] send response", cleanup_exit());
                LOG("[child] sent response");

                Py_DECREF(py_result);
                Py_DECREF(py_func);
            }
        }
    }

    LOG("[child] after while");

    cleanup_exit();
}

#define EMPTY_TUPLE_STR "(t."

class py_ff_node_process: public ff::ff_node {
public:
    /* Inherit the constructors */
    using ff_node::ff_node;

    py_ff_node_process(PyObject* node): node(node), send_fd(-1), read_fd(-1) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        Py_INCREF(node);
    }
    
    int svc_init() override {
        auto svc_init_start_time = std::chrono::system_clock::now();
        // associate a new thread state with ff_node's thread
        PyThreadState* cached_tstate = tstate;
        tstate = PyThreadState_New(cached_tstate->interp);

        // Hold the main GIL
        PyEval_RestoreThread(tstate);

        has_svc_init = PyObject_HasAttrString(node, "svc_init") == 1;
        has_svc_end = PyObject_HasAttrString(node, "svc_end") == 1;
        
        // Load pickling/unpickling functions
        pickling pickl;
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return -1;)

        // serialize Python node to string
        auto node_str = pickl.pickle(node);
        CHECK_ERROR_THEN("pickle node failure: ", return -1;)

        none_str = std::string(pickl.pickle(Py_None));
        CHECK_ERROR_THEN("pickle None failure: ", return -1;)

        int returnValue = 0;

        int mainToChildFD[2]; // data to be sent from main process to child process
        int childToMainFD[2]; // data to be sent from child process to main process
        // create pipes
        if (pipe(mainToChildFD) == -1) {
            perror("pipe mainToChildFD");
            returnValue = -1;
        }
        if (pipe(childToMainFD) == -1) {
            perror("pipe childToMainFD");
            returnValue = -1;
        }

        if (returnValue == 0) {
            // from cpython source code
            // https://github.com/python/cpython/blob/main/Modules/posixmodule.c#L8056
            PyOS_BeforeFork();
            pid = fork();
            if (pid == -1) {
                perror("fork failed");
                returnValue = -1;
            } else if (pid == 0) { // child
                PyOS_AfterFork_Child();

                close(mainToChildFD[1]); // Close write end of mainToChildFD
                close(childToMainFD[0]); // Close read end of childToMainFD

                process_body(mainToChildFD[0], childToMainFD[1]);
                perror("[child] shouldn't be here...");
                returnValue = -1;
            } else { // parent
                PyOS_AfterFork_Parent();

                // Release the main GIL
                PyEval_SaveThread();

                close(mainToChildFD[0]); // Close read end of mainToChildFD
                close(childToMainFD[1]); // Close write end of childToMainFD

                send_fd = mainToChildFD[1];
                read_fd = childToMainFD[0];

                int err = sendMessage(read_fd, send_fd, node_str);
                if (err <= 0) handleError("send serialized node", returnValue = -1);

                if (err > 0 && has_svc_init) {
                    auto empty_tuple = std::string(EMPTY_TUPLE_STR);
                    std::string response;
                    int err = remote_function_call(empty_tuple, "svc_init", response);
                    if (err <= 0) {
                        handleError("read result of remote call of svc_init", );
                    } else {
                        // Hold the main GIL
                        PyEval_RestoreThread(tstate);
                        PyObject *svc_init_result = pickl.unpickle(response);
                        CHECK_ERROR_THEN("unpickle svc_init_result failure: ", returnValue = -1;)
                        returnValue = 0;
                        // if we are here, then the GIL was acquired before
                        if (PyLong_Check(svc_init_result)) {
                            long res_as_long = PyLong_AsLong(svc_init_result);
                            returnValue = static_cast<int>(res_as_long);
                        }
                        Py_DECREF(svc_init_result);
                        pickl.~pickling();
                        // Release the main GIL
                        PyEval_SaveThread();
                    }
                }
            }
        }

        // from here the GIL is NOT acquired
        auto svc_init_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_init_start_time).count();
        LOG("svc_init time " << svc_init_time_ms << "ms");
        return returnValue;
    }

    void * svc(void *arg) override {
        auto svc_start_time = std::chrono::system_clock::now(); 
        std::string serialized_data = arg == NULL ? EMPTY_TUPLE_STR:*reinterpret_cast<std::string*>(arg);
        
        std::string response;
        int err = remote_function_call(serialized_data, "svc", response);
        if (err <= 0) handleError("remote call of svc", );
        else {
            auto svc_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_start_time).count();
            LOG("serialized size = " << serialized_data->size() << ", svc time " << svc_time_ms << "ms");
            if (response == none_str) return NULL;

            return new std::string(response);
        }

        LOG("an error occurred, abort.");

        return NULL;
    }

    void cleanup() {
        // Cleanup of objects created
        Py_DECREF(node);
        node = nullptr;
        tstate = nullptr;
        PyEval_SaveThread();

        if (send_fd > 0) close(send_fd);
        send_fd = -1;
        if (read_fd > 0) close(read_fd);
        send_fd = -1;
    }

    int remote_function_call(std::string &data, std::string f_name, std::string &response) {
        if (send_fd == -1 || read_fd == -1) {
            // they are -1 if an error occurred during svc_init or svc
            return -1;
        }
        
        int err = sendMessage(read_fd, send_fd, data, f_name);
        if (err <= 0) return err;

        return receiveMessage(read_fd, send_fd, response);
    }

    void svc_end() override {
        auto svc_end_start_time = std::chrono::system_clock::now();

        if (has_svc_end) {
            std::string response;
            auto empty_tuple = std::string(EMPTY_TUPLE_STR);
            int err = remote_function_call(empty_tuple, "svc_end", response);
            if (err <= 0) handleError("read result of remote call of svc_end", );
        }

        // close the pipes, so the process can stop meanwhile we acquire the GIL and cleanup everything
        if (send_fd > 0) close(send_fd);
        send_fd = -1;
        if (read_fd > 0) close(read_fd);
        send_fd = -1;

        waitpid(pid, nullptr, 0);

        // Acquire the main GIL
        PyEval_RestoreThread(tstate);
        cleanup();

        auto svc_end_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_end_start_time).count();
        LOG("svc_end time " << svc_end_time_ms << "ms");
    }

private:
    PyThreadState *tstate;
    PyObject* node;
    bool has_svc_init;
    bool has_svc_end;
    int send_fd;
    int read_fd;
    pid_t pid;
    std::string none_str;
};

#endif // PY_FF_NODE_PROCESS
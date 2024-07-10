#ifndef PY_FF_NODE_PROCESS
#define PY_FF_NODE_PROCESS

#include <Python.h>
#include <ff/ff.hpp>
#include <iostream>
#include <sys/wait.h>
#include "error_macros.hpp"
#include "pickle.hpp"

#define DEBUG 1

#ifdef DEBUG
#    define LOG(msg) std::cerr << msg << std::endl
#else
#    define LOG(msg) do { } while(0);
#endif

#define MESSAGE_TYPE_RESPONSE '1'
#define MESSAGE_TYPE_REMOTE_FUNCTION_CALL '2'
#define MESSAGE_TYPE_ACK '3'

struct Message {
    unsigned char type;
    std::string data;
    std::string f_name;
};

#define handleError(msg, then) do { if (errno < 0) { perror(msg); } else { LOG(msg": errno == 0, fd closed\n"); } then; } while(0)

int sendMessage(int read_fd, int send_fd, const Message& message) {
    // send type
    if (write(send_fd, &message.type, sizeof(message.type)) == -1) {
        handleError("write type", return -1);
    }

    // send data
    uint32_t dataSize = message.data.size();
    if (write(send_fd, &dataSize, sizeof(dataSize)) == -1) {
        handleError("write data size", return -1);
    }
    if (dataSize > 0 && write(send_fd, message.data.c_str(), dataSize) == -1) {
        handleError("write data", return -1);
    }

    // send f_name
    uint32_t fnameSize = message.f_name.size();
    if (write(send_fd, &fnameSize, sizeof(fnameSize)) == -1) {
        handleError("write f_name size", return -1);
    }

    if (fnameSize > 0 && write(send_fd, message.f_name.c_str(), fnameSize) == -1) {
        handleError("write f_name", return -1);
    }

    int dummy; 
    int err = read(read_fd, &dummy, sizeof(dummy)); 
    if (err <= 0) handleError("error receiving ACK", return err);

    return 1; // 0 = EOF, -1 = ERROR, 1 = SUCCESS
}

int receiveMessage(int read_fd, int send_fd, Message& message) {
    // recv type
    int res = read(read_fd, (void*) &(message.type), sizeof(message.type));
    if (res <= 0) handleError("read type", return res);

    // recv data
    uint32_t dataSize;
    res = read(read_fd, &dataSize, sizeof(dataSize));
    if (res <= 0) handleError("read data size", return res);

    char* bufferData = new char[dataSize + 1];
    if (dataSize > 0) {
        res = read(read_fd, bufferData, dataSize);
        if (res <= 0) handleError("read data", return res);
    }
    
    bufferData[dataSize] = '\0';
    message.data = std::string(bufferData);
    delete[] bufferData;

    // recv f_name
    uint32_t fnameSize;
    res = read(read_fd, &fnameSize, sizeof(fnameSize));
    if (res <= 0) handleError("read fname size", return res);

    char* bufferFname = new char[fnameSize + 1];
    if (fnameSize > 0) {
        res = read(read_fd, bufferFname, fnameSize);
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

void process_body(int read_fd, int send_fd) {
    Message message;
    
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

    // receive serialized node
    int err = receiveMessage(read_fd, send_fd, message);
    if (err <= 0) handleError("[child] read serialized node", cleanup_exit());
    LOG("[child] read serialized node");

    // deserialize Python node
    auto node = pickl.unpickle(message.data);
    CHECK_ERROR_THEN("[child] unpickle node failure: ", cleanup_exit();)
    LOG("[child] deserialized node");
    
    while(err > 0) {
        err = receiveMessage(read_fd, send_fd, message);
        if (err < 0) handleError("[child] read next", cleanup_exit());

        if (err > 0) {
            // deserialize data
            auto py_args_tuple = pickl.unpickle(message.data);
            CHECK_ERROR_THEN("[child] deserialize data failure: ", cleanup_exit();)

            // call function
            PyObject *py_func = PyObject_GetAttrString(node, message.f_name.c_str());
            CHECK_ERROR_THEN("[child] get node function: ", cleanup_exit();)

            if (py_func) {
                // finally call the function
                PyObject* py_result = PyTuple_Check(py_args_tuple) == 1 ? PyObject_CallObject(py_func, py_args_tuple):PyObject_CallFunctionObjArgs(py_func, py_args_tuple, nullptr);
                CHECK_ERROR_THEN("[child] call function failure: ", cleanup_exit();)

                // serialize response
                auto result_str = pickl.pickle(py_result, 0);
                CHECK_ERROR_THEN("[child] pickle result failure: ", cleanup_exit();)

                // send response
                err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_RESPONSE, .data = result_str, .f_name = "" });
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

class py_ff_node_process: public ff::ff_node {
public:
    /* Inherit the constructors */
    using ff_node::ff_node;

    py_ff_node_process(PyObject* node): node(node), send_fd(-1), read_fd(-1) {
        // initialize the thread state with main thread state
        tstate = PyThreadState_Get();
        Py_INCREF(node);
        CHECK_ERROR("py_ff_node_process failure: ")
    }
    
    int svc_init() override {
        auto svc_init_start_time = std::chrono::system_clock::now();
        // associate a new thread state with ff_node's thread
        PyThreadState* cached_tstate = tstate;
        tstate = PyThreadState_New(cached_tstate->interp);

        // Hold the main GIL
        PyEval_RestoreThread(tstate);

        // Load pickling/unpickling functions
        pickling pickl;
        CHECK_ERROR_THEN("load pickle and unpickle failure: ", return -1;)

        // serialize Python node to string
        auto node_str = pickl.pickle(node, 0);
        CHECK_ERROR_THEN("pickle node failure: ", return -1;)

        PyObject* empty_tuple = PyTuple_New(0);
        empty_tuple_str = std::string(pickl.pickle(empty_tuple));
        CHECK_ERROR_THEN("pickle empty tuple failure: ", return -1;)
        Py_DECREF(empty_tuple);

        none_str = std::string(pickl.pickle(Py_None, 0));
        CHECK_ERROR_THEN("pickle None failure: ", return -1;)

        pickl.~pickling();
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

                int err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_REMOTE_FUNCTION_CALL, .data = node_str, .f_name = "" });
                if (err <= 0) handleError("send serialized node", returnValue = -1);

                if (err > 0) {
                    err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_REMOTE_FUNCTION_CALL, .data = empty_tuple_str, .f_name = "svc_init" });
                    if (err <= 0) handleError("remote svc_init call", returnValue = -1);
                    else if (err > 0) {
                        Message message;
                        err = receiveMessage(read_fd, send_fd, message);
                        if (err <= 0) handleError("read result of remote svc_init call", returnValue = -1);
                        else {
                            std::cout << "TODO: parse svc_init response" << std::endl;
                            returnValue = 0;
                        }
                    }
                }
            }
        }

        if (returnValue != 0) {
            // Acquire the main GIL
            PyEval_RestoreThread(tstate);
            cleanup();
        }

        auto svc_init_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_init_start_time).count();
        std::cerr << "svc_init time " << svc_init_time_ms << "ms" << std::endl;
        return returnValue;
    }

    void * svc(void *arg) override {
        auto svc_start_time = std::chrono::system_clock::now(); 
        std::string* serialized_data = arg == NULL ? &empty_tuple_str:reinterpret_cast<std::string*>(arg);
        
        int err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_REMOTE_FUNCTION_CALL, .data = *serialized_data, .f_name = "svc" });
        //todo if (arg != NULL) free(serialized_data);
        if (err <= 0) {
            perror("svc send serialized data");
            return NULL;
        }

        Message response;
        err = receiveMessage(read_fd, send_fd, response);
        if (err <= 0) handleError("read result of remote svc call", );
        else {
            auto svc_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_start_time).count();
            std::cerr << "svc time " << svc_time_ms << "ms" << std::endl;
            if (response.data == none_str) return NULL;

            return new std::string(response.data);
        }

        std::cerr << "an error occurred, abort." << std::endl;

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

    void svc_end() override {
        auto svc_end_start_time = std::chrono::system_clock::now();

        if (send_fd > 0 && read_fd > 0) { // they are -1 if an error occurred during svc_init or svc
            int err = sendMessage(read_fd, send_fd, { .type = MESSAGE_TYPE_REMOTE_FUNCTION_CALL, .data = empty_tuple_str, .f_name = "svc_end" });
            if (err <= 0) perror("remote call of svc_end");
            else if (err > 0) {
                Message message;
                err = receiveMessage(read_fd, send_fd, message);
                if (err <= 0) handleError("read result of remote call of svc_end", );
            }
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
        //waitpid(pid, nullptr, 0);

        auto svc_end_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - svc_end_start_time).count();
        std::cerr << "svc_end time " << svc_end_time_ms << "ms" << std::endl;
    }

private:
    PyThreadState *tstate;
    PyObject* node;
    int send_fd;
    int read_fd;
    pid_t pid;
    std::string empty_tuple_str;
    std::string none_str;
};

#endif // PY_FF_NODE_PROCESS
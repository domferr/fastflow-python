#include <iostream>
#include <ff/ff.hpp>

// Stage 1: Generate numbers from 1 to 10
struct NumberGenerator : ff::ff_node_t<int> {
    int* svc(int*) {
        for (int i = 1; i <= 10; ++i) {
            ff_send_out(new int(i));  // Send numbers 1 to 10
        }
        return (int*) ff::FF_EOS;  // End of stream
    }
};

// Stage 2: Square each number
struct SquareNode : ff::ff_node_t<int> {
    int* svc(int* number) {
        int* result = new int((*number) * (*number));
        delete number;
        return result;
    }
};

// Stage 3: Subtract 5 from each squared number
struct SubtractFiveNode : ff::ff_node_t<int> {
    int* svc(int* squared_number) {
        int* result = new int((*squared_number) - 5);
        delete squared_number;
        return result;
    }
};

// Build the pipeline and add the stages
auto pipe = new ff::ff_pipeline();
pipe->add_stage(new NumberGenerator());
pipe->add_stage(new SquareNode());
pipe->add_stage(new SubtractFiveNode());

// Run the pipeline
if (pipe->run_and_wait_end() < 0) {
    /* handle error... */
    return -1;
}
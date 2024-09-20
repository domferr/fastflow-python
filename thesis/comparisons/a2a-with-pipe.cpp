#include <ff/ff.hpp>
#include <ff/multinode.hpp>
#include <iostream>

struct Source: ff::ff_node_t<std::vector<long>> {
  long counter;
  long id;
  Source(const long id): id(id), counter(1) {}
  
  std::vector<long>* svc(std::vector<long>*) {
    if (counter > 5) return EOS;
    counter++;

    auto* lis = new std::vector<long>();
    lis->push_back(id);
    return lis;
	}
};

struct Stage: ff::ff_node_t<std::vector<long>> {
  long id;
  Stage(const long id): id(id) {}

  std::vector<long>* svc(std::vector<long>* arg) {
    arg->push_back(id);
    return arg;
  }
};

struct MOStage: ff::ff_monode_t<std::vector<long>> {
  long id;
	MOStage(const long id): id(id) {}

	std::vector<long>* svc(std::vector<long>* arg) {
    arg->push_back(id);
    return arg;
	}
};

struct Sink: ff::ff_minode_t<std::vector<long>> {
  long id;
  Sink(const long id): id(id) {}

  std::vector<long>* svc(std::vector<long>* arg) {
    arg->push_back(id);
    return nullptr;
  }
};

int main() {
  // initialize all-to-all
  ff::ff_a2a a2a;

  // create the first set
  std::vector<ff::ff_node*> firstset;
  for(long i=1; i<=4; i++) {
    // create a pipeline and add stages
    auto pipe = new ff::ff_pipeline();
    pipe->add_stage(new Source((i*10)+1));
    pipe->add_stage(new Stage((i*10)+2));
    pipe->add_stage(new MOStage((i*10)+3));
    // add pipeline to first set
    firstset.push_back(pipe);
  }

  // create the second set
  std::vector<ff::ff_node*> secondset;
  // add nodes to the second set
  secondset.push_back(new Sink(1));
  secondset.push_back(new Sink(2));
  secondset.push_back(new Sink(3));

  // add first and second sets to the all-to-all
  a2a.add_firstset(firstset, 0, true);
  a2a.add_secondset(secondset, true);

  // run the all-to-all and wait for the end
  a2a.run_and_wait_end();

  return 0;
}
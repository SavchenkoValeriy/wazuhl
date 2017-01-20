#include "llvm/Wazuhl/DQN.h"
#include "llvm/Wazuhl/Config.h"
#include "llvm/Support/raw_ostream.h"

#include <caffe/caffe.hpp>

namespace llvm {
namespace wazuhl {
  DQNCore::ResultsVector DQNCore::calculate(const DQN::State &S) const {
    //caffe::Caffe::set_mode(caffe::Caffe::GPU);
    //caffe::Net<double> NN{config::getCaffeModelPath(), caffe::TEST};
    return { 0 };
  }

  void DQNCore::update(const DQNCore::State &S,
                       const DQNCore::Action &A,
                       DQNCore::Result value) {
    //    llvm::errs() << "update was called for Action {" <<
    //      A.getName() << "} and Value{" << value << "}\n";
    // TODO: implement the following steps
    // 1. add (S, A, value) to experience replay
    // 2. randomly pick a minibatch of triplets (S, A, value)
    //    out of experience replay
    // 3. do one step of SGD towards the minibatch by L_2 measure
  }
}
}

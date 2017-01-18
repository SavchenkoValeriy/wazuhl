#include "llvm/Wazuhl/DQN.h"
#include <caffe/caffe.hpp>

namespace llvm {
namespace wazuhl {
  class DQNImpl {
  public:
    DQN::Result calculate(const DQN::State &S, const DQN::Action &A) {
      caffe::Caffe::set_mode(caffe::Caffe::GPU);
      caffe::Net<double> NN{"model_file", caffe::TEST};
      return 0.0;
    }
  };
}
}

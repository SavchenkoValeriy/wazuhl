#include "llvm/Wazuhl/DQN.h"
#include "llvm/Wazuhl/Config.h"
#include "llvm/Support/raw_ostream.h"

#include <caffe/caffe.hpp>

namespace llvm {
namespace wazuhl {
  using Action        = DQNCore::Action;
  using Result        = DQNCore::Result;
  using State         = DQNCore::State;
  using ResultsVector = DQNCore::ResultsVector;

  class DQNCoreImpl {
  public:
    DQNCoreImpl() { initialize(); }
    ResultsVector calculate(const DQN::State &S) const;
    void update(const State &S, const Action &A, Result value);
  private:
    void addToExperience(const State &S,
                         const Action &A, Result value);
    void experienceUpdate();
    void initialize();
    void initializeSolver();
    void initializeNets();

    using Net = std::unique_ptr<caffe::Net<Result> >;
    using SolverTy = std::unique_ptr<caffe::Solver<Result> >;

    Net LearningNet;
    Net CalculatingNet;
    SolverTy Solver;
  };


  ResultsVector DQNCore::calculate(const DQN::State &S) const {
    return pImpl->calculate(S);
  }

  void DQNCore::update(const State &S,
                       const Action &A, Result value) {
    //    llvm::errs() << "update was called for Action {" <<
    //      A.getName() << "} and Value{" << value << "}\n";
    // TODO: implement the following steps
    // 1. add (S, A, value) to experience replay
    // 2. randomly pick a minibatch of triplets (S, A, value)
    //    out of experience replay
    // 3. do one step of SGD towards the minibatch by L_2 measure
    pImpl->update(S, A, value);
  }

  DQNCore::DQNCore()  = default;
  DQNCore::~DQNCore() = default;

  ResultsVector DQNCoreImpl::calculate(const State &S) const {
    return { 0 };
  }

  void DQNCoreImpl::update(const State &S,
                           const Action &A, Result value) {
    addToExperience(S, A, value);
    experienceUpdate();
  }

  void DQNCoreImpl::addToExperience(const State &S,
                                    const Action &A,
                                    Result value) {
    //TODO: implement experience replay
  }

  void DQNCoreImpl::experienceUpdate() {
    //TODO: implement experience replay
  }

  void DQNCoreImpl::initialize() {
    caffe::Caffe::set_mode(caffe::Caffe::CPU);
    caffe::Net<double> NN{config::getCaffeModelPath(), caffe::TRAIN};
  }

  void DQNCoreImpl::initializeSolver() {
    caffe::SolverParameter SolverParam;
    caffe::ReadProtoFromTextFileOrDie(config::getCaffeSolverPath(),
                                      &SolverParam);

    Solver.reset(caffe::SolverRegistry<Result>::CreateSolver(SolverParam));
  }
}
}

#include "llvm/Wazuhl/DQN.h"
#include "llvm/Wazuhl/Config.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <caffe/caffe.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <unistd.h>

namespace {
  class GoToDirectory {
  public:
    GoToDirectory(llvm::StringRef path) {
      llvm::sys::fs::current_path(ToReturn);
      cd(path);
    }
    ~GoToDirectory() {
      cd(ToReturn);
    }
  private:
    void cd(llvm::StringRef path) {
      auto ret = chdir(path.str().c_str());
      if (ret != 0)
        llvm_unreachable("Moving to a non-existent directory");
    }
    llvm::SmallString<120> ToReturn;
  };

  template< class T, class U >
  boost::shared_ptr<T> pointer_cast(const boost::shared_ptr<U>& r) noexcept
  {
    auto p = (typename boost::shared_ptr<T>::element_type*)(r.get());
    return boost::shared_ptr<T>(r, p);
  }
}

namespace llvm {
namespace wazuhl {
  using Action        = DQNCore::Action;
  using Result        = DQNCore::Result;
  using State         = DQNCore::State;
  using ResultsVector = DQNCore::ResultsVector;

  // TODO: Make a unified place for this
  constexpr unsigned MinibatchSize = 32;
  llvm::SmallVector<Result, MinibatchSize> TrainDummy(MinibatchSize);
  llvm::SmallVector<Result, 1> TestDummy(1);

  class DQNCoreImpl {
  public:
    DQNCoreImpl() { initialize(); }
    ~DQNCoreImpl();
    ResultsVector calculate(const DQN::State &S) const;
    void update(const State &S, const Action &A, Result value);
  private:
    void addToExperience(const State &S,
                         const Action &A, Result value);
    void experienceUpdate();

    inline void initialize();
    inline void initializeSolver();
    inline void initializeNets();
    inline void initializeInputs();
    inline void initializeOutputs();

    inline void loadTrainedNet();
    inline void saveTrainedNet();

    using Net = caffe::Net<Result>;
    using NetU = std::unique_ptr<Net>;
    // solver stores boost::shared_ptr,
    // and it's not convertible to std::shared_ptr
    using NetS = boost::shared_ptr<Net>;
    using SolverTy = std::unique_ptr<caffe::Solver<Result> >;
    using InputLayer = caffe::MemoryDataLayer<Result>;
    using InputLayerS = boost::shared_ptr<InputLayer>;

    // learning net is shared with the solver
    NetS LearningNet;
    NetU CalculatingNet;
    InputLayerS LearningNetInput;
    InputLayerS LearningNetExpected;
    InputLayerS CalculatingNetInput;
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

  DQNCore::DQNCore() : pImpl(llvm::make_unique<DQNCoreImpl>()) {}
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

  inline void DQNCoreImpl::initialize() {
    caffe::Caffe::set_mode(caffe::Caffe::CPU);
    initializeSolver();
    initializeNets();
    initializeInputs();
  }

  inline void DQNCoreImpl::initializeSolver() {
    caffe::SolverParameter SolverParam;

    // caffe looks for 'net' from solver.prototxt not relatively to itself
    // but to a current directory, that's why
    // we're temporaly going to wazuhl's config directory
    GoToDirectory x{config::getWazuhlConfigPath()};

    caffe::ReadProtoFromTextFileOrDie(config::getCaffeSolverPath(),
                                      &SolverParam);

    Solver.reset(caffe::SolverRegistry<Result>::CreateSolver(SolverParam));
  }

  inline void DQNCoreImpl::initializeNets() {
    LearningNet = Solver->net();
    loadTrainedNet();
    CalculatingNet = llvm::make_unique<Net>(config::getCaffeModelPath(),
                                            caffe::TEST);
    CalculatingNet->ShareTrainedLayersWith(LearningNet.get());
  }

  inline void DQNCoreImpl::initializeInputs() {
    CalculatingNetInput = pointer_cast<InputLayer>(
          CalculatingNet->layer_by_name("live_input"));
    LearningNetInput = pointer_cast<InputLayer>(
          LearningNet->layer_by_name("experience_replay_state"));
    LearningNetExpected = pointer_cast<InputLayer>(
          LearningNet->layer_by_name("experience_replay_action"));
  }

  inline void DQNCoreImpl::loadTrainedNet() {
    auto SavedNet = config::getTrainedNetFile();
    if (sys::fs::exists(SavedNet))
      LearningNet->CopyTrainedLayersFrom(SavedNet);
  }

  inline void DQNCoreImpl::saveTrainedNet() {
    caffe::NetParameter net_param;
    LearningNet->ToProto(&net_param, false);
    caffe::WriteProtoToBinaryFile(net_param, config::getTrainedNetFile());
  }

  DQNCoreImpl::~DQNCoreImpl() {
    saveTrainedNet();
  }
}
}

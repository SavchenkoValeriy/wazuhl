#include "llvm/Wazuhl/DQN.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Wazuhl/Config.h"
#include "llvm/Wazuhl/ExperienceReplay.h"

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
  ~GoToDirectory() { cd(ToReturn); }

private:
  void cd(llvm::StringRef path) {
    auto ret = chdir(path.str().c_str());
    if (ret != 0)
      llvm_unreachable("Moving to a non-existent directory");
  }
  llvm::SmallString<120> ToReturn;
};
} // namespace

namespace llvm {
namespace wazuhl {
using Action = DQNCore::Action;
using Result = DQNCore::Result;
using State = DQNCore::State;
using ResultsVector = DQNCore::ResultsVector;

SmallVector<Result, config::MinibatchSize> TrainDummy(config::MinibatchSize);
SmallVector<Result, 1> TestDummy(1);
constexpr auto ContextTrainBlobSize =
    config::MinibatchSize * config::ContextSize;
SmallVector<Result, ContextTrainBlobSize>
    ContextTrainDummy(ContextTrainBlobSize);
SmallVector<Result, config::ContextSize> ContextTestDummy(config::ContextSize);
SmallVector<Result, ContextTrainBlobSize> ClipTrainDummy(ContextTrainBlobSize,
                                                         1.0);
SmallVector<Result, config::ContextSize> ClipTestDummy(config::ContextSize,
                                                       1.0);

class DQNCoreImpl {
public:
  DQNCoreImpl() { initialize(); }
  ~DQNCoreImpl();
  ResultsVector calculate(const DQN::State &S) const;
  void update(const State &S, const Action &A, Result value);

private:
  void addToExperience(const State &S, const Action &A, Result value);
  void experienceUpdate();

  inline void loadData(ExperienceReplay::RecalledExperience &Chunk);

  inline void initialize();
  inline void initializeSolver();
  inline void initializeNets();
  inline void initializeInputs();
  inline void initializeOutputs();

  inline void lazyInitializeExperience();

  inline void loadTrainedNet();
  inline void saveTrainedNet();

  inline ResultsVector getResultsVector() const;

  using Blob = caffe::Blob<Result>;
  using Net = caffe::Net<Result>;
  using BlobS = boost::shared_ptr<Blob>;
  using NetU = std::unique_ptr<Net>;
  // solver stores boost::shared_ptr,
  // and it's not convertible to std::shared_ptr
  using NetS = boost::shared_ptr<Net>;
  using SolverTy = std::unique_ptr<caffe::Solver<Result>>;
  using InputLayer = caffe::MemoryDataLayer<Result>;
  using InputLayerS = boost::shared_ptr<InputLayer>;
  using ExperienceReplayPtr = std::unique_ptr<ExperienceReplay>;

  // learning net is shared with the solver
  NetS LearningNet;
  NetU CalculatingNet;
  InputLayerS LearningNetInputState;
  InputLayerS LearningNetInputAction;
  InputLayerS LearningNetInputContext;
  InputLayerS LearningNetInputClip;
  InputLayerS LearningNetExpected;
  InputLayerS CalculatingNetInput;
  InputLayerS CalculatingNetInputContext;
  InputLayerS CalculatingNetInputClip;
  SolverTy Solver;
  BlobS Output, Loss;

  mutable State LastState;
  mutable ResultsVector LastResultsVector;

  // experience is needed only for learning
  ExperienceReplayPtr Experience = nullptr;
};

ResultsVector DQNCore::calculate(const DQN::State &S) const {
  return pImpl->calculate(S);
}

void DQNCore::update(const State &S, const Action &A, Result value) {
  pImpl->update(S, A, value);
}

DQNCore::DQNCore() : pImpl(llvm::make_unique<DQNCoreImpl>()) {}
DQNCore::~DQNCore() = default;

ResultsVector DQNCoreImpl::calculate(const State &S) const {
  if (LastState != S) {
    // caffe requires non-const array for MemoryData
    LastState = S;

    CalculatingNetInput->Reset(LastState.data(), TestDummy.data(),
                               config::NumberOfActions);
    SmallVector<Result, config::ContextSize> Context(config::ContextSize);

    unsigned i = 0, j = 0, N = LastState.getContext().size();
    for (j = std::max(i, N - config::ContextSize); j < N; ++j, ++i) {
      Context[i] = LastState.getContext()[j] + 1;
    }

    CalculatingNetInputContext->Reset(Context.data(), ContextTestDummy.data(),
                                      config::ContextSize);

    CalculatingNetInputClip->Reset(
        ClipTestDummy.data(), ContextTestDummy.data(), config::ContextSize);

    CalculatingNet->Forward();
    LastResultsVector = getResultsVector();
  }
  return LastResultsVector;
}

void DQNCoreImpl::update(const State &S, const Action &A, Result value) {
  // Connect to experience database only if we're trying to
  // use experience (remember or recall)
  lazyInitializeExperience();
  // 1. add (S, A, value) to experience replay
  addToExperience(S, A, value);
  // 2. randomly pick a minibatch of triplets (S, A, value)
  //    out of experience replay
  // 3. do one step of SGD towards the minibatch by L_2 measure
  experienceUpdate();
}

inline ResultsVector DQNCoreImpl::getResultsVector() const {
  ResultsVector result(config::NumberOfActions, 0.0);

  for (auto i : seq<unsigned>(0, config::NumberOfActions)) {
    result[i] = Output->data_at(0, i, 0, 0);
  }

  return result;
}

void DQNCoreImpl::addToExperience(const State &S, const Action &A,
                                  Result value) {
  auto TakenActionIndex = A.getIndex();
  llvm::errs() << "Remembering value " << value << " at index " << A.getIndex()
               << "\n";

  Experience->addToExperience({S, TakenActionIndex, value});
}

void DQNCoreImpl::experienceUpdate() {
  auto ChunkOfExperience = Experience->replay();
  if (ChunkOfExperience.empty()) {
    return;
  }
  loadData(ChunkOfExperience);
  Solver->Step(1);
  llvm::errs() << "Loss = " << Loss->data_at(0, 0, 0, 0) << "\n";
}

void DQNCoreImpl::loadData(ExperienceReplay::RecalledExperience &Chunk) {
  SmallVector<Result, config::MinibatchSize * config::NumberOfFeatures> States;

  constexpr unsigned SizeOfActionsBlob =
      config::MinibatchSize * config::NumberOfActions;
  SmallVector<Result, SizeOfActionsBlob> Actions(SizeOfActionsBlob, 0.0);
  SmallVector<Result, SizeOfActionsBlob> Outcomes(SizeOfActionsBlob, 0.0);

  constexpr unsigned SizeOfContextsBlob =
      config::MinibatchSize * config::ContextSize;
  SmallVector<Result, SizeOfContextsBlob> Contexts(SizeOfContextsBlob, 0.0);

  unsigned i = 0;
  for (auto &Element : Chunk) {
    States.append(Element.state.begin(), Element.state.end());
    Actions[i * config::NumberOfActions + Element.actionIndex] = 1;
    Outcomes[i * config::NumberOfActions + Element.actionIndex] = Element.value;
    unsigned j = 0, k = 0, N = Element.state.getContext().size();
    for (k = std::max(i, N - config::ContextSize); k < N; ++k, ++j) {
      Contexts[i * config::MinibatchSize + j] =
          Element.state.getContext()[k] + 1;
    }
  }

  LearningNetInputState->Reset(States.data(), TrainDummy.data(),
                               config::MinibatchSize);
  LearningNetInputAction->Reset(Actions.data(), TrainDummy.data(),
                                config::MinibatchSize);
  LearningNetInputContext->Reset(Contexts.data(), ContextTrainDummy.data(),
                                 SizeOfContextsBlob);
  LearningNetInputClip->Reset(ClipTrainDummy.data(), ContextTrainDummy.data(),
                              SizeOfContextsBlob);
  LearningNetExpected->Reset(Outcomes.data(), TrainDummy.data(),
                             config::MinibatchSize);
}

inline void DQNCoreImpl::initialize() {
  google::InitGoogleLogging("wazuhl");
  google::SetStderrLogging(google::GLOG_FATAL);
  caffe::Caffe::set_mode(caffe::Caffe::CPU);
  initializeSolver();
  initializeNets();
  initializeInputs();
  initializeOutputs();
}

inline void DQNCoreImpl::initializeSolver() {
  caffe::SolverParameter SolverParam;

  // caffe looks for 'net' from solver.prototxt not relatively to itself
  // but to a current directory, that's why
  // we're temporaly going to wazuhl's config directory
  GoToDirectory x{config::getWazuhlConfigPath()};

  caffe::ReadProtoFromTextFileOrDie(config::getCaffeSolverPath(), &SolverParam);

  Solver.reset(caffe::SolverRegistry<Result>::CreateSolver(SolverParam));
}

inline void DQNCoreImpl::initializeNets() {
  LearningNet = Solver->net();
  loadTrainedNet();
  CalculatingNet =
      llvm::make_unique<Net>(config::getCaffeModelPath(), caffe::TEST);
  CalculatingNet->ShareTrainedLayersWith(LearningNet.get());
}

inline void DQNCoreImpl::initializeInputs() {
  ClipTestDummy[0] = 0;
  for (auto i : seq<unsigned>(0, config::MinibatchSize)) {
    ClipTrainDummy[i * config::ContextSize] = 0;
  }

  CalculatingNetInput = boost::static_pointer_cast<InputLayer>(
      CalculatingNet->layer_by_name("live_input_state"));
  CalculatingNetInputContext = boost::static_pointer_cast<InputLayer>(
      CalculatingNet->layer_by_name("live_input_context"));
  CalculatingNetInputClip = boost::static_pointer_cast<InputLayer>(
      CalculatingNet->layer_by_name("live_input_clip"));

  LearningNetInputState = boost::static_pointer_cast<InputLayer>(
      LearningNet->layer_by_name("experience_replay_state"));
  LearningNetInputAction = boost::static_pointer_cast<InputLayer>(
      LearningNet->layer_by_name("experience_replay_action"));
  LearningNetInputContext = boost::static_pointer_cast<InputLayer>(
      LearningNet->layer_by_name("experience_replay_context"));
  LearningNetInputClip = boost::static_pointer_cast<InputLayer>(
      LearningNet->layer_by_name("experience_replay_clip"));
  LearningNetExpected = boost::static_pointer_cast<InputLayer>(
      LearningNet->layer_by_name("experience_replay_value"));
}

inline void DQNCoreImpl::initializeOutputs() {
  Output = CalculatingNet->blob_by_name("Q_values");
  Loss = LearningNet->blob_by_name("loss");
}

inline void DQNCoreImpl::lazyInitializeExperience() {
  if (Experience.get())
    return;
  Experience = llvm::make_unique<ExperienceReplay>();
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

DQNCoreImpl::~DQNCoreImpl() { saveTrainedNet(); }
} // namespace wazuhl
} // namespace llvm

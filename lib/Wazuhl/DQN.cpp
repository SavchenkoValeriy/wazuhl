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

llvm::SmallVector<Result, config::MinibatchSize>
    TrainDummy(config::MinibatchSize);
llvm::SmallVector<Result, 1> TestDummy(1);

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
  InputLayerS LearningNetInput;
  InputLayerS LearningNetExpected;
  InputLayerS CalculatingNetInput;
  SolverTy Solver;
  BlobS Output;

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
    CalculatingNetInput->Reset(LastState.data(), TestDummy.data(), 1);
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
  static auto NumberOfActions = Action::getAllPossibleActions().size();
  ResultsVector result(NumberOfActions, 0.0);

  for (auto i : seq<unsigned>(0, NumberOfActions)) {
    result[i] = Output->data_at(1 /* batch-size for CalculatingNet */, i, 0, 0);
  }

  return result;
}

void DQNCoreImpl::addToExperience(const State &S, const Action &A,
                                  Result value) {
  auto Values = calculate(S);
  auto TakenActionIndex = A.getIndex();
  Values[TakenActionIndex] = value;

  Experience->addToExperience({S, Values}, TakenActionIndex);
}

void DQNCoreImpl::experienceUpdate() {
  auto ChunkOfExperience = Experience->replay();
  loadData(ChunkOfExperience);
  Solver->Step(1);
}

void DQNCoreImpl::loadData(ExperienceReplay::RecalledExperience &Chunk) {
  SmallVector<double, config::MinibatchSize * config::NumberOfFeatures> States;
  SmallVector<double, config::MinibatchSize * config::NumberOfActions> Outcomes;

  for (auto &Element : Chunk) {
    States.append(Element.first.begin(), Element.first.end());
    Outcomes.append(Element.second.begin(), Element.second.end());
  }

  LearningNetInput->Reset(States.data(), TrainDummy.data(),
                          config::MinibatchSize);
  LearningNetExpected->Reset(Outcomes.data(), TrainDummy.data(),
                             config::MinibatchSize);
}

inline void DQNCoreImpl::initialize() {
  caffe::Caffe::set_mode(caffe::Caffe::CPU);
  google::SetStderrLogging(google::GLOG_FATAL);
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
  CalculatingNetInput = boost::static_pointer_cast<InputLayer>(
      CalculatingNet->layer_by_name("live_input"));
  LearningNetInput = boost::static_pointer_cast<InputLayer>(
      LearningNet->layer_by_name("experience_replay_state"));
  LearningNetExpected = boost::static_pointer_cast<InputLayer>(
      LearningNet->layer_by_name("experience_replay_action"));
}

inline void DQNCoreImpl::initializeOutputs() {
  Output = CalculatingNet->blob_by_name("Q_values");
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

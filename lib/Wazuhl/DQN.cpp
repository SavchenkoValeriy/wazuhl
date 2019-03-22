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
template <class T> using Batch = DQNCore::Batch<T>;
constexpr auto ContextTrainBlobSize =
    config::MinibatchSize * config::ContextSize;
SmallVector<Result, ContextTrainBlobSize> ClipTrainDummy(ContextTrainBlobSize,
                                                         1.0);
SmallVector<Result, config::ContextSize> ClipTestDummy(config::ContextSize,
                                                       1.0);

constexpr SmallVectorImpl<Result> &getClipDummy(unsigned size) {
  return size == 1 ? static_cast<SmallVectorImpl<Result> &>(ClipTestDummy)
                   : static_cast<SmallVectorImpl<Result> &>(ClipTrainDummy);
}

class DQNCoreImpl {
public:
  DQNCoreImpl() { initialize(); }
  ~DQNCoreImpl();
  ResultsVector calculate(const State &S) const;
  Result max(const State &S) const;
  Action argmax(const State &S) const;

  Batch<ResultsVector> calculate(const Batch<State> &S) const;
  Batch<Result> max(const Batch<State> &S) const;
  Batch<Action> argmax(const Batch<State> &S) const;

  void update(const State &S, const Action &A, Result value);
  void update(const Batch<State> &S, const Batch<Action> &A,
              const Batch<Result> value);

private:
  inline void initialize();
  inline void initializeSolver();
  inline void initializeNets();
  inline void initializeInputs();
  inline void initializeOutputs();

  inline void loadTrainedNet();
  inline void saveTrainedNet();

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

  template <unsigned Size, class NetT>
  inline void forward(const Batch<State> &S, NetT &NeuralNet) const;
  template <unsigned Size, class NetT>
  inline void loadInputs(const Batch<State> &S, NetT &NeuralNet) const;
  template <unsigned Size, class NetT>
  inline Batch<ResultsVector> calculateImpl(const Batch<State> &S,
                                            NetT &NeuralNet) const;
  template <unsigned Size, class NetT>
  inline Batch<Result> maxImpl(const Batch<State> &S, NetT &NeuralNet) const;
  template <unsigned Size, class NetT>
  inline Batch<Action> argmaxImpl(const Batch<State> &S, NetT &NeuralNet) const;

  template <unsigned Size> inline Batch<State> &getCache() const;

  template <class NetT>
  inline static Blob *getInput(NetT &NeuralNet, const std::string &name);
  template <class NetT>
  inline static Result *getInputArray(NetT &NeuralNet, const std::string &name);
  template <unsigned Size> static inline bool reshape(Blob *Input);
  template <unsigned Size> static inline bool reshapeRecurrentBlob(Blob *Input);

  static void initializeLogger();

  // learning net is shared with the solver
  NetS LearningNet;
  NetU CalculatingNet;
  SolverTy Solver;
  BlobS Output, Loss;

  mutable Batch<State> LastState, LastBatchOfStates;
  mutable ResultsVector LastResultsVector;
};

//===----------------------------------------------------------------------===//
//                        Original class implementation
//===----------------------------------------------------------------------===//

ResultsVector DQNCore::calculate(const DQN::State &S) const {
  return pImpl->calculate(S);
}
Result DQNCore::max(const DQN::State &S) const { return pImpl->max(S); }
Action DQNCore::argmax(const DQN::State &S) const { return pImpl->argmax(S); }

Batch<ResultsVector> DQNCore::calculate(const DQN::Batch<State> &S) const {
  return pImpl->calculate(S);
}
Batch<Result> DQNCore::max(const DQN::Batch<State> &S) const {
  return pImpl->max(S);
}
Batch<Action> DQNCore::argmax(const DQN::Batch<State> &S) const {
  return pImpl->argmax(S);
}

void DQNCore::update(const State &S, const Action &A, Result value) {
  pImpl->update(S, A, value);
}

void DQNCore::update(const Batch<State> &S, const Batch<Action> &A,
                     Batch<Result> value) {
  pImpl->update(S, A, value);
}

void DQNCore::copyWeightsFrom(const DQNCore &Source) {}

DQNCore::DQNCore() : pImpl(llvm::make_unique<DQNCoreImpl>()) {}
DQNCore::~DQNCore() = default;

//===----------------------------------------------------------------------===//
//                          pImpl's method defintions
//===----------------------------------------------------------------------===//

template <unsigned Size, class NetT>
inline void DQNCoreImpl::forward(const Batch<State> &S, NetT &NeuralNet) const {
  auto &Cache = getCache<Size>();

  // we just loaded and forwarded exactly this state!
  if (Cache == S) {
    return;
  }

  loadInputs<Size>(S, NeuralNet);
  NeuralNet->Forward();

  Cache = S;
}

template <unsigned Size, class NetT>
inline void DQNCoreImpl::loadInputs(const Batch<State> &S,
                                    NetT &NeuralNet) const {
  assert(Size == S.size() && "Wrong size of batch is given");

  auto StateInput = getInput(NeuralNet, "data");
  auto ContextInput = getInput(NeuralNet, "context");
  auto ClipInput = getInput(NeuralNet, "clip");

  bool NeedReshape = reshape<Size>(StateInput);
  NeedReshape |= reshapeRecurrentBlob<Size>(ContextInput);
  NeedReshape |= reshapeRecurrentBlob<Size>(ClipInput);

  if (NeedReshape) {
    NeuralNet->Reshape();
  }

  auto States = getInputArray(NeuralNet, "data");
  auto Contexts = getInputArray(NeuralNet, "context");

  for (auto i : seq<unsigned>(0, Size)) {
    auto &StateRef = S[i];
    for (auto j : seq<unsigned>(0, config::NumberOfFeatures)) {
      States[i * config::NumberOfFeatures + j] = StateRef[j];
    }

    for (auto j : seq<unsigned>(0, config::ContextSize)) {
      Contexts[j * Size + i] = 0;
    }

    unsigned j = 0, k = 0, N = StateRef.getContext().size();
    for (k = std::max<int>(j, N - config::ContextSize); k < N; ++k, ++j) {
      Contexts[j * Size + i] = StateRef.getContext()[k] + 1;
    }
  }

  ClipInput->set_cpu_data(getClipDummy(Size).data());
}

ResultsVector DQNCoreImpl::calculate(const State &S) const {
  Batch<State> Wrapper{S};
  return calculateImpl<1>(Wrapper, CalculatingNet)[0];
}

Result DQNCoreImpl::max(const State &S) const {
  Batch<State> Wrapper{S};
  return maxImpl<1>(Wrapper, CalculatingNet)[0];
}

Action DQNCoreImpl::argmax(const State &S) const {
  Batch<State> Wrapper{S};
  return argmaxImpl<1>(Wrapper, CalculatingNet)[0];
}

Batch<ResultsVector> DQNCoreImpl::calculate(const Batch<State> &S) const {
  return calculateImpl<config::MinibatchSize>(S, CalculatingNet);
}

Batch<Result> DQNCoreImpl::max(const Batch<State> &S) const {
  return maxImpl<config::MinibatchSize>(S, CalculatingNet);
}

Batch<Action> DQNCoreImpl::argmax(const Batch<State> &S) const {
  return argmaxImpl<config::MinibatchSize>(S, CalculatingNet);
}

template <unsigned Size, class NetT>
inline Batch<ResultsVector> DQNCoreImpl::calculateImpl(const Batch<State> &S,
                                                       NetT &NeuralNet) const {
  forward<Size>(S, NeuralNet);

  Batch<ResultsVector> result(Size,
                              ResultsVector(config::NumberOfActions, 0.0));

  for (auto i : seq<unsigned>(0, Size)) {
    for (auto j : seq<unsigned>(0, config::NumberOfActions)) {
      result[i][j] = NeuralNet->blob_by_name("Q_values")->data_at(i, j, 0, 0);
    }
  }

  return result;
}

template <unsigned Size, class NetT>
inline Batch<Result> DQNCoreImpl::maxImpl(const Batch<State> &S,
                                          NetT &NeuralNet) const {
  forward<Size>(S, NeuralNet);

  Batch<Result> Results;

  for (auto i : seq<unsigned>(0, Size)) {
    Results.push_back(NeuralNet->blob_by_name("argmax")->data_at(i, 1, 0, 0));
  }

  return Results;
}

template <unsigned Size, class NetT>
inline Batch<Action> DQNCoreImpl::argmaxImpl(const Batch<State> &S,
                                             NetT &NeuralNet) const {
  forward<Size>(S, NeuralNet);

  Batch<Action> Actions;

  for (auto i : seq<unsigned>(0, Size)) {
    auto index = NeuralNet->blob_by_name("argmax")->data_at(i, 0, 0, 0);
    Actions.push_back(Action::getActionByIndex(index));
  }

  return Actions;
}

template <> inline Batch<State> &DQNCoreImpl::getCache<1>() const {
  return LastState;
}

template <>
inline Batch<State> &DQNCoreImpl::getCache<config::MinibatchSize>() const {
  return LastBatchOfStates;
}

template <class NetT>
inline DQNCoreImpl::Blob *DQNCoreImpl::getInput(NetT &NeuralNet,
                                                const std::string &name) {
  return NeuralNet->blob_by_name(name).get();
}

template <class NetT>
inline Result *DQNCoreImpl::getInputArray(NetT &NeuralNet,
                                          const std::string &name) {
  return static_cast<Result *>(
      NeuralNet->blob_by_name(name)->mutable_cpu_data());
}

template <unsigned Size>
inline bool DQNCoreImpl::reshape(DQNCoreImpl::Blob *Input) {
  if (Input->shape(0) != Size) {
    Input->Reshape({Size, Input->shape(1)});
    return true;
  }
  return false;
}

template <unsigned Size>
inline bool DQNCoreImpl::reshapeRecurrentBlob(DQNCoreImpl::Blob *Input) {
  if (Input->shape(1) != Size) {
    Input->Reshape({Input->shape(0), Size});
    return true;
  }
  return false;
}

void DQNCoreImpl::update(const State &S, const Action &A, Result value) {
  // this implementation doesn't support non-minibatch update
}

void DQNCoreImpl::update(const Batch<State> &S, const Batch<Action> &A,
                         Batch<Result> V) {
  loadInputs<config::MinibatchSize>(S, LearningNet);

  auto Actions = getInputArray(LearningNet, "data_action");
  auto Outcomes = getInputArray(LearningNet, "value_input");

  for (auto i : seq<unsigned>(0, config::MinibatchSize)) {
    auto index = i * config::NumberOfActions + A[i].getIndex();
    Actions[index] = 1;
    Outcomes[index] = V[i];
  }

  Solver->Step(1);

  llvm::errs() << "Loss = " << Loss->data_at(0, 0, 0, 0) << "\n";
}

void DQNCoreImpl::initializeLogger() {
  static bool IsInitialized = false;
  if (!IsInitialized) {
    google::InitGoogleLogging("wazuhl");
    google::SetStderrLogging(google::GLOG_FATAL);
    IsInitialized = true;
  }
}

inline void DQNCoreImpl::initialize() {
  initializeLogger();
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
    ClipTrainDummy[i] = 0;
  }
}

inline void DQNCoreImpl::initializeOutputs() {
  Output = CalculatingNet->blob_by_name("Q_values");
  Loss = LearningNet->blob_by_name("loss");
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

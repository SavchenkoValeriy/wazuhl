#include "llvm/Wazuhl/DQN.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Wazuhl/Config.h"

#include <torch/torch.h>
#include <unistd.h>

using namespace llvm::wazuhl;

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

class Net : public torch::nn::Module {
public:
  Net() : torch::nn::Module() {
    ContextEmbedding = register_module(
        "embedding", torch::nn::Embedding(config::NumberOfActions + 1,
                                          config::ContextEmbeddingSize));

    ContextGRU =
        register_module("gru", torch::nn::GRU(config::ContextEmbeddingSize,
                                              config::ContextLSTMSize));

    StateNorm = register_module("state_norm",
                                torch::nn::BatchNorm(config::NumberOfFeatures));
    ContextNorm = register_module(
        "context_norm", torch::nn::BatchNorm(config::ContextLSTMSize));

    AdvantageHidden = register_module(
        "advantage_hidden",
        torch::nn::Linear(config::EncodedStateSize, config::ActionHiddenSize));
    AdvantageOutput = register_module(
        "advantage_output",
        torch::nn::Linear(config::ActionHiddenSize, config::NumberOfActions));
    ValueOutput = register_module(
        "value_output", torch::nn::Linear(config::EncodedStateSize, 1));

    unsigned InputSize = config::NumberOfFeatures, i = 0;
    for (auto Size : config::EncoderLayerSizes) {
      EncoderLayers.push_back(
          register_module(llvm::formatv("state_hidden_{0}", ++i),
                          torch::nn::Linear(InputSize, Size)));
      InputSize = Size;
    }
  }

  torch::Tensor forward(torch::Tensor state, torch::Tensor context) {
    context = ContextEmbedding(context);
    context = ContextGRU(context).output.select(0, config::ContextSize - 1);
    context = ContextNorm(context);

    state = StateNorm(state);
    for (auto &Linear : EncoderLayers) {
      state = torch::relu(Linear(state));
    }

    state = torch::cat({state, context}, 1);

    auto advantage = torch::relu(AdvantageHidden(state));
    advantage = torch::tanh(AdvantageOutput(advantage)) * 20;

    auto value = torch::tanh(ValueOutput(state)) * 20;
    return value + advantage - advantage.mean(1).unsqueeze(1);
  }

  void backward(torch::Tensor actual, torch::Tensor expected) {
    auto loss = torch::smooth_l1_loss(actual, expected);
    loss.backward();
    std::cout << "Loss: " << loss << std::endl;
  }

private:
  torch::nn::Embedding ContextEmbedding{nullptr};
  torch::nn::GRU ContextGRU{nullptr};
  torch::nn::BatchNorm StateNorm{nullptr}, ContextNorm{nullptr};
  torch::nn::Linear AdvantageHidden{nullptr}, AdvantageOutput{nullptr},
      ValueOutput{nullptr};
  static constexpr unsigned EncoderDepth = 5;
  llvm::SmallVector<torch::nn::Linear, EncoderDepth> EncoderLayers{};
};
} // namespace

namespace llvm {
namespace wazuhl {
using Action = DQNCore::Action;
using Result = DQNCore::Result;
using State = DQNCore::State;
using ResultsVector = DQNCore::ResultsVector;
template <class T> using Batch = DQNCore::Batch<T>;

class DQNCoreImpl {
public:
  DQNCoreImpl(StringRef ModelFile) : ModelFile(ModelFile) { initialize(); }
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

  inline void copyWeightsFrom(const DQNCoreImpl &);

private:
  inline void initialize();

  inline void loadNet();
  inline void saveNet() const;
  inline void loadNetFromFile(StringRef File);
  inline void saveNetToFile(StringRef File) const;

  template <unsigned Size>
  inline torch::Tensor forward(const Batch<State> &S) const;
  template <unsigned Size>
  inline Batch<ResultsVector> calculateImpl(const Batch<State> &S) const;
  template <unsigned Size>
  inline Batch<Result> maxImpl(const Batch<State> &S) const;
  template <unsigned Size>
  inline Batch<Action> argmaxImpl(const Batch<State> &S) const;

  template <unsigned Size> inline Batch<State> &getCache() const;

  StringRef ModelFile;
  mutable Batch<State> LastState, LastBatchOfStates;
  mutable torch::Tensor LastTensor;
  mutable Net Brain;
  mutable std::unique_ptr<torch::optim::Optimizer> Optimizer;
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

void DQNCore::copyWeightsFrom(const DQNCore &Source) {
  pImpl->copyWeightsFrom(*Source.pImpl);
}

DQNCore::DQNCore(StringRef ModelFile)
    : pImpl(llvm::make_unique<DQNCoreImpl>(ModelFile)) {}
DQNCore::~DQNCore() = default;

//===----------------------------------------------------------------------===//
//                          pImpl's method defintions
//===----------------------------------------------------------------------===//

template <unsigned Size>
inline torch::Tensor DQNCoreImpl::forward(const Batch<State> &S) const {
  auto &Cache = getCache<Size>();

  // we just loaded and forwarded exactly this state!
  if (Cache == S) {
    return LastTensor;
  }

  torch::Tensor state = torch::zeros({Size, config::NumberOfFeatures}),
                context =
                    torch::zeros({config::ContextSize, Size}, torch::kLong);
  for (auto i : seq<unsigned>(0, Size)) {
    auto &StateRef = S[i];
    for (auto j : seq<unsigned>(0, config::NumberOfFeatures)) {
      state[i][j] = StateRef[j];
    }

    unsigned j = 0, k = 0, N = StateRef.getContext().size();
    for (k = std::max<int>(j, N - config::ContextSize); k < N; ++k, ++j) {
      context[j][i] = static_cast<int64_t>(StateRef.getContext()[k] + 1);
    }
  }

  Cache = S;
  LastTensor = Brain.forward(state, context);
  return LastTensor;
}

ResultsVector DQNCoreImpl::calculate(const State &S) const {
  Batch<State> Wrapper{S};
  return calculateImpl<1>(Wrapper)[0];
}

Result DQNCoreImpl::max(const State &S) const {
  Batch<State> Wrapper{S};
  return maxImpl<1>(Wrapper)[0];
}

Action DQNCoreImpl::argmax(const State &S) const {
  Batch<State> Wrapper{S};
  return argmaxImpl<1>(Wrapper)[0];
}

Batch<ResultsVector> DQNCoreImpl::calculate(const Batch<State> &S) const {
  return calculateImpl<config::MinibatchSize>(S);
}

Batch<Result> DQNCoreImpl::max(const Batch<State> &S) const {
  return maxImpl<config::MinibatchSize>(S);
}

Batch<Action> DQNCoreImpl::argmax(const Batch<State> &S) const {
  return argmaxImpl<config::MinibatchSize>(S);
}

template <unsigned Size>
inline Batch<ResultsVector>
DQNCoreImpl::calculateImpl(const Batch<State> &S) const {
  torch::Tensor tensor = forward<Size>(S);
  auto accessor = tensor.accessor<Result, 2>();

  Batch<ResultsVector> result(Size,
                              ResultsVector(config::NumberOfActions, 0.0));

  for (auto i : seq<unsigned>(0, Size)) {
    for (auto j : seq<unsigned>(0, config::NumberOfActions)) {
      result[i][j] = accessor[i][j];
    }
  }

  return result;
}

template <unsigned Size>
inline Batch<Result> DQNCoreImpl::maxImpl(const Batch<State> &S) const {
  torch::Tensor tensor = std::get<0>(forward<Size>(S).max(1));
  auto accessor = tensor.accessor<Result, 1>();

  Batch<Result> Results;
  for (auto i : seq<unsigned>(0, Size)) {
    Results.push_back(accessor[i]);
  }

  return Results;
}

template <unsigned Size>
inline Batch<Action> DQNCoreImpl::argmaxImpl(const Batch<State> &S) const {
  torch::Tensor tensor = std::get<1>(forward<Size>(S).max(1));
  auto accessor = tensor.accessor<long, 1>();

  Batch<Action> Actions;
  for (auto i : seq<unsigned>(0, Size)) {
    Actions.push_back(
        Action::getActionByIndex(static_cast<unsigned>(accessor[i])));
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

void DQNCoreImpl::update(const State &S, const Action &A, Result value) {
  // this implementation doesn't support non-minibatch update
}

void DQNCoreImpl::update(const Batch<State> &S, const Batch<Action> &A,
                         Batch<Result> V) {
  Brain.train();
  Brain.zero_grad();

  auto QValues = forward<config::MinibatchSize>(S);

  torch::Tensor Actions =
                    torch::zeros({config::MinibatchSize, 1}, torch::kLong),
                Values = torch::zeros({config::MinibatchSize, 1});

  for (auto i : seq<unsigned>(0, A.size())) {
    Actions[i] = static_cast<int64_t>(A[i].getIndex());
    Values[i] = V[i];
  }
  auto Q = QValues.gather(1, Actions);

  Brain.backward(Q, Values);
  for (auto Param : Brain.parameters()) {
    Param.grad().clamp_(-1, 1);
  }
  Optimizer->step();
  Brain.eval();
}

void DQNCoreImpl::copyWeightsFrom(const DQNCoreImpl &Source) {
  Source.saveNet();
  loadNetFromFile(Source.ModelFile);
}

inline void DQNCoreImpl::initialize() {
  Optimizer = llvm::make_unique<torch::optim::Adam>(
      Brain.parameters(), torch::optim::AdamOptions(1e-4).beta1(0.5));
  loadNet();
  Brain.eval();
}

inline void DQNCoreImpl::loadNet() { loadNetFromFile(ModelFile); }

inline void DQNCoreImpl::saveNet() const { saveNetToFile(ModelFile); }

inline void DQNCoreImpl::loadNetFromFile(StringRef File) {
  if (sys::fs::exists(File)) {
    torch::serialize::InputArchive SerializedModel;
    SerializedModel.load_from(File);
    Brain.load(SerializedModel);
  }
}

inline void DQNCoreImpl::saveNetToFile(StringRef File) const {
  torch::serialize::OutputArchive SerializedModel;
  Brain.save(SerializedModel);
  SerializedModel.save_to(File);
}

DQNCoreImpl::~DQNCoreImpl() { saveNet(); }
} // namespace wazuhl
} // namespace llvm

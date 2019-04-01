#include "llvm/Wazuhl/ExperienceReplay.h"

#include "llvm/Support/raw_ostream.h"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <cmath>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::document::view;
using mongocxx::pipeline;

constexpr unsigned NumberOfCollections = 3;
using Collections =
    llvm::SmallVector<mongocxx::collection *, NumberOfCollections>;

namespace {
using State = llvm::wazuhl::ExperienceReplay::State;
using Action = llvm::wazuhl::ExperienceReplay::Action;
using Result = llvm::wazuhl::ExperienceReplay::Result;
using RecalledExperience = llvm::wazuhl::ExperienceReplay::RecalledExperience;
using MongifiedExperience = bsoncxx::document::value;
struct ExperienceUnit {
  State state;
  unsigned actionIndex;
  Result value;
  State newState;
};

mongocxx::instance instance{};

template <typename Type, class Doc, class IterableT>
void addArray(Doc &Destination, const llvm::StringRef ArrayName,
              const IterableT &List) {
  auto array = Destination << ArrayName << open_array;
  for (auto element : List) {
    array = array << (Type)element;
  }
  array << close_array;
}

template <class IterableT>
void fillStateArray(IterableT &List, const llvm::StringRef ArrayName,
                    view &DocumentView) {
  auto element = DocumentView[ArrayName.str()];
  bsoncxx::array::view array = element.get_array();
  unsigned i = 0;
  for (auto value : array) {
    List[i++] = value.get_double();
  }
}

template <class IterableT>
void fillContextArray(IterableT &List, const llvm::StringRef ArrayName,
                      view &DocumentView) {
  auto element = DocumentView[ArrayName.str()];
  bsoncxx::array::view array = element.get_array();
  for (auto value : array) {
    List.push_back(static_cast<unsigned>(value.get_int32()));
  }
}
} // anonymous namespace

namespace llvm {
namespace wazuhl {
class ExperienceReplayImpl {
public:
  ExperienceReplayImpl();
  ExperienceReplayImpl(const ExperienceReplayImpl &) = delete;
  ExperienceReplayImpl(ExperienceReplayImpl &&) = delete;
  ExperienceReplayImpl &operator=(const ExperienceReplayImpl &) = delete;
  ExperienceReplayImpl &operator=(ExperienceReplayImpl &&) = delete;

  ~ExperienceReplayImpl();

  void push(const State &S, const Action &A, Result R, const State &newS);
  RecalledExperience sample(unsigned size);
  bool isBigEnoughForReplay();

private:
  void initialize();
  void createCollection(mongocxx::collection &collection,
                        const std::string &name, bool capped);

  void uploadApprovedExperience();
  void uploadExperienceWaitingForApproval();

  void uploadExperienceImpl(Collections collections, bool isTerminal);

  RecalledExperience aggregate(mongocxx::cursor &cursor, int size = -1);

  mongocxx::client MongoClient{mongocxx::uri{"mongodb://mongo:27017"}};
  mongocxx::database ExternalMemory;
  mongocxx::collection ApprovedRecords;
  mongocxx::collection RecordsWaitingForApproval;
  mongocxx::collection AllRecords;

  // it should be at least one action taken (terminal)
  ExperienceUnit LastExperience{};
};

//===----------------------------------------------------------------------===//
//                    Original class methods implementation
//===----------------------------------------------------------------------===//

ExperienceReplay::ExperienceReplay()
    : pImpl(llvm::make_unique<ExperienceReplayImpl>()) {}
ExperienceReplay::~ExperienceReplay() = default;

void ExperienceReplay::push(const State &S, const Action &A, Result R,
                            const State &newS) {
  pImpl->push(S, A, R, newS);
}

ExperienceReplay::RecalledExperience ExperienceReplay::sample(unsigned size) {
  return pImpl->sample(size);
}

}

bool ExperienceReplay::isBigEnoughForReplay() {
  return pImpl->isBigEnoughForReplay();
}

//===----------------------------------------------------------------------===//
//                         Implementation class methods
//===----------------------------------------------------------------------===//

ExperienceReplayImpl::ExperienceReplayImpl() { initialize(); }

void ExperienceReplayImpl::initialize() {
  ExternalMemory = MongoClient["wazuhl"];
  createCollection(ApprovedRecords, "approved", true);
  createCollection(RecordsWaitingForApproval, "waiting", false);
  createCollection(AllRecords, "all", false);
}

void ExperienceReplayImpl::createCollection(mongocxx::collection &collection,
                                            const std::string &name,
                                            bool capped) {
  if (!ExternalMemory.has_collection(name)) {
    if (capped) {
      // Experience replay should be renewable
      // it means that old records must not be in use.
      // Mongo 'max' option for collections does exactly what we want:
      // when max is reached, old records are deleted
      auto createCollectionCommand =
          document{} << "create" << name << "capped"
                     << true // collection with 'max' is capped
                     << "size"
                     << 4294967296 // capped collection should have 'size'
                                   // attribute we limit it to 4GB
                     << "max" << (int)config::ExperienceSize << finalize;
      // TODO: move constants to config

      // collection is created by a 'document' command and not by
      // 'create_collection' function because its' 'size' parameter is
      // restricted to int values
      ExternalMemory.run_command(std::move(createCollectionCommand));
    } else {
      ExternalMemory.create_collection(name);
    }
  }
  collection = ExternalMemory.collection(name);
}

void ExperienceReplayImpl::push(const State &S, const Action &A, Result R,
                                const State &newS) {
  uploadApprovedExperience();
  LastExperience = {S, A.getIndex(), R, newS};
}

void ExperienceReplayImpl::uploadApprovedExperience() {
  uploadExperienceImpl({&ApprovedRecords, &AllRecords}, false);
}

void ExperienceReplayImpl::uploadExperienceWaitingForApproval() {
  uploadExperienceImpl({&RecordsWaitingForApproval}, true);
}

void ExperienceReplayImpl::uploadExperienceImpl(Collections collections,
                                                bool isTerminal) {
  const auto &S = LastExperience.state;
  const auto A = LastExperience.actionIndex;
  const auto R = LastExperience.value;
  const auto &NewS = LastExperience.newState;

  if (not S.isInitialized()) {
    return;
  }

  auto ExperienceRecord = document{};

  addArray<double>(ExperienceRecord, "state", S);
  addArray<int>(ExperienceRecord, "context", S.getContext());
  ExperienceRecord << "reward" << R;
  ExperienceRecord << "index" << (int)A;
  ExperienceRecord << "isTerminal" << isTerminal;
  addArray<double>(ExperienceRecord, "newState", NewS);
  addArray<int>(ExperienceRecord, "newContext", NewS.getContext());

  for (auto *collection : collections) {
    collection->insert_one(ExperienceRecord.view());
  }
}

ExperienceReplayImpl::~ExperienceReplayImpl() {
  uploadExperienceWaitingForApproval();
}

bool ExperienceReplayImpl::isBigEnoughForReplay() {
  return ApprovedRecords.count_documents({}) >= config::MinimalExperienceSize;
}

RecalledExperience ExperienceReplayImpl::sample(unsigned size) {
  // Randomly choose MinibatchSize number of experience entries
  auto SampleQuery = pipeline{};
  int querySize = std::max<int>(size, config::ExperienceSize / 20 + 1);
  SampleQuery.sample(querySize);
  auto cursor = ApprovedRecords.aggregate(SampleQuery);
  return aggregate(cursor, size);
}

RecalledExperience ExperienceReplayImpl::aggregate(mongocxx::cursor &cursor,
                                                   int size) {
  RecalledExperience Sample;

  for (auto doc : cursor) {
    State S, newS;

    fillStateArray(S, "state", doc);
    fillContextArray(S.getContext(), "context", doc);
    Result reward = doc["reward"].get_double();
    unsigned index = static_cast<unsigned>(doc["index"].get_int32());
    bool isTerminal = doc["isTerminal"].get_bool();
    fillStateArray(newS, "newState", doc);
    fillContextArray(newS.getContext(), "newContext", doc);

    Sample.S.emplace_back(std::move(S));
    Sample.A.emplace_back(Action::getActionByIndex(index));
    Sample.R.push_back(reward);
    Sample.newS.emplace_back(std::move(newS));
    Sample.isTerminal.push_back(isTerminal);
    if (Sample.S.size() == size) {
      break;
    }
  }

  return Sample;
}
} // namespace wazuhl
} // namespace llvm

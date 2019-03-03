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

namespace {
using ExperienceUnit = llvm::wazuhl::ExperienceReplay::ExperienceUnit;
using StateType = llvm::wazuhl::ExperienceReplay::State;
using ResultType = llvm::wazuhl::ExperienceReplay::Result;
using RecalledExperience = llvm::wazuhl::ExperienceReplay::RecalledExperience;
using MongifiedExperience = bsoncxx::document::value;

mongocxx::instance instance{};

template <class Doc, class IterableT>
void addArray(Doc &Destination, const llvm::StringRef ArrayName,
              const IterableT &List) {
  auto array = Destination << ArrayName << open_array;
  for (auto element : List) {
    array = array << element;
  }
  array << close_array;
}

template <class IterableT>
void fillArray(IterableT &List, const llvm::StringRef ArrayName,
               view &DocumentView) {
  auto element = DocumentView[ArrayName.str()];
  bsoncxx::array::view array = element.get_array();
  unsigned i = 0;
  for (auto value : array) {
    List[i++] = value.get_double();
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

  void addToExperience(ExperienceUnit);
  RecalledExperience replay();

private:
  void initialize();
  void createCollection(mongocxx::collection &collection,
                        const std::string &name, bool capped);

  void uploadRecordedTrace();

  mongocxx::client MongoClient{mongocxx::uri{"mongodb://mongo:27017"}};
  mongocxx::database ExternalMemory;
  mongocxx::collection ApprovedRecords;
  mongocxx::collection RecordsWaitingForApproval;

  // it should be at least one action taken (terminal)
  static constexpr unsigned TraceLength = 100;
  SmallVector<ExperienceUnit, TraceLength> RecordedTrace;
};

ExperienceReplay::ExperienceReplay()
    : pImpl(llvm::make_unique<ExperienceReplayImpl>()) {}
ExperienceReplay::~ExperienceReplay() = default;

void ExperienceReplay::addToExperience(
    ExperienceReplay::ExperienceUnit toRemember) {
  pImpl->addToExperience(toRemember);
}

ExperienceReplay::RecalledExperience ExperienceReplay::replay() {
  return pImpl->replay();
}

ExperienceReplayImpl::ExperienceReplayImpl() { initialize(); }

void ExperienceReplayImpl::initialize() {
  ExternalMemory = MongoClient["wazuhl"];
  createCollection(ApprovedRecords, "approved", true);
  createCollection(RecordsWaitingForApproval, "waiting", false);
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
                     << "max" << config::ExperienceSize << finalize;
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

void ExperienceReplayImpl::addToExperience(ExperienceUnit toRemember) {
  RecordedTrace.emplace_back(std::move(toRemember));
}

void ExperienceReplayImpl::uploadRecordedTrace() {
  if (RecordedTrace.empty()) {
    return;
  }

  auto ExperienceRecord = document{};

  auto array = ExperienceRecord << "trace" << open_array;

  for (auto &Experience : RecordedTrace) {
    const auto &State = Experience.state;
    const auto Index = Experience.actionIndex;
    const auto Value = Experience.value;

    auto object = array << open_document;

    addArray(object, "state", State);
    object << "value" << Value;
    object << "index" << (int)Index;

    object << close_document;
  }

  array << close_array;

  RecordsWaitingForApproval.insert_one(ExperienceRecord.view());
}

ExperienceReplayImpl::~ExperienceReplayImpl() { uploadRecordedTrace(); }

RecalledExperience ExperienceReplayImpl::replay() {
  RecalledExperience Result;
  // Wazuhl doesn't have enough experience to even start
  // the learning process
  constexpr unsigned MinimalSizeForDB = 300;
  if (ApprovedRecords.count_documents({}) <
      std::max(MinimalSizeForDB, config::MinibatchSize))
    return Result;

  // Randomly choose MinibatchSize number of experience entries
  auto SampleQuery = pipeline{};
  SampleQuery.sample(MinimalSizeForDB);

  auto Cursor = ApprovedRecords.aggregate(SampleQuery);

  for (auto doc : Cursor) {
    StateType state;
    fillArray(state, "state", doc);
    ResultType value = doc["value"].get_double();
    unsigned index = static_cast<unsigned>(doc["index"].get_int32());
    Result.push_back({state, index, value});
    if (Result.size() == config::MinibatchSize) {
      break;
    }
  }

  return Result;
}
} // namespace wazuhl
} // namespace llvm

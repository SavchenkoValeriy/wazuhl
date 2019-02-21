#include "llvm/Wazuhl/ExperienceReplay.h"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

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
using RecalledExperience = llvm::wazuhl::ExperienceReplay::RecalledExperience;
using MongifiedExperience = bsoncxx::document::value;

mongocxx::instance instance{};

template <class IterableT>
void addArray(document &Destination, const llvm::StringRef ArrayName,
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
  for (auto value : array) {
    List.push_back(value.get_double());
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

  void addToExperience(ExperienceUnit);
  void addToExperience(StateType);
  RecalledExperience replay();

private:
  void initialize();
  void createCollection(mongocxx::collection &collection,
                        const std::string &name);

  mongocxx::client MongoClient{mongocxx::uri{"mongodb://mongo:27017"}};
  mongocxx::database ExternalMemory;
  mongocxx::collection ApprovedRecords;
  mongocxx::collection RecordsWaitingForApproval;
};

ExperienceReplay::ExperienceReplay()
    : pImpl(llvm::make_unique<ExperienceReplayImpl>()) {}
ExperienceReplay::~ExperienceReplay() = default;

void ExperienceReplay::addToExperience(
    ExperienceReplay::ExperienceUnit toRemember) {
  pImpl->addToExperience(toRemember);
}

void ExperienceReplay::addToExperience(ExperienceReplay::State terminal) {
  pImpl->addToExperience(terminal);
}

ExperienceReplay::RecalledExperience ExperienceReplay::replay() {
  return pImpl->replay();
}

ExperienceReplayImpl::ExperienceReplayImpl() { initialize(); }

void ExperienceReplayImpl::initialize() {
  ExternalMemory = MongoClient["wazuhl"];
  createCollection(ApprovedRecords, "approved");
  createCollection(RecordsWaitingForApproval, "waiting");
}

void ExperienceReplayImpl::createCollection(mongocxx::collection &collection,
                                            const std::string &name) {
  if (!ExternalMemory.has_collection(name)) {
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
    // 'create_collection' function because its' 'size' parameter is restricted
    // to int values
    ExternalMemory.run_command(std::move(createCollectionCommand));
  }
  collection = ExternalMemory.collection(name);
}

void ExperienceReplayImpl::addToExperience(ExperienceUnit toRemember) {
  auto ExperienceRecord = document{};

  const auto &State = toRemember.first;
  const auto &Values = toRemember.second;

  addArray(ExperienceRecord, "state", State);
  addArray(ExperienceRecord, "values", Values);

  ApprovedRecords.insert_one(ExperienceRecord.view());
}

void ExperienceReplayImpl::addToExperience(StateType terminal) {
  auto UnapprovedExperience = document{};

  addArray(UnapprovedExperience, "state", terminal);

  RecordsWaitingForApproval.insert_one(UnapprovedExperience.view());
}

RecalledExperience ExperienceReplayImpl::replay() {
  using StateType = ExperienceUnit::first_type;
  using ValuesType = ExperienceUnit::second_type;

  RecalledExperience Result;
  // Wazuhl doesn't have enough experience to even start
  // the learning process
  llvm::errs() << "Wazuhl's checking for experience\n";
  constexpr auto MinimalSizeForDB = 100;
  if (ApprovedRecords.count({}) <
      std::max(MinimalSizeForDB, config::MinibatchSize))
    return Result;

  // Randomly choose MinibatchSize number of experience entries
  llvm::errs() << "There is some experience for Wazuhl to learn from\n";
  auto SampleQuery = pipeline{};
  SampleQuery.sample(config::MinibatchSize);

  auto Cursor = ApprovedRecords.aggregate(SampleQuery);
  for (auto doc : Cursor) {
    StateType state;
    fillArray(state, "state", doc);
    ValuesType values;
    fillArray(values, "values", doc);
    Result.push_back({state, values});
  }

  return Result;
}
} // namespace wazuhl
} // namespace llvm

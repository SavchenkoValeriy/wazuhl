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

namespace {
  using ExperienceUnit = llvm::wazuhl::ExperienceReplay::ExperienceUnit;
  using RecalledExperience = llvm::wazuhl::ExperienceReplay::RecalledExperience;
  using MongifiedExperience = bsoncxx::document::value;

  mongocxx::instance instance{};
}

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
    RecalledExperience replay();

  private:
    void initialize();
    void createCollection(mongocxx::collection &collection, const std::string &name);

    mongocxx::client MongoClient{mongocxx::uri{}};
    mongocxx::database ExternalMemory;
    mongocxx::collection ApprovedRecords;
    mongocxx::collection RecordsWaitingForApproval;
  };

  ExperienceReplay::ExperienceReplay() :
    pImpl(llvm::make_unique<ExperienceReplayImpl>()) {}
  ExperienceReplay::~ExperienceReplay() = default;

  void ExperienceReplay::addToExperience(ExperienceReplay::ExperienceUnit toRemember) {
    pImpl->addToExperience(toRemember);
  }

  ExperienceReplay::RecalledExperience ExperienceReplay::replay() {
    return pImpl->replay();
  }

  ExperienceReplayImpl::ExperienceReplayImpl() {
    initialize();
  }

  void ExperienceReplayImpl::initialize() {
    ExternalMemory = MongoClient["wazuhl"];
    createCollection(ApprovedRecords, "approved");
    createCollection(RecordsWaitingForApproval, "waiting");
  }

  void ExperienceReplayImpl::createCollection(mongocxx::collection &collection,
                                              const std::string &name) {
    if (!ExternalMemory.has_collection(name)) {
      // Experience replay should be renewable
      // it means that old records must not be in use
      // Mongo 'max' option for collections does exactly what we want:
      // when max is reached, old records are deleted
      auto createCollectionCommand = document{}
          << "create" << name
          << "capped" << true     // collection with 'max' is capped
          << "size" << 4294967296 // capped collection should have 'size' attribute
                                  // we limit it to 4GB
          << "max" << 1000 << finalize;
      // TODO: move constants to config

      ExternalMemory.run_command(std::move(createCollectionCommand));
    }
    collection = ExternalMemory.collection(name);
  }

  void ExperienceReplayImpl::addToExperience(ExperienceUnit toRemember) {
    auto ExperienceRecord = document{};

    const auto &State = toRemember.first;
    const auto &Values = toRemember.second;

    auto array = ExperienceRecord << "state" << open_array;
    for (auto feature : State) {
      array = array << feature;
    }
    array << close_array;

    array = ExperienceRecord << "values" << open_array;
    for (auto value : Values) {
      array = array << value;
    }
    array << close_array;

    ApprovedRecords.insert_one(ExperienceRecord.view());
  }

  RecalledExperience ExperienceReplayImpl::replay() {
    return {};
  }

}
}

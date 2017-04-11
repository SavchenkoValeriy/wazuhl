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
    mongocxx::client client{mongocxx::uri{}};
    ExternalMemory = client["wazuhl"];
    createCollection(ApprovedRecords, "approved");
    createCollection(RecordsWaitingForApproval, "waiting");
  }

  void ExperienceReplayImpl::createCollection(mongocxx::collection &collection,
                                              const std::string &name) {
    if (ExternalMemory.has_collection(name)) return; //nothing to do here

    // Experience replay should be renewable
    // it means that old records must not be in use
    // Mongo max option for collections does exactly what we want:
    // when max is reached, old records are deleted
    auto collectionOptions = mongocxx::options::create_collection().
                                                capped(true).
                                                size(4294967296).
                                                max(1000);
    collection = ExternalMemory.create_collection(name, collectionOptions);
  }

  void ExperienceReplayImpl::addToExperience(ExperienceUnit toRemember) {

  }

  RecalledExperience ExperienceReplayImpl::replay() {
    return {};
  }

}
}

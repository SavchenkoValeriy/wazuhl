find_package(libmongocxx REQUIRED)
find_package(libbsoncxx REQUIRED)
find_package(Protobuf REQUIRED)

include_directories(
  SYSTEM
  ${LLVM_MAIN_SRC_DIR}/lib/Passes
  ${LIBMONGOCXX_INCLUDE_DIRS}
  ${LIBBSONCXX_INCLUDE_DIRS}
  ${TORCH_INCLUDE_DIRS}
  )

add_definitions(
  -DBOOST_NO_EXCEPTIONS
  )

add_llvm_library(LLVMWazuhl
  Config.cpp
  DQN.cpp
  Environment.cpp
  ExperienceReplay.cpp
  FeatureCollector.cpp
  Manager.cpp
  NormalizedTimer.cpp
  PassAction.cpp
  PolicyEvaluator.cpp
  Random.cpp

  LINK_LIBS
  ${TORCH_LIBRARIES}
  ${PROTOBUF_LIBRARY}
  ${LIBMONGOCXX_LIBRARIES}
  ${LIBBSONCXX_LIBRARIES}
  )

set_property(TARGET LLVMWazuhl APPEND_STRING PROPERTY
  COMPILE_FLAGS " -fexceptions")

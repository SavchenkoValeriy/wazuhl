find_package(Caffe REQUIRED CONFIG)

find_package(PkgConfig)
pkg_check_modules(LIBBSONXX REQUIRED libbsoncxx)
pkg_check_modules(LIBMONGOCXX REQUIRED libmongocxx)

include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Passes
  ${Caffe_INCLUDE_DIRS}
  ${LIBMONGOCXX_INCLUDE_DIRS}
  ${LIBBSONCXX_INCLUDE_DIRS}
  )

link_directories(
  ${LIBMONGOCXX_LIBRARIES}
  ${LIBBSONCXX_LIBRARIES}
  )

add_definitions(
  ${Caffe_DEFINITIONS}
  -DGOOGLE_PROTOBUF_NO_RTTI
  -DBOOST_NO_EXCEPTIONS
  -DBOOST_NO_RTTI
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
  ${Caffe_LIBRARIES}
  ${LIBMONGOCXX_LIBRARIES}
  ${LIBBSONCXX_LIBRARIES}
  )

set_property(TARGET LLVMWazuhl APPEND_STRING PROPERTY
  COMPILE_FLAGS " -fexceptions")

install(
  FILES
  model.prototxt
  solver.prototxt
  DESTINATION wazuhl
  )

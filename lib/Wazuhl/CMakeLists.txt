find_package(Caffe REQUIRED CONFIG)

include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Passes
  ${Caffe_INCLUDE_DIRS}
  )

add_definitions(
  ${Caffe_DEFINITIONS}
  )

add_llvm_library(LLVMWazuhl
  Action.cpp
  DQN.cpp
  FeatureCollector.cpp
  Manager.cpp
  Random.cpp
  LINK_LIBS
  ${Caffe_LIBRARIES}
  )

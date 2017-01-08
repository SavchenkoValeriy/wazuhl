include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Passes
  )

add_llvm_library(LLVMWazuhl
  Action.cpp
  Manager.cpp
  )

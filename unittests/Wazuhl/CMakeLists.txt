set(LLVM_LINK_COMPONENTS
  Wazuhl
  )

add_llvm_unittest(WazuhlTests
  ReinforcementLearningTest.cpp
  )

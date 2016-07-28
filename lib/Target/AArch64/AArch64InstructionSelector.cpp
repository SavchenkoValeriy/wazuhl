//===- AArch64InstructionSelector.cpp ----------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the InstructionSelector class for
/// AArch64.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#include "AArch64InstructionSelector.h"
#include "AArch64InstrInfo.h"
#include "AArch64RegisterBankInfo.h"
#include "AArch64RegisterInfo.h"
#include "AArch64Subtarget.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "aarch64-isel"

using namespace llvm;

#ifndef LLVM_BUILD_GLOBAL_ISEL
#error "You shouldn't build this"
#endif

AArch64InstructionSelector::AArch64InstructionSelector(
    const AArch64Subtarget &STI, const AArch64RegisterBankInfo &RBI)
    : InstructionSelector(), TII(*STI.getInstrInfo()),
      TRI(*STI.getRegisterInfo()), RBI(RBI) {}

/// Select the AArch64 opcode for the basic binary operation \p GenericOpc
/// (such as G_OR or G_ADD), appropriate for the register bank \p RegBankID
/// and of size \p OpSize.
/// \returns \p GenericOpc if the combination is unsupported.
static unsigned selectBinaryOp(unsigned GenericOpc, unsigned RegBankID,
                               unsigned OpSize) {
  switch (RegBankID) {
  case AArch64::GPRRegBankID:
    switch (OpSize) {
    case 32:
      switch (GenericOpc) {
      case TargetOpcode::G_OR:
        return AArch64::ORRWrr;
      case TargetOpcode::G_ADD:
        return AArch64::ADDWrr;
      default:
        return GenericOpc;
      }
    case 64:
      switch (GenericOpc) {
      case TargetOpcode::G_OR:
        return AArch64::ORRXrr;
      case TargetOpcode::G_ADD:
        return AArch64::ADDXrr;
      default:
        return GenericOpc;
      }
    }
  };
  return GenericOpc;
}

bool AArch64InstructionSelector::select(MachineInstr &I) const {
  assert(I.getParent() && "Instruction should be in a basic block!");
  assert(I.getParent()->getParent() && "Instruction should be in a function!");

  MachineBasicBlock &MBB = *I.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  // FIXME: Is there *really* nothing to be done here?  This assumes that
  // no upstream pass introduces things like generic vreg on copies or
  // target-specific instructions.
  // We should document (and verify) that assumption.
  if (!isPreISelGenericOpcode(I.getOpcode()))
    return true;

  if (I.getNumOperands() != I.getNumExplicitOperands()) {
    DEBUG(dbgs() << "Generic instruction has unexpected implicit operands\n");
    return false;
  }

  LLT Ty = I.getType();
  assert(Ty.isValid() && "Generic instruction doesn't have a type");

  // FIXME: Support unsized instructions (e.g., G_BR).
  if (!Ty.isSized()) {
    DEBUG(dbgs() << "Unsized generic instructions are unsupported\n");
    return false;
  }

  // The size (in bits) of the operation, or 0 for the label type.
  const unsigned OpSize = Ty.getSizeInBits();

  switch (I.getOpcode()) {
  case TargetOpcode::G_OR:
  case TargetOpcode::G_ADD: {
    DEBUG(dbgs() << "AArch64: Selecting: binop\n");

    // Reject the various things we don't support yet.
    {
      const RegisterBank *PrevOpBank = nullptr;
      for (auto &MO : I.operands()) {
        // FIXME: Support non-register operands.
        if (!MO.isReg()) {
          DEBUG(dbgs() << "Generic inst non-reg operands are unsupported\n");
          return false;
        }

        // FIXME: Can generic operations have physical registers operands? If
        // so, this will need to be taught about that, and we'll need to get the
        // bank out of the minimal class for the register.
        // Either way, this needs to be documented (and possibly verified).
        if (!TargetRegisterInfo::isVirtualRegister(MO.getReg())) {
          DEBUG(dbgs() << "Generic inst has physical register operand\n");
          return false;
        }

        const RegisterBank *OpBank = RBI.getRegBank(MO.getReg(), MRI, TRI);
        if (!OpBank) {
          DEBUG(dbgs() << "Generic register has no bank or class\n");
          return false;
        }

        if (PrevOpBank && OpBank != PrevOpBank) {
          DEBUG(dbgs() << "Generic inst operands have different banks\n");
          return false;
        }
        PrevOpBank = OpBank;
      }
    }

    const unsigned DefReg = I.getOperand(0).getReg();
    const RegisterBank &RB = *RBI.getRegBank(DefReg, MRI, TRI);

    const unsigned NewOpc = selectBinaryOp(I.getOpcode(), RB.getID(), OpSize);
    if (NewOpc == I.getOpcode())
      return false;

    I.setDesc(TII.get(NewOpc));
    // FIXME: Should the type be always reset in setDesc?
    I.setType(LLT());

    // Now that we selected an opcode, we need to constrain the register
    // operands to use appropriate classes.
    return constrainSelectedInstRegOperands(I, TII, TRI, RBI);
  }
  }

  return false;
}
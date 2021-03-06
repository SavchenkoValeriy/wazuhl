; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -instcombine -S | FileCheck %s

define i32 @t1(i16 zeroext %x, i32 %y) {
; CHECK-LABEL: @t1(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[CONV:%.*]] = zext i16 %x to i32
; CHECK-NEXT:    [[TMP0:%.*]] = add i32 %y, 1
; CHECK-NEXT:    [[D:%.*]] = lshr i32 [[CONV]], [[TMP0]]
; CHECK-NEXT:    ret i32 [[D]]
;
entry:
  %conv = zext i16 %x to i32
  %s = shl i32 2, %y
  %d = sdiv i32 %conv, %s
  ret i32 %d
}

; rdar://11721329
define i64 @t2(i64 %x, i32 %y) {
; CHECK-LABEL: @t2(
; CHECK-NEXT:    [[TMP1:%.*]] = zext i32 %y to i64
; CHECK-NEXT:    [[TMP2:%.*]] = lshr i64 %x, [[TMP1]]
; CHECK-NEXT:    ret i64 [[TMP2]]
;
  %1 = shl i32 1, %y
  %2 = zext i32 %1 to i64
  %3 = udiv i64 %x, %2
  ret i64 %3
}

; PR13250
define i64 @t3(i64 %x, i32 %y) {
; CHECK-LABEL: @t3(
; CHECK-NEXT:    [[TMP1:%.*]] = add i32 %y, 2
; CHECK-NEXT:    [[TMP2:%.*]] = zext i32 [[TMP1]] to i64
; CHECK-NEXT:    [[TMP3:%.*]] = lshr i64 %x, [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP3]]
;
  %1 = shl i32 4, %y
  %2 = zext i32 %1 to i64
  %3 = udiv i64 %x, %2
  ret i64 %3
}

define i32 @t4(i32 %x, i32 %y) {
; CHECK-LABEL: @t4(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp ugt i32 %y, 5
; CHECK-NEXT:    [[DOTV:%.*]] = select i1 [[TMP1]], i32 %y, i32 5
; CHECK-NEXT:    [[TMP2:%.*]] = lshr i32 %x, [[DOTV]]
; CHECK-NEXT:    ret i32 [[TMP2]]
;
  %1 = shl i32 1, %y
  %2 = icmp ult i32 %1, 32
  %3 = select i1 %2, i32 32, i32 %1
  %4 = udiv i32 %x, %3
  ret i32 %4
}

define i32 @t5(i1 %x, i1 %y, i32 %V) {
; CHECK-LABEL: @t5(
; CHECK-NEXT:    [[DOTV:%.*]] = select i1 %x, i32 5, i32 6
; CHECK-NEXT:    [[TMP1:%.*]] = lshr i32 %V, [[DOTV]]
; CHECK-NEXT:    [[TMP2:%.*]] = select i1 %y, i32 [[TMP1]], i32 0
; CHECK-NEXT:    ret i32 [[TMP2]]
;
  %1 = shl i32 1, %V
  %2 = select i1 %x, i32 32, i32 64
  %3 = select i1 %y, i32 %2, i32 %1
  %4 = udiv i32 %V, %3
  ret i32 %4
}

define i32 @t6(i32 %x, i32 %z) {
; CHECK-LABEL: @t6(
; CHECK-NEXT:    [[X_IS_ZERO:%.*]] = icmp eq i32 %x, 0
; CHECK-NEXT:    [[DIVISOR:%.*]] = select i1 [[X_IS_ZERO]], i32 1, i32 %x
; CHECK-NEXT:    [[Y:%.*]] = udiv i32 %z, [[DIVISOR]]
; CHECK-NEXT:    ret i32 [[Y]]
;
  %x_is_zero = icmp eq i32 %x, 0
  %divisor = select i1 %x_is_zero, i32 1, i32 %x
  %y = udiv i32 %z, %divisor
  ret i32 %y
}

%TrycatchContext = type {i8*, i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, double, double, double, double, double, double, double, double}
%BcTrycatchContext = type {%TrycatchContext, i8*}

define private void @checkso() alwaysinline {
  tail call void asm sideeffect "sub x9, sp, 0x10000 \0A ldr x9, [x9]", "~{x9},~{dirflag},~{fpsr},~{flags},~{cc}"() nounwind
  ret void
}

define private i8* @getpc() alwaysinline {
  %1 = tail call i8* asm sideeffect "adr $0, #0", "=r,~{dirflag},~{fpsr},~{flags}"() nounwind
  ret i8* %1
}

define private float @frem(%Env* %env, float %op1, float %op2) alwaysinline {
    %result = frem float %op1, %op2
    ret float %result
}

define private i1 @atomic_cas(i32 %old, i32 %new, i32* %ptr) alwaysinline {
  %1 = cmpxchg i32* %ptr, i32 %old, i32 %new seq_cst seq_cst
  %2 = extractvalue {i32, i1} %1, 0
  %3 = icmp eq i32 %2, %old
  ret i1 %3
}
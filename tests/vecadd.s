; ModuleID = 'vecadd.cl'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir"

; Function Attrs: nounwind
define void @vecadd(float addrspace(1)* nocapture %a, float addrspace(1)* nocapture %b, float addrspace(1)* nocapture %c) #0 {
entry:
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #2
  %arrayidx = getelementptr inbounds float addrspace(1)* %a, i32 %call
  %0 = load float addrspace(1)* %arrayidx, align 4, !tbaa !1
  %arrayidx1 = getelementptr inbounds float addrspace(1)* %b, i32 %call
  %1 = load float addrspace(1)* %arrayidx1, align 4, !tbaa !1
  %add = fadd float %0, %1
  %arrayidx2 = getelementptr inbounds float addrspace(1)* %c, i32 %call
  store float %add, float addrspace(1)* %arrayidx2, align 4, !tbaa !1
  ret void
}

declare i32 @get_global_id(...) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nobuiltin nounwind }

!opencl.kernels = !{!0}

!0 = metadata !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @vecadd}
!1 = metadata !{metadata !"float", metadata !2}
!2 = metadata !{metadata !"omnipotent char", metadata !3}
!3 = metadata !{metadata !"Simple C/C++ TBAA"}

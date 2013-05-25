; ModuleID = 'vecadd.cl'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir"

; Function Attrs: nounwind
define void @vecadd(float addrspace(1)* nocapture %a, float addrspace(1)* nocapture %b, float addrspace(1)* nocapture %c) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %a}, i64 0, metadata !11), !dbg !27
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %b}, i64 0, metadata !12), !dbg !28
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %c}, i64 0, metadata !13), !dbg !29
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #3, !dbg !30
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !14), !dbg !30
  %arrayidx = getelementptr inbounds float addrspace(1)* %a, i32 %call, !dbg !31
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !31, !tbaa !32
  %arrayidx1 = getelementptr inbounds float addrspace(1)* %b, i32 %call, !dbg !31
  %1 = load float addrspace(1)* %arrayidx1, align 4, !dbg !31, !tbaa !32
  %add = fadd float %0, %1, !dbg !31
  %arrayidx2 = getelementptr inbounds float addrspace(1)* %c, i32 %call, !dbg !31
  store float %add, float addrspace(1)* %arrayidx2, align 4, !dbg !31, !tbaa !32
  ret void, !dbg !35
}

declare i32 @get_global_id(...) #1

; Function Attrs: nounwind
define void @vecadd_guarded(float addrspace(1)* nocapture %a, float addrspace(1)* nocapture %b, float addrspace(1)* nocapture %c, i32 %n) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %a}, i64 0, metadata !20), !dbg !36
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %b}, i64 0, metadata !21), !dbg !37
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %c}, i64 0, metadata !22), !dbg !38
  tail call void @llvm.dbg.value(metadata !{i32 %n}, i64 0, metadata !23), !dbg !39
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #3, !dbg !40
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !24), !dbg !40
  %cmp = icmp slt i32 %call, %n, !dbg !41
  br i1 %cmp, label %if.end, label %return, !dbg !41

if.end:                                           ; preds = %entry
  %arrayidx = getelementptr inbounds float addrspace(1)* %a, i32 %call, !dbg !42
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !42, !tbaa !32
  %arrayidx1 = getelementptr inbounds float addrspace(1)* %b, i32 %call, !dbg !42
  %1 = load float addrspace(1)* %arrayidx1, align 4, !dbg !42, !tbaa !32
  %add = fadd float %0, %1, !dbg !42
  %arrayidx2 = getelementptr inbounds float addrspace(1)* %c, i32 %call, !dbg !42
  store float %add, float addrspace(1)* %arrayidx2, align 4, !dbg !42, !tbaa !32
  br label %return, !dbg !42

return:                                           ; preds = %entry, %if.end
  ret void, !dbg !42
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #2

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
attributes #3 = { nobuiltin nounwind }

!llvm.dbg.cu = !{!0}
!opencl.kernels = !{!25, !26}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.4 (trunk 182581)", i1 true, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !""} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/vecadd.cl] [DW_LANG_C99]
!1 = metadata !{metadata !"vecadd.cl", metadata !"/Users/james/projects/oclgrind/tests"}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4, metadata !16}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"vecadd", metadata !"vecadd", metadata !"", i32 1, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @vecadd, null, null, metadata !10, i32 4} ; [ DW_TAG_subprogram ] [line 1] [def] [scope 4] [vecadd]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/Users/james/projects/oclgrind/tests/vecadd.cl]
!6 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{null, metadata !8, metadata !8, metadata !8}
!8 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, metadata !9} ; [ DW_TAG_pointer_type ] [line 0, size 32, align 32, offset 0] [from float]
!9 = metadata !{i32 786468, null, null, metadata !"float", i32 0, i64 32, i64 32, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [float] [line 0, size 32, align 32, offset 0, enc DW_ATE_float]
!10 = metadata !{metadata !11, metadata !12, metadata !13, metadata !14}
!11 = metadata !{i32 786689, metadata !4, metadata !"a", metadata !5, i32 16777217, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [a] [line 1]
!12 = metadata !{i32 786689, metadata !4, metadata !"b", metadata !5, i32 33554434, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [b] [line 2]
!13 = metadata !{i32 786689, metadata !4, metadata !"c", metadata !5, i32 50331651, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [c] [line 3]
!14 = metadata !{i32 786688, metadata !4, metadata !"i", metadata !5, i32 5, metadata !15, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 5]
!15 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!16 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"vecadd_guarded", metadata !"vecadd_guarded", metadata !"", i32 9, metadata !17, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, i32)* @vecadd_guarded, null, null, metadata !19, i32 13} ; [ DW_TAG_subprogram ] [line 9] [def] [scope 13] [vecadd_guarded]
!17 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !18, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!18 = metadata !{null, metadata !8, metadata !8, metadata !8, metadata !15}
!19 = metadata !{metadata !20, metadata !21, metadata !22, metadata !23, metadata !24}
!20 = metadata !{i32 786689, metadata !16, metadata !"a", metadata !5, i32 16777225, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [a] [line 9]
!21 = metadata !{i32 786689, metadata !16, metadata !"b", metadata !5, i32 33554442, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [b] [line 10]
!22 = metadata !{i32 786689, metadata !16, metadata !"c", metadata !5, i32 50331659, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [c] [line 11]
!23 = metadata !{i32 786689, metadata !16, metadata !"n", metadata !5, i32 67108876, metadata !15, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [n] [line 12]
!24 = metadata !{i32 786688, metadata !16, metadata !"i", metadata !5, i32 14, metadata !15, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 14]
!25 = metadata !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @vecadd}
!26 = metadata !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, i32)* @vecadd_guarded}
!27 = metadata !{i32 1, i32 0, metadata !4, null}
!28 = metadata !{i32 2, i32 0, metadata !4, null}
!29 = metadata !{i32 3, i32 0, metadata !4, null}
!30 = metadata !{i32 5, i32 0, metadata !4, null}
!31 = metadata !{i32 6, i32 0, metadata !4, null}
!32 = metadata !{metadata !"float", metadata !33}
!33 = metadata !{metadata !"omnipotent char", metadata !34}
!34 = metadata !{metadata !"Simple C/C++ TBAA"}
!35 = metadata !{i32 7, i32 0, metadata !4, null}
!36 = metadata !{i32 9, i32 0, metadata !16, null}
!37 = metadata !{i32 10, i32 0, metadata !16, null}
!38 = metadata !{i32 11, i32 0, metadata !16, null}
!39 = metadata !{i32 12, i32 0, metadata !16, null}
!40 = metadata !{i32 14, i32 0, metadata !16, null}
!41 = metadata !{i32 15, i32 0, metadata !16, null}
!42 = metadata !{i32 20, i32 0, metadata !16, null}

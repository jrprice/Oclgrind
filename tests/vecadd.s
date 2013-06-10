; ModuleID = 'vecadd.cl'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @vecadd(float addrspace(1)* nocapture %a, float addrspace(1)* nocapture %b, float addrspace(1)* nocapture %c) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %a}, i64 0, metadata !13), !dbg !42
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %b}, i64 0, metadata !14), !dbg !43
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %c}, i64 0, metadata !15), !dbg !44
  %call = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !45
  %sext = shl i64 %call, 32, !dbg !46
  %idxprom = ashr exact i64 %sext, 32, !dbg !46
  %arrayidx = getelementptr inbounds float addrspace(1)* %a, i64 %idxprom, !dbg !46
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !46, !tbaa !47
  %arrayidx2 = getelementptr inbounds float addrspace(1)* %b, i64 %idxprom, !dbg !46
  %1 = load float addrspace(1)* %arrayidx2, align 4, !dbg !46, !tbaa !47
  %add = fadd float %0, %1, !dbg !46
  %arrayidx4 = getelementptr inbounds float addrspace(1)* %c, i64 %idxprom, !dbg !46
  store float %add, float addrspace(1)* %arrayidx4, align 4, !dbg !46, !tbaa !47
  ret void, !dbg !50
}

declare spir_func i64 @get_global_id(i32)

define spir_kernel void @vecadd_guarded(float addrspace(1)* nocapture %a, float addrspace(1)* nocapture %b, float addrspace(1)* nocapture %c, i32 %n) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %a}, i64 0, metadata !24), !dbg !51
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %b}, i64 0, metadata !25), !dbg !52
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %c}, i64 0, metadata !26), !dbg !53
  tail call void @llvm.dbg.value(metadata !{i32 %n}, i64 0, metadata !27), !dbg !54
  %call = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !55
  %conv = trunc i64 %call to i32, !dbg !55
  tail call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !28), !dbg !55
  %cmp = icmp slt i32 %conv, %n, !dbg !56
  br i1 %cmp, label %if.end, label %return, !dbg !56

if.end:                                           ; preds = %entry
  %idxprom = sext i32 %conv to i64, !dbg !57
  %arrayidx = getelementptr inbounds float addrspace(1)* %a, i64 %idxprom, !dbg !57
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !57, !tbaa !47
  %arrayidx3 = getelementptr inbounds float addrspace(1)* %b, i64 %idxprom, !dbg !57
  %1 = load float addrspace(1)* %arrayidx3, align 4, !dbg !57, !tbaa !47
  %add = fadd float %0, %1, !dbg !57
  %arrayidx5 = getelementptr inbounds float addrspace(1)* %c, i64 %idxprom, !dbg !57
  store float %add, float addrspace(1)* %arrayidx5, align 4, !dbg !57, !tbaa !47
  br label %return, !dbg !58

return:                                           ; preds = %entry, %if.end
  ret void, !dbg !58
}

declare void @llvm.dbg.value(metadata, i64, metadata) nounwind readnone

!llvm.dbg.cu = !{!0}
!opencl.kernels = !{!30, !36}
!opencl.enable.FP_CONTRACT = !{}

!0 = metadata !{i32 786449, i32 0, i32 12, metadata !"<unknown>", metadata !"/Users/james/projects/oclgrind/tests", metadata !"clang version 3.2 (tags/RELEASE_32/final 183304)", i1 true, i1 true, metadata !"", i32 0, metadata !1, metadata !1, metadata !3, metadata !1} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/<unknown>] [DW_LANG_C99]
!1 = metadata !{metadata !2}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4}
!4 = metadata !{metadata !5, metadata !19}
!5 = metadata !{i32 786478, i32 0, metadata !6, metadata !"vecadd", metadata !"vecadd", metadata !"", metadata !6, i32 3, metadata !7, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @vecadd, null, null, metadata !11, i32 6} ; [ DW_TAG_subprogram ] [line 3] [def] [scope 6] [vecadd]
!6 = metadata !{i32 786473, metadata !"vecadd.cl", metadata !"/Users/james/projects/oclgrind/tests", null} ; [ DW_TAG_file_type ]
!7 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !8, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!8 = metadata !{null, metadata !9, metadata !9, metadata !9}
!9 = metadata !{i32 786447, null, metadata !"", null, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !10} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from float]
!10 = metadata !{i32 786468, null, metadata !"float", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [float] [line 0, size 32, align 32, offset 0, enc DW_ATE_float]
!11 = metadata !{metadata !12}
!12 = metadata !{metadata !13, metadata !14, metadata !15, metadata !16}
!13 = metadata !{i32 786689, metadata !5, metadata !"a", metadata !6, i32 16777219, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [a] [line 3]
!14 = metadata !{i32 786689, metadata !5, metadata !"b", metadata !6, i32 33554436, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [b] [line 4]
!15 = metadata !{i32 786689, metadata !5, metadata !"c", metadata !6, i32 50331653, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [c] [line 5]
!16 = metadata !{i32 786688, metadata !17, metadata !"i", metadata !6, i32 7, metadata !18, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 7]
!17 = metadata !{i32 786443, metadata !5, i32 6, i32 0, metadata !6, i32 0} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/vecadd.cl]
!18 = metadata !{i32 786468, null, metadata !"int", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!19 = metadata !{i32 786478, i32 0, metadata !6, metadata !"vecadd_guarded", metadata !"vecadd_guarded", metadata !"", metadata !6, i32 11, metadata !20, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, i32)* @vecadd_guarded, null, null, metadata !22, i32 15} ; [ DW_TAG_subprogram ] [line 11] [def] [scope 15] [vecadd_guarded]
!20 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !21, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!21 = metadata !{null, metadata !9, metadata !9, metadata !9, metadata !18}
!22 = metadata !{metadata !23}
!23 = metadata !{metadata !24, metadata !25, metadata !26, metadata !27, metadata !28}
!24 = metadata !{i32 786689, metadata !19, metadata !"a", metadata !6, i32 16777227, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [a] [line 11]
!25 = metadata !{i32 786689, metadata !19, metadata !"b", metadata !6, i32 33554444, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [b] [line 12]
!26 = metadata !{i32 786689, metadata !19, metadata !"c", metadata !6, i32 50331661, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [c] [line 13]
!27 = metadata !{i32 786689, metadata !19, metadata !"n", metadata !6, i32 67108878, metadata !18, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [n] [line 14]
!28 = metadata !{i32 786688, metadata !29, metadata !"i", metadata !6, i32 16, metadata !18, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 16]
!29 = metadata !{i32 786443, metadata !19, i32 15, i32 0, metadata !6, i32 1} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/vecadd.cl]
!30 = metadata !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @vecadd, metadata !31, metadata !32, metadata !33, metadata !34, metadata !35}
!31 = metadata !{metadata !"kernel_arg_addr_space", i32 1, i32 1, i32 1}
!32 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"none", metadata !"none"}
!33 = metadata !{metadata !"kernel_arg_type", metadata !"float*", metadata !"float*", metadata !"float*"}
!34 = metadata !{metadata !"kernel_arg_type_qual", metadata !"", metadata !"", metadata !""}
!35 = metadata !{metadata !"kernel_arg_name", metadata !"a", metadata !"b", metadata !"c"}
!36 = metadata !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, i32)* @vecadd_guarded, metadata !37, metadata !38, metadata !39, metadata !40, metadata !41}
!37 = metadata !{metadata !"kernel_arg_addr_space", i32 1, i32 1, i32 1, i32 0}
!38 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"none", metadata !"none", metadata !"none"}
!39 = metadata !{metadata !"kernel_arg_type", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"int"}
!40 = metadata !{metadata !"kernel_arg_type_qual", metadata !"", metadata !"", metadata !"", metadata !""}
!41 = metadata !{metadata !"kernel_arg_name", metadata !"a", metadata !"b", metadata !"c", metadata !"n"}
!42 = metadata !{i32 3, i32 0, metadata !5, null}
!43 = metadata !{i32 4, i32 0, metadata !5, null}
!44 = metadata !{i32 5, i32 0, metadata !5, null}
!45 = metadata !{i32 7, i32 0, metadata !17, null}
!46 = metadata !{i32 8, i32 0, metadata !17, null}
!47 = metadata !{metadata !"float", metadata !48}
!48 = metadata !{metadata !"omnipotent char", metadata !49}
!49 = metadata !{metadata !"Simple C/C++ TBAA"}
!50 = metadata !{i32 9, i32 0, metadata !17, null}
!51 = metadata !{i32 11, i32 0, metadata !19, null}
!52 = metadata !{i32 12, i32 0, metadata !19, null}
!53 = metadata !{i32 13, i32 0, metadata !19, null}
!54 = metadata !{i32 14, i32 0, metadata !19, null}
!55 = metadata !{i32 16, i32 0, metadata !29, null}
!56 = metadata !{i32 17, i32 0, metadata !29, null}
!57 = metadata !{i32 22, i32 0, metadata !29, null}
!58 = metadata !{i32 23, i32 0, metadata !29, null}

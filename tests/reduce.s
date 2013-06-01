; ModuleID = 'reduce.cl'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir"

; Function Attrs: nounwind
define void @reduce(i32 %n, i32 addrspace(1)* nocapture %data, i32 addrspace(1)* nocapture %result, i32 addrspace(3)* nocapture %localData) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %n}, i64 0, metadata !11), !dbg !23
  tail call void @llvm.dbg.value(metadata !{i32 addrspace(1)* %data}, i64 0, metadata !12), !dbg !24
  tail call void @llvm.dbg.value(metadata !{i32 addrspace(1)* %result}, i64 0, metadata !13), !dbg !25
  tail call void @llvm.dbg.value(metadata !{i32 addrspace(3)* %localData}, i64 0, metadata !14), !dbg !26
  %call = tail call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 0) #3, !dbg !27
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !15), !dbg !27
  %call1 = tail call i32 bitcast (i32 (...)* @get_local_size to i32 (i32)*)(i32 0) #3, !dbg !28
  tail call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !16), !dbg !28
  tail call void @llvm.dbg.value(metadata !2, i64 0, metadata !17), !dbg !29
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !18), !dbg !30
  %cmp39 = icmp ult i32 %call, %n, !dbg !30
  br i1 %cmp39, label %for.body, label %for.end, !dbg !30

for.body:                                         ; preds = %entry, %for.body
  %i.041 = phi i32 [ %add2, %for.body ], [ %call, %entry ]
  %sum.040 = phi i32 [ %add, %for.body ], [ 0, %entry ]
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %data, i32 %i.041, !dbg !31
  %0 = load i32 addrspace(1)* %arrayidx, align 4, !dbg !31, !tbaa !33
  %add = add i32 %0, %sum.040, !dbg !31
  tail call void @llvm.dbg.value(metadata !{i32 %add}, i64 0, metadata !17), !dbg !31
  %add2 = add i32 %i.041, %call1, !dbg !30
  tail call void @llvm.dbg.value(metadata !{i32 %add2}, i64 0, metadata !18), !dbg !30
  %cmp = icmp ult i32 %add2, %n, !dbg !30
  br i1 %cmp, label %for.body, label %for.end, !dbg !30

for.end:                                          ; preds = %for.body, %entry
  %sum.0.lcssa = phi i32 [ 0, %entry ], [ %add, %for.body ]
  %arrayidx3 = getelementptr inbounds i32 addrspace(3)* %localData, i32 %call, !dbg !36
  store i32 %sum.0.lcssa, i32 addrspace(3)* %arrayidx3, align 4, !dbg !36, !tbaa !33
  %offset.036 = lshr i32 %call1, 1, !dbg !37
  %cmp537 = icmp eq i32 %offset.036, 0, !dbg !37
  br i1 %cmp537, label %for.end15, label %for.body6, !dbg !37

for.body6:                                        ; preds = %for.end, %for.cond4.backedge
  %offset.038 = phi i32 [ %offset.0, %for.cond4.backedge ], [ %offset.036, %for.end ]
  %call7 = tail call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #3, !dbg !38
  %cmp8 = icmp ult i32 %call, %offset.038, !dbg !40
  br i1 %cmp8, label %if.then, label %for.cond4.backedge, !dbg !40

for.cond4.backedge:                               ; preds = %for.body6, %if.then
  %offset.0 = lshr i32 %offset.038, 1, !dbg !37
  %cmp5 = icmp eq i32 %offset.0, 0, !dbg !37
  br i1 %cmp5, label %for.end15, label %for.body6, !dbg !37

if.then:                                          ; preds = %for.body6
  %add9 = add i32 %offset.038, %call, !dbg !41
  %arrayidx10 = getelementptr inbounds i32 addrspace(3)* %localData, i32 %add9, !dbg !41
  %1 = load i32 addrspace(3)* %arrayidx10, align 4, !dbg !41, !tbaa !33
  %2 = load i32 addrspace(3)* %arrayidx3, align 4, !dbg !41, !tbaa !33
  %add12 = add i32 %2, %1, !dbg !41
  store i32 %add12, i32 addrspace(3)* %arrayidx3, align 4, !dbg !41, !tbaa !33
  br label %for.cond4.backedge, !dbg !43

for.end15:                                        ; preds = %for.cond4.backedge, %for.end
  %cmp16 = icmp eq i32 %call, 0, !dbg !44
  br i1 %cmp16, label %if.then17, label %if.end19, !dbg !44

if.then17:                                        ; preds = %for.end15
  %3 = load i32 addrspace(3)* %arrayidx3, align 4, !dbg !45, !tbaa !33
  store i32 %3, i32 addrspace(1)* %result, align 4, !dbg !45, !tbaa !33
  br label %if.end19, !dbg !47

if.end19:                                         ; preds = %if.then17, %for.end15
  ret void, !dbg !48
}

declare i32 @get_local_id(...) #1

declare i32 @get_local_size(...) #1

declare i32 @barrier(...) #1

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #2

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
attributes #3 = { nobuiltin nounwind }

!llvm.dbg.cu = !{!0}
!opencl.kernels = !{!22}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.4 (trunk 182581)", i1 true, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !""} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/reduce.cl] [DW_LANG_C99]
!1 = metadata !{metadata !"reduce.cl", metadata !"/Users/james/projects/oclgrind/tests"}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"reduce", metadata !"reduce", metadata !"", i32 3, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(3)*)* @reduce, null, null, metadata !10, i32 7} ; [ DW_TAG_subprogram ] [line 3] [def] [scope 7] [reduce]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/Users/james/projects/oclgrind/tests/reduce.cl]
!6 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{null, metadata !8, metadata !9, metadata !9, metadata !9}
!8 = metadata !{i32 786468, null, null, metadata !"unsigned int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 7} ; [ DW_TAG_base_type ] [unsigned int] [line 0, size 32, align 32, offset 0, enc DW_ATE_unsigned]
!9 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, metadata !8} ; [ DW_TAG_pointer_type ] [line 0, size 32, align 32, offset 0] [from unsigned int]
!10 = metadata !{metadata !11, metadata !12, metadata !13, metadata !14, metadata !15, metadata !16, metadata !17, metadata !18, metadata !20}
!11 = metadata !{i32 786689, metadata !4, metadata !"n", metadata !5, i32 16777219, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [n] [line 3]
!12 = metadata !{i32 786689, metadata !4, metadata !"data", metadata !5, i32 33554436, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [data] [line 4]
!13 = metadata !{i32 786689, metadata !4, metadata !"result", metadata !5, i32 50331653, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [result] [line 5]
!14 = metadata !{i32 786689, metadata !4, metadata !"localData", metadata !5, i32 67108870, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [localData] [line 6]
!15 = metadata !{i32 786688, metadata !4, metadata !"lid", metadata !5, i32 8, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [lid] [line 8]
!16 = metadata !{i32 786688, metadata !4, metadata !"lsz", metadata !5, i32 9, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [lsz] [line 9]
!17 = metadata !{i32 786688, metadata !4, metadata !"sum", metadata !5, i32 10, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [sum] [line 10]
!18 = metadata !{i32 786688, metadata !19, metadata !"i", metadata !5, i32 11, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 11]
!19 = metadata !{i32 786443, metadata !1, metadata !4, i32 11, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/reduce.cl]
!20 = metadata !{i32 786688, metadata !21, metadata !"offset", metadata !5, i32 17, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [offset] [line 17]
!21 = metadata !{i32 786443, metadata !1, metadata !4, i32 17, i32 0, i32 2} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/reduce.cl]
!22 = metadata !{void (i32, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(3)*)* @reduce}
!23 = metadata !{i32 3, i32 0, metadata !4, null}
!24 = metadata !{i32 4, i32 0, metadata !4, null}
!25 = metadata !{i32 5, i32 0, metadata !4, null}
!26 = metadata !{i32 6, i32 0, metadata !4, null}
!27 = metadata !{i32 8, i32 0, metadata !4, null} ; [ DW_TAG_imported_declaration ]
!28 = metadata !{i32 9, i32 0, metadata !4, null}
!29 = metadata !{i32 10, i32 0, metadata !4, null}
!30 = metadata !{i32 11, i32 0, metadata !19, null}
!31 = metadata !{i32 13, i32 0, metadata !32, null}
!32 = metadata !{i32 786443, metadata !1, metadata !19, i32 12, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/reduce.cl]
!33 = metadata !{metadata !"int", metadata !34}
!34 = metadata !{metadata !"omnipotent char", metadata !35}
!35 = metadata !{metadata !"Simple C/C++ TBAA"}
!36 = metadata !{i32 16, i32 0, metadata !4, null}
!37 = metadata !{i32 17, i32 0, metadata !21, null}
!38 = metadata !{i32 19, i32 0, metadata !39, null}
!39 = metadata !{i32 786443, metadata !1, metadata !21, i32 18, i32 0, i32 3} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/reduce.cl]
!40 = metadata !{i32 20, i32 0, metadata !39, null}
!41 = metadata !{i32 22, i32 0, metadata !42, null}
!42 = metadata !{i32 786443, metadata !1, metadata !39, i32 21, i32 0, i32 4} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/reduce.cl]
!43 = metadata !{i32 23, i32 0, metadata !42, null}
!44 = metadata !{i32 26, i32 0, metadata !4, null}
!45 = metadata !{i32 28, i32 0, metadata !46, null}
!46 = metadata !{i32 786443, metadata !1, metadata !4, i32 27, i32 0, i32 5} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/reduce.cl]
!47 = metadata !{i32 29, i32 0, metadata !46, null}
!48 = metadata !{i32 30, i32 0, metadata !4, null}

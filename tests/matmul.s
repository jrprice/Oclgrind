; ModuleID = 'matmul.cl'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir"

; Function Attrs: nounwind
define void @matmul_elem(i32 %dim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %dim}, i64 0, metadata !13), !dbg !99
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !14), !dbg !100
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !15), !dbg !101
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !16), !dbg !102
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !103
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !18), !dbg !103
  %call1 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 1) #4, !dbg !104
  tail call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !19), !dbg !104
  %cmp = icmp slt i32 %call, %dim, !dbg !105
  %cmp2 = icmp slt i32 %call1, %dim, !dbg !105
  %or.cond = and i1 %cmp, %cmp2, !dbg !105
  br i1 %or.cond, label %for.cond.preheader, label %if.end, !dbg !105

for.cond.preheader:                               ; preds = %entry
  %cmp325 = icmp sgt i32 %dim, 0, !dbg !106
  %mul = mul nsw i32 %call, %dim, !dbg !109
  br i1 %cmp325, label %for.body, label %for.end, !dbg !106

for.body:                                         ; preds = %for.cond.preheader, %for.body
  %tmp.027 = phi float [ %add8, %for.body ], [ 0.000000e+00, %for.cond.preheader ]
  %k.026 = phi i32 [ %inc, %for.body ], [ 0, %for.cond.preheader ]
  %add = add nsw i32 %k.026, %mul, !dbg !109
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !109
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !109, !tbaa !111
  %mul4 = mul nsw i32 %k.026, %dim, !dbg !109
  %add5 = add nsw i32 %mul4, %call1, !dbg !109
  %arrayidx6 = getelementptr inbounds float addrspace(1)* %B, i32 %add5, !dbg !109
  %1 = load float addrspace(1)* %arrayidx6, align 4, !dbg !109, !tbaa !111
  %mul7 = fmul float %0, %1, !dbg !109
  %add8 = fadd float %tmp.027, %mul7, !dbg !109
  tail call void @llvm.dbg.value(metadata !{float %add8}, i64 0, metadata !20), !dbg !109
  %inc = add nsw i32 %k.026, 1, !dbg !106
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !17), !dbg !106
  %cmp3 = icmp slt i32 %inc, %dim, !dbg !106
  br i1 %cmp3, label %for.body, label %for.end, !dbg !106

for.end:                                          ; preds = %for.body, %for.cond.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond.preheader ], [ %add8, %for.body ]
  %add10 = add nsw i32 %mul, %call1, !dbg !114
  %arrayidx11 = getelementptr inbounds float addrspace(1)* %C, i32 %add10, !dbg !114
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx11, align 4, !dbg !114, !tbaa !111
  br label %if.end, !dbg !115

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !116
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

declare i32 @get_global_id(...) #2

; Function Attrs: nounwind
define void @matmul_row(i32 %dim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %dim}, i64 0, metadata !23), !dbg !117
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !24), !dbg !118
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !25), !dbg !119
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !26), !dbg !120
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !121
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !29), !dbg !121
  %cmp = icmp slt i32 %call, %dim, !dbg !122
  %cmp134 = icmp sgt i32 %dim, 0, !dbg !123
  %or.cond = and i1 %cmp, %cmp134, !dbg !122
  br i1 %or.cond, label %for.cond2.preheader.lr.ph, label %if.end, !dbg !122

for.cond2.preheader.lr.ph:                        ; preds = %entry
  %mul = mul nsw i32 %call, %dim, !dbg !126
  br label %for.body4.lr.ph, !dbg !123

for.body4.lr.ph:                                  ; preds = %for.end, %for.cond2.preheader.lr.ph
  %j.035 = phi i32 [ 0, %for.cond2.preheader.lr.ph ], [ %inc14, %for.end ]
  br label %for.body4, !dbg !129

for.body4:                                        ; preds = %for.body4.lr.ph, %for.body4
  %tmp.032 = phi float [ 0.000000e+00, %for.body4.lr.ph ], [ %add9, %for.body4 ]
  %k.031 = phi i32 [ 0, %for.body4.lr.ph ], [ %inc, %for.body4 ]
  %add = add nsw i32 %k.031, %mul, !dbg !126
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !126
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !126, !tbaa !111
  %mul5 = mul nsw i32 %k.031, %dim, !dbg !126
  %add6 = add nsw i32 %mul5, %j.035, !dbg !126
  %arrayidx7 = getelementptr inbounds float addrspace(1)* %B, i32 %add6, !dbg !126
  %1 = load float addrspace(1)* %arrayidx7, align 4, !dbg !126, !tbaa !111
  %mul8 = fmul float %0, %1, !dbg !126
  %add9 = fadd float %tmp.032, %mul8, !dbg !126
  tail call void @llvm.dbg.value(metadata !{float %add9}, i64 0, metadata !30), !dbg !126
  %inc = add nsw i32 %k.031, 1, !dbg !129
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !27), !dbg !129
  %cmp3 = icmp slt i32 %inc, %dim, !dbg !129
  br i1 %cmp3, label %for.body4, label %for.end, !dbg !129

for.end:                                          ; preds = %for.body4
  %add11 = add nsw i32 %j.035, %mul, !dbg !130
  %arrayidx12 = getelementptr inbounds float addrspace(1)* %C, i32 %add11, !dbg !130
  store float %add9, float addrspace(1)* %arrayidx12, align 4, !dbg !130, !tbaa !111
  %inc14 = add nsw i32 %j.035, 1, !dbg !123
  tail call void @llvm.dbg.value(metadata !{i32 %inc14}, i64 0, metadata !28), !dbg !123
  %cmp1 = icmp slt i32 %inc14, %dim, !dbg !123
  br i1 %cmp1, label %for.body4.lr.ph, label %if.end, !dbg !123

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !131
}

; Function Attrs: nounwind
define void @matmul_row_priv(i32 %dim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  %Awrk = alloca [16 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %dim}, i64 0, metadata !33), !dbg !132
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !34), !dbg !133
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !35), !dbg !134
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !36), !dbg !135
  %call = call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !136
  call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !39), !dbg !136
  %0 = bitcast [16 x float]* %Awrk to i8*, !dbg !137
  call void @llvm.lifetime.start(i64 64, i8* %0) #3, !dbg !137
  call void @llvm.dbg.declare(metadata !{[16 x float]* %Awrk}, metadata !40), !dbg !137
  %cmp = icmp slt i32 %call, %dim, !dbg !138
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !138

for.cond.preheader:                               ; preds = %entry
  %cmp148 = icmp sgt i32 %dim, 0, !dbg !139
  br i1 %cmp148, label %for.body.lr.ph, label %if.end, !dbg !139

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %call, %dim, !dbg !142
  br label %for.body, !dbg !139

for.cond3.preheader:                              ; preds = %for.body
  br i1 %cmp148, label %for.cond6.preheader.lr.ph, label %if.end, !dbg !143

for.cond6.preheader.lr.ph:                        ; preds = %for.cond3.preheader
  %mul18 = mul nsw i32 %call, %dim, !dbg !145
  br label %for.body8.lr.ph, !dbg !143

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.049 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.049, %mul, !dbg !142
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !142
  %1 = load float addrspace(1)* %arrayidx, align 4, !dbg !142, !tbaa !111
  %arrayidx2 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.049, !dbg !142
  store float %1, float* %arrayidx2, align 4, !dbg !142, !tbaa !111
  %inc = add nsw i32 %k.049, 1, !dbg !139
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !37), !dbg !139
  %cmp1 = icmp slt i32 %inc, %dim, !dbg !139
  br i1 %cmp1, label %for.body, label %for.cond3.preheader, !dbg !139

for.body8.lr.ph:                                  ; preds = %for.end17, %for.cond6.preheader.lr.ph
  %j.047 = phi i32 [ 0, %for.cond6.preheader.lr.ph ], [ %inc22, %for.end17 ]
  br label %for.body8, !dbg !147

for.body8:                                        ; preds = %for.body8.lr.ph, %for.body8
  %tmp.045 = phi float [ 0.000000e+00, %for.body8.lr.ph ], [ %add14, %for.body8 ]
  %k.144 = phi i32 [ 0, %for.body8.lr.ph ], [ %inc16, %for.body8 ]
  %arrayidx9 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.144, !dbg !149
  %2 = load float* %arrayidx9, align 4, !dbg !149, !tbaa !111
  %mul10 = mul nsw i32 %k.144, %dim, !dbg !149
  %add11 = add nsw i32 %mul10, %j.047, !dbg !149
  %arrayidx12 = getelementptr inbounds float addrspace(1)* %B, i32 %add11, !dbg !149
  %3 = load float addrspace(1)* %arrayidx12, align 4, !dbg !149, !tbaa !111
  %mul13 = fmul float %2, %3, !dbg !149
  %add14 = fadd float %tmp.045, %mul13, !dbg !149
  call void @llvm.dbg.value(metadata !{float %add14}, i64 0, metadata !44), !dbg !149
  %inc16 = add nsw i32 %k.144, 1, !dbg !147
  call void @llvm.dbg.value(metadata !{i32 %inc16}, i64 0, metadata !37), !dbg !147
  %cmp7 = icmp slt i32 %inc16, %dim, !dbg !147
  br i1 %cmp7, label %for.body8, label %for.end17, !dbg !147

for.end17:                                        ; preds = %for.body8
  %add19 = add nsw i32 %j.047, %mul18, !dbg !145
  %arrayidx20 = getelementptr inbounds float addrspace(1)* %C, i32 %add19, !dbg !145
  store float %add14, float addrspace(1)* %arrayidx20, align 4, !dbg !145, !tbaa !111
  %inc22 = add nsw i32 %j.047, 1, !dbg !143
  call void @llvm.dbg.value(metadata !{i32 %inc22}, i64 0, metadata !38), !dbg !143
  %cmp4 = icmp slt i32 %inc22, %dim, !dbg !143
  br i1 %cmp4, label %for.body8.lr.ph, label %if.end, !dbg !143

if.end:                                           ; preds = %for.cond.preheader, %for.cond3.preheader, %for.end17, %entry
  call void @llvm.lifetime.end(i64 64, i8* %0) #3, !dbg !150
  ret void, !dbg !150
}

; Function Attrs: nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #3

; Function Attrs: nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #3

; Function Attrs: nounwind
define void @matmul_row_local(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C, float addrspace(3)* nocapture %Bwrk) #0 {
entry:
  %Awrk = alloca [1024 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !49), !dbg !151
  call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !50), !dbg !152
  call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !51), !dbg !153
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !52), !dbg !154
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !53), !dbg !155
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !54), !dbg !156
  call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bwrk}, i64 0, metadata !55), !dbg !157
  %call = call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !158
  call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !58), !dbg !158
  %call1 = call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 0) #4, !dbg !159
  call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !59), !dbg !159
  %call2 = call i32 bitcast (i32 (...)* @get_local_size to i32 (i32)*)(i32 0) #4, !dbg !160
  call void @llvm.dbg.value(metadata !{i32 %call2}, i64 0, metadata !60), !dbg !160
  %0 = bitcast [1024 x float]* %Awrk to i8*, !dbg !161
  call void @llvm.lifetime.start(i64 4096, i8* %0) #3, !dbg !161
  call void @llvm.dbg.declare(metadata !{[1024 x float]* %Awrk}, metadata !61), !dbg !161
  %cmp = icmp slt i32 %call, %Ndim, !dbg !162
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !162

for.cond.preheader:                               ; preds = %entry
  %cmp365 = icmp sgt i32 %Pdim, 0, !dbg !163
  br i1 %cmp365, label %for.body.lr.ph, label %for.cond5.preheader, !dbg !163

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %call, %Ndim, !dbg !166
  br label %for.body, !dbg !163

for.cond5.preheader:                              ; preds = %for.body, %for.cond.preheader
  %cmp663 = icmp sgt i32 %Mdim, 0, !dbg !167
  br i1 %cmp663, label %for.cond8.preheader.lr.ph, label %if.end, !dbg !167

for.cond8.preheader.lr.ph:                        ; preds = %for.cond5.preheader
  %cmp958 = icmp slt i32 %call1, %Pdim, !dbg !169
  %mul29 = mul nsw i32 %call, %Ndim, !dbg !172
  br label %for.cond8.preheader, !dbg !167

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.066 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.066, %mul, !dbg !166
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !166
  %1 = load float addrspace(1)* %arrayidx, align 4, !dbg !166, !tbaa !111
  %arrayidx4 = getelementptr inbounds [1024 x float]* %Awrk, i32 0, i32 %k.066, !dbg !166
  store float %1, float* %arrayidx4, align 4, !dbg !166, !tbaa !111
  %inc = add nsw i32 %k.066, 1, !dbg !163
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !56), !dbg !163
  %cmp3 = icmp slt i32 %inc, %Pdim, !dbg !163
  br i1 %cmp3, label %for.body, label %for.cond5.preheader, !dbg !163

for.cond8.preheader:                              ; preds = %for.cond8.preheader.lr.ph, %for.end28
  %j.064 = phi i32 [ 0, %for.cond8.preheader.lr.ph ], [ %inc33, %for.end28 ]
  br i1 %cmp958, label %for.body10, label %for.end17, !dbg !169

for.body10:                                       ; preds = %for.cond8.preheader, %for.body10
  %k.159 = phi i32 [ %add16, %for.body10 ], [ %call1, %for.cond8.preheader ]
  %mul11 = mul nsw i32 %k.159, %Pdim, !dbg !173
  %add12 = add nsw i32 %mul11, %j.064, !dbg !173
  %arrayidx13 = getelementptr inbounds float addrspace(1)* %B, i32 %add12, !dbg !173
  %2 = load float addrspace(1)* %arrayidx13, align 4, !dbg !173, !tbaa !111
  %arrayidx14 = getelementptr inbounds float addrspace(3)* %Bwrk, i32 %k.159, !dbg !173
  store float %2, float addrspace(3)* %arrayidx14, align 4, !dbg !173, !tbaa !111
  %add16 = add nsw i32 %k.159, %call2, !dbg !169
  call void @llvm.dbg.value(metadata !{i32 %add16}, i64 0, metadata !56), !dbg !169
  %cmp9 = icmp slt i32 %add16, %Pdim, !dbg !169
  br i1 %cmp9, label %for.body10, label %for.end17, !dbg !169

for.end17:                                        ; preds = %for.body10, %for.cond8.preheader
  %call18 = call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !174
  call void @llvm.dbg.value(metadata !175, i64 0, metadata !65), !dbg !176
  call void @llvm.dbg.value(metadata !2, i64 0, metadata !56), !dbg !177
  br i1 %cmp365, label %for.body21, label %for.end28, !dbg !177

for.body21:                                       ; preds = %for.end17, %for.body21
  %tmp.062 = phi float [ %add25, %for.body21 ], [ 0.000000e+00, %for.end17 ]
  %k.261 = phi i32 [ %inc27, %for.body21 ], [ 0, %for.end17 ]
  %arrayidx22 = getelementptr inbounds [1024 x float]* %Awrk, i32 0, i32 %k.261, !dbg !179
  %3 = load float* %arrayidx22, align 4, !dbg !179, !tbaa !111
  %arrayidx23 = getelementptr inbounds float addrspace(3)* %Bwrk, i32 %k.261, !dbg !179
  %4 = load float addrspace(3)* %arrayidx23, align 4, !dbg !179, !tbaa !111
  %mul24 = fmul float %3, %4, !dbg !179
  %add25 = fadd float %tmp.062, %mul24, !dbg !179
  call void @llvm.dbg.value(metadata !{float %add25}, i64 0, metadata !65), !dbg !179
  %inc27 = add nsw i32 %k.261, 1, !dbg !177
  call void @llvm.dbg.value(metadata !{i32 %inc27}, i64 0, metadata !56), !dbg !177
  %cmp20 = icmp slt i32 %inc27, %Pdim, !dbg !177
  br i1 %cmp20, label %for.body21, label %for.end28, !dbg !177

for.end28:                                        ; preds = %for.body21, %for.end17
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.end17 ], [ %add25, %for.body21 ]
  %add30 = add nsw i32 %j.064, %mul29, !dbg !172
  %arrayidx31 = getelementptr inbounds float addrspace(1)* %C, i32 %add30, !dbg !172
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx31, align 4, !dbg !172, !tbaa !111
  %inc33 = add nsw i32 %j.064, 1, !dbg !167
  call void @llvm.dbg.value(metadata !{i32 %inc33}, i64 0, metadata !57), !dbg !167
  %cmp6 = icmp slt i32 %inc33, %Mdim, !dbg !167
  br i1 %cmp6, label %for.cond8.preheader, label %if.end, !dbg !167

if.end:                                           ; preds = %for.cond5.preheader, %for.end28, %entry
  call void @llvm.lifetime.end(i64 4096, i8* %0) #3, !dbg !180
  ret void, !dbg !180
}

declare i32 @get_local_id(...) #2

declare i32 @get_local_size(...) #2

declare i32 @barrier(...) #2

; Function Attrs: nounwind
define void @matmul_block(i32 %uiWA, i32 %uiWB, i32 %uiWC, float addrspace(1)* nocapture %C, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(3)* nocapture %As, float addrspace(3)* nocapture %Bs) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %uiWA}, i64 0, metadata !70), !dbg !181
  tail call void @llvm.dbg.value(metadata !{i32 %uiWB}, i64 0, metadata !71), !dbg !181
  tail call void @llvm.dbg.value(metadata !{i32 %uiWC}, i64 0, metadata !72), !dbg !181
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !73), !dbg !182
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !74), !dbg !182
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !75), !dbg !182
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %As}, i64 0, metadata !76), !dbg !183
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bs}, i64 0, metadata !77), !dbg !183
  %call = tail call i32 bitcast (i32 (...)* @get_group_id to i32 (i32)*)(i32 0) #4, !dbg !184
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !78), !dbg !184
  %call1 = tail call i32 bitcast (i32 (...)* @get_group_id to i32 (i32)*)(i32 1) #4, !dbg !185
  tail call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !79), !dbg !185
  %call2 = tail call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 0) #4, !dbg !186
  tail call void @llvm.dbg.value(metadata !{i32 %call2}, i64 0, metadata !80), !dbg !186
  %call3 = tail call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 1) #4, !dbg !187
  tail call void @llvm.dbg.value(metadata !{i32 %call3}, i64 0, metadata !81), !dbg !187
  %mul = shl nsw i32 %uiWA, 4, !dbg !188
  %mul4 = mul nsw i32 %call1, %mul, !dbg !188
  tail call void @llvm.dbg.value(metadata !{i32 %mul4}, i64 0, metadata !82), !dbg !188
  %add = add i32 %uiWA, -1, !dbg !189
  %sub = add i32 %add, %mul4, !dbg !189
  tail call void @llvm.dbg.value(metadata !{i32 %sub}, i64 0, metadata !83), !dbg !189
  tail call void @llvm.dbg.value(metadata !190, i64 0, metadata !84), !dbg !191
  tail call void @llvm.dbg.value(metadata !{i32 %mul5}, i64 0, metadata !85), !dbg !192
  %mul6 = shl i32 %uiWB, 4, !dbg !193
  tail call void @llvm.dbg.value(metadata !{i32 %mul6}, i64 0, metadata !86), !dbg !193
  tail call void @llvm.dbg.value(metadata !175, i64 0, metadata !87), !dbg !194
  tail call void @llvm.dbg.value(metadata !{i32 %mul4}, i64 0, metadata !88), !dbg !195
  tail call void @llvm.dbg.value(metadata !{i32 %mul5}, i64 0, metadata !90), !dbg !195
  %cmp66 = icmp sgt i32 %mul4, %sub, !dbg !195
  br i1 %cmp66, label %for.end36, label %for.body.lr.ph, !dbg !195

for.body.lr.ph:                                   ; preds = %entry
  %mul5 = shl nsw i32 %call, 4, !dbg !192
  %mul7 = mul nsw i32 %call3, %uiWA, !dbg !196
  %add8 = add i32 %mul7, %call2, !dbg !196
  %mul10 = shl nsw i32 %call3, 4, !dbg !196
  %add11 = add nsw i32 %mul10, %call2, !dbg !196
  %arrayidx12 = getelementptr inbounds float addrspace(3)* %As, i32 %add11, !dbg !196
  %mul13 = mul nsw i32 %call3, %uiWB, !dbg !197
  %add14 = add i32 %mul13, %call2, !dbg !197
  %arrayidx19 = getelementptr inbounds float addrspace(3)* %Bs, i32 %add11, !dbg !197
  br label %for.body, !dbg !195

for.body:                                         ; preds = %for.body.lr.ph, %for.end
  %b.069 = phi i32 [ %mul5, %for.body.lr.ph ], [ %add35, %for.end ]
  %a.068 = phi i32 [ %mul4, %for.body.lr.ph ], [ %add34, %for.end ]
  %Csub.067 = phi float [ 0.000000e+00, %for.body.lr.ph ], [ %add31, %for.end ]
  %add9 = add i32 %add8, %a.068, !dbg !196
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add9, !dbg !196
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !196, !tbaa !111
  store float %0, float addrspace(3)* %arrayidx12, align 4, !dbg !196, !tbaa !111
  %add15 = add i32 %add14, %b.069, !dbg !197
  %arrayidx16 = getelementptr inbounds float addrspace(1)* %B, i32 %add15, !dbg !197
  %1 = load float addrspace(1)* %arrayidx16, align 4, !dbg !197, !tbaa !111
  store float %1, float addrspace(3)* %arrayidx19, align 4, !dbg !197, !tbaa !111
  %call20 = tail call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !198
  tail call void @llvm.dbg.value(metadata !2, i64 0, metadata !91), !dbg !199
  br label %for.body23, !dbg !199

for.body23:                                       ; preds = %for.body, %for.body23
  %k.065 = phi i32 [ 0, %for.body ], [ %inc, %for.body23 ]
  %Csub.164 = phi float [ %Csub.067, %for.body ], [ %add31, %for.body23 ]
  %add25 = add nsw i32 %k.065, %mul10, !dbg !200
  %arrayidx26 = getelementptr inbounds float addrspace(3)* %As, i32 %add25, !dbg !200
  %2 = load float addrspace(3)* %arrayidx26, align 4, !dbg !200, !tbaa !111
  %mul27 = shl i32 %k.065, 4, !dbg !200
  %add28 = add nsw i32 %mul27, %call2, !dbg !200
  %arrayidx29 = getelementptr inbounds float addrspace(3)* %Bs, i32 %add28, !dbg !200
  %3 = load float addrspace(3)* %arrayidx29, align 4, !dbg !200, !tbaa !111
  %mul30 = fmul float %2, %3, !dbg !200
  %add31 = fadd float %Csub.164, %mul30, !dbg !200
  tail call void @llvm.dbg.value(metadata !{float %add31}, i64 0, metadata !87), !dbg !200
  %inc = add nsw i32 %k.065, 1, !dbg !199
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !91), !dbg !199
  %cmp22 = icmp slt i32 %inc, 16, !dbg !199
  br i1 %cmp22, label %for.body23, label %for.end, !dbg !199

for.end:                                          ; preds = %for.body23
  %call32 = tail call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !201
  %add34 = add nsw i32 %a.068, 16, !dbg !202
  tail call void @llvm.dbg.value(metadata !{i32 %add34}, i64 0, metadata !88), !dbg !202
  %add35 = add nsw i32 %b.069, %mul6, !dbg !202
  tail call void @llvm.dbg.value(metadata !{i32 %add35}, i64 0, metadata !90), !dbg !202
  %cmp = icmp sgt i32 %add34, %sub, !dbg !195
  br i1 %cmp, label %for.end36, label %for.body, !dbg !195

for.end36:                                        ; preds = %for.end, %entry
  %Csub.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %add31, %for.end ]
  %call37 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 1) #4, !dbg !203
  %call38 = tail call i32 bitcast (i32 (...)* @get_global_size to i32 (i32)*)(i32 0) #4, !dbg !203
  %mul39 = mul nsw i32 %call38, %call37, !dbg !203
  %call40 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !203
  %add41 = add nsw i32 %mul39, %call40, !dbg !203
  %arrayidx42 = getelementptr inbounds float addrspace(1)* %C, i32 %add41, !dbg !203
  store float %Csub.0.lcssa, float addrspace(1)* %arrayidx42, align 4, !dbg !203, !tbaa !111
  ret void, !dbg !204
}

declare i32 @get_group_id(...) #2

declare i32 @get_global_size(...) #2

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }
attributes #4 = { nobuiltin nounwind }

!llvm.dbg.cu = !{!0}
!opencl.kernels = !{!94, !95, !96, !97, !98}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.4 (trunk 182581)", i1 true, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !""} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/matmul.cl] [DW_LANG_C99]
!1 = metadata !{metadata !"matmul.cl", metadata !"/Users/james/projects/oclgrind/tests"}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4, metadata !21, metadata !31, metadata !45, metadata !66}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_elem", metadata !"matmul_elem", metadata !"", i32 3, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_elem, null, null, metadata !12, i32 11} ; [ DW_TAG_subprogram ] [line 3] [def] [scope 11] [matmul_elem]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!6 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{null, metadata !8, metadata !10, metadata !10, metadata !10}
!8 = metadata !{i32 786470, null, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, metadata !9} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from int]
!9 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!10 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, metadata !11} ; [ DW_TAG_pointer_type ] [line 0, size 32, align 32, offset 0] [from float]
!11 = metadata !{i32 786468, null, null, metadata !"float", i32 0, i64 32, i64 32, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [float] [line 0, size 32, align 32, offset 0, enc DW_ATE_float]
!12 = metadata !{metadata !13, metadata !14, metadata !15, metadata !16, metadata !17, metadata !18, metadata !19, metadata !20}
!13 = metadata !{i32 786689, metadata !4, metadata !"dim", metadata !5, i32 16777220, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [dim] [line 4]
!14 = metadata !{i32 786689, metadata !4, metadata !"A", metadata !5, i32 33554440, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 8]
!15 = metadata !{i32 786689, metadata !4, metadata !"B", metadata !5, i32 50331657, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 9]
!16 = metadata !{i32 786689, metadata !4, metadata !"C", metadata !5, i32 67108874, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 10]
!17 = metadata !{i32 786688, metadata !4, metadata !"k", metadata !5, i32 12, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 12]
!18 = metadata !{i32 786688, metadata !4, metadata !"i", metadata !5, i32 13, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 13]
!19 = metadata !{i32 786688, metadata !4, metadata !"j", metadata !5, i32 14, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 14]
!20 = metadata !{i32 786688, metadata !4, metadata !"tmp", metadata !5, i32 15, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 15]
!21 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_row", metadata !"matmul_row", metadata !"", i32 31, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row, null, null, metadata !22, i32 39} ; [ DW_TAG_subprogram ] [line 31] [def] [scope 39] [matmul_row]
!22 = metadata !{metadata !23, metadata !24, metadata !25, metadata !26, metadata !27, metadata !28, metadata !29, metadata !30}
!23 = metadata !{i32 786689, metadata !21, metadata !"dim", metadata !5, i32 16777248, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [dim] [line 32]
!24 = metadata !{i32 786689, metadata !21, metadata !"A", metadata !5, i32 33554468, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 36]
!25 = metadata !{i32 786689, metadata !21, metadata !"B", metadata !5, i32 50331685, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 37]
!26 = metadata !{i32 786689, metadata !21, metadata !"C", metadata !5, i32 67108902, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 38]
!27 = metadata !{i32 786688, metadata !21, metadata !"k", metadata !5, i32 40, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 40]
!28 = metadata !{i32 786688, metadata !21, metadata !"j", metadata !5, i32 40, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 40]
!29 = metadata !{i32 786688, metadata !21, metadata !"i", metadata !5, i32 41, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 41]
!30 = metadata !{i32 786688, metadata !21, metadata !"tmp", metadata !5, i32 42, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 42]
!31 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_row_priv", metadata !"matmul_row_priv", metadata !"", i32 59, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row_priv, null, null, metadata !32, i32 67} ; [ DW_TAG_subprogram ] [line 59] [def] [scope 67] [matmul_row_priv]
!32 = metadata !{metadata !33, metadata !34, metadata !35, metadata !36, metadata !37, metadata !38, metadata !39, metadata !40, metadata !44}
!33 = metadata !{i32 786689, metadata !31, metadata !"dim", metadata !5, i32 16777276, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [dim] [line 60]
!34 = metadata !{i32 786689, metadata !31, metadata !"A", metadata !5, i32 33554496, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 64]
!35 = metadata !{i32 786689, metadata !31, metadata !"B", metadata !5, i32 50331713, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 65]
!36 = metadata !{i32 786689, metadata !31, metadata !"C", metadata !5, i32 67108930, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 66]
!37 = metadata !{i32 786688, metadata !31, metadata !"k", metadata !5, i32 68, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 68]
!38 = metadata !{i32 786688, metadata !31, metadata !"j", metadata !5, i32 68, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 68]
!39 = metadata !{i32 786688, metadata !31, metadata !"i", metadata !5, i32 69, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 69]
!40 = metadata !{i32 786688, metadata !31, metadata !"Awrk", metadata !5, i32 70, metadata !41, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Awrk] [line 70]
!41 = metadata !{i32 786433, null, null, metadata !"", i32 0, i64 512, i64 32, i32 0, i32 0, metadata !11, metadata !42, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 512, align 32, offset 0] [from float]
!42 = metadata !{metadata !43}
!43 = metadata !{i32 786465, i64 0, i64 16}       ; [ DW_TAG_subrange_type ] [0, 15]
!44 = metadata !{i32 786688, metadata !31, metadata !"tmp", metadata !5, i32 71, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 71]
!45 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_row_local", metadata !"matmul_row_local", metadata !"", i32 93, metadata !46, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local, null, null, metadata !48, i32 101} ; [ DW_TAG_subprogram ] [line 93] [def] [scope 101] [matmul_row_local]
!46 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !47, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!47 = metadata !{null, metadata !8, metadata !8, metadata !8, metadata !10, metadata !10, metadata !10, metadata !10}
!48 = metadata !{metadata !49, metadata !50, metadata !51, metadata !52, metadata !53, metadata !54, metadata !55, metadata !56, metadata !57, metadata !58, metadata !59, metadata !60, metadata !61, metadata !65}
!49 = metadata !{i32 786689, metadata !45, metadata !"Mdim", metadata !5, i32 16777310, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 94]
!50 = metadata !{i32 786689, metadata !45, metadata !"Ndim", metadata !5, i32 33554527, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 95]
!51 = metadata !{i32 786689, metadata !45, metadata !"Pdim", metadata !5, i32 50331744, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 96]
!52 = metadata !{i32 786689, metadata !45, metadata !"A", metadata !5, i32 67108961, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 97]
!53 = metadata !{i32 786689, metadata !45, metadata !"B", metadata !5, i32 83886178, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 98]
!54 = metadata !{i32 786689, metadata !45, metadata !"C", metadata !5, i32 100663395, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 99]
!55 = metadata !{i32 786689, metadata !45, metadata !"Bwrk", metadata !5, i32 117440612, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bwrk] [line 100]
!56 = metadata !{i32 786688, metadata !45, metadata !"k", metadata !5, i32 102, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 102]
!57 = metadata !{i32 786688, metadata !45, metadata !"j", metadata !5, i32 102, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 102]
!58 = metadata !{i32 786688, metadata !45, metadata !"i", metadata !5, i32 103, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 103]
!59 = metadata !{i32 786688, metadata !45, metadata !"iloc", metadata !5, i32 104, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [iloc] [line 104]
!60 = metadata !{i32 786688, metadata !45, metadata !"nloc", metadata !5, i32 105, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [nloc] [line 105]
!61 = metadata !{i32 786688, metadata !45, metadata !"Awrk", metadata !5, i32 106, metadata !62, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Awrk] [line 106]
!62 = metadata !{i32 786433, null, null, metadata !"", i32 0, i64 32768, i64 32, i32 0, i32 0, metadata !11, metadata !63, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 32768, align 32, offset 0] [from float]
!63 = metadata !{metadata !64}
!64 = metadata !{i32 786465, i64 0, i64 1024}     ; [ DW_TAG_subrange_type ] [0, 1023]
!65 = metadata !{i32 786688, metadata !45, metadata !"tmp", metadata !5, i32 107, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 107]
!66 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_block", metadata !"matmul_block", metadata !"", i32 148, metadata !67, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block, null, null, metadata !69, i32 151} ; [ DW_TAG_subprogram ] [line 148] [def] [scope 151] [matmul_block]
!67 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !68, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!68 = metadata !{null, metadata !9, metadata !9, metadata !9, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10}
!69 = metadata !{metadata !70, metadata !71, metadata !72, metadata !73, metadata !74, metadata !75, metadata !76, metadata !77, metadata !78, metadata !79, metadata !80, metadata !81, metadata !82, metadata !83, metadata !84, metadata !85, metadata !86, metadata !87, metadata !88, metadata !90, metadata !91}
!70 = metadata !{i32 786689, metadata !66, metadata !"uiWA", metadata !5, i32 16777364, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWA] [line 148]
!71 = metadata !{i32 786689, metadata !66, metadata !"uiWB", metadata !5, i32 33554580, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWB] [line 148]
!72 = metadata !{i32 786689, metadata !66, metadata !"uiWC", metadata !5, i32 50331796, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWC] [line 148]
!73 = metadata !{i32 786689, metadata !66, metadata !"C", metadata !5, i32 67109013, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 149]
!74 = metadata !{i32 786689, metadata !66, metadata !"A", metadata !5, i32 83886229, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 149]
!75 = metadata !{i32 786689, metadata !66, metadata !"B", metadata !5, i32 100663445, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 149]
!76 = metadata !{i32 786689, metadata !66, metadata !"As", metadata !5, i32 117440662, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [As] [line 150]
!77 = metadata !{i32 786689, metadata !66, metadata !"Bs", metadata !5, i32 134217878, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bs] [line 150]
!78 = metadata !{i32 786688, metadata !66, metadata !"bx", metadata !5, i32 153, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bx] [line 153]
!79 = metadata !{i32 786688, metadata !66, metadata !"by", metadata !5, i32 154, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [by] [line 154]
!80 = metadata !{i32 786688, metadata !66, metadata !"tx", metadata !5, i32 157, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tx] [line 157]
!81 = metadata !{i32 786688, metadata !66, metadata !"ty", metadata !5, i32 158, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [ty] [line 158]
!82 = metadata !{i32 786688, metadata !66, metadata !"aBegin", metadata !5, i32 161, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aBegin] [line 161]
!83 = metadata !{i32 786688, metadata !66, metadata !"aEnd", metadata !5, i32 164, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aEnd] [line 164]
!84 = metadata !{i32 786688, metadata !66, metadata !"aStep", metadata !5, i32 167, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aStep] [line 167]
!85 = metadata !{i32 786688, metadata !66, metadata !"bBegin", metadata !5, i32 170, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bBegin] [line 170]
!86 = metadata !{i32 786688, metadata !66, metadata !"bStep", metadata !5, i32 173, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bStep] [line 173]
!87 = metadata !{i32 786688, metadata !66, metadata !"Csub", metadata !5, i32 177, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Csub] [line 177]
!88 = metadata !{i32 786688, metadata !89, metadata !"a", metadata !5, i32 181, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [a] [line 181]
!89 = metadata !{i32 786443, metadata !1, metadata !66, i32 181, i32 0, i32 18} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!90 = metadata !{i32 786688, metadata !89, metadata !"b", metadata !5, i32 181, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [b] [line 181]
!91 = metadata !{i32 786688, metadata !92, metadata !"k", metadata !5, i32 198, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 198]
!92 = metadata !{i32 786443, metadata !1, metadata !93, i32 198, i32 0, i32 20} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!93 = metadata !{i32 786443, metadata !1, metadata !89, i32 183, i32 0, i32 19} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!94 = metadata !{void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_elem}
!95 = metadata !{void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row}
!96 = metadata !{void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row_priv}
!97 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local}
!98 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block}
!99 = metadata !{i32 4, i32 0, metadata !4, null}
!100 = metadata !{i32 8, i32 0, metadata !4, null} ; [ DW_TAG_imported_declaration ]
!101 = metadata !{i32 9, i32 0, metadata !4, null}
!102 = metadata !{i32 10, i32 0, metadata !4, null}
!103 = metadata !{i32 13, i32 0, metadata !4, null}
!104 = metadata !{i32 14, i32 0, metadata !4, null}
!105 = metadata !{i32 17, i32 0, metadata !4, null}
!106 = metadata !{i32 21, i32 0, metadata !107, null}
!107 = metadata !{i32 786443, metadata !1, metadata !108, i32 21, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!108 = metadata !{i32 786443, metadata !1, metadata !4, i32 18, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!109 = metadata !{i32 24, i32 0, metadata !110, null}
!110 = metadata !{i32 786443, metadata !1, metadata !107, i32 22, i32 0, i32 2} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!111 = metadata !{metadata !"float", metadata !112}
!112 = metadata !{metadata !"omnipotent char", metadata !113}
!113 = metadata !{metadata !"Simple C/C++ TBAA"}
!114 = metadata !{i32 27, i32 0, metadata !108, null}
!115 = metadata !{i32 28, i32 0, metadata !108, null}
!116 = metadata !{i32 29, i32 0, metadata !4, null}
!117 = metadata !{i32 32, i32 0, metadata !21, null}
!118 = metadata !{i32 36, i32 0, metadata !21, null}
!119 = metadata !{i32 37, i32 0, metadata !21, null}
!120 = metadata !{i32 38, i32 0, metadata !21, null}
!121 = metadata !{i32 41, i32 0, metadata !21, null}
!122 = metadata !{i32 44, i32 0, metadata !21, null}
!123 = metadata !{i32 47, i32 0, metadata !124, null}
!124 = metadata !{i32 786443, metadata !1, metadata !125, i32 47, i32 0, i32 4} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!125 = metadata !{i32 786443, metadata !1, metadata !21, i32 45, i32 0, i32 3} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!126 = metadata !{i32 52, i32 0, metadata !127, null}
!127 = metadata !{i32 786443, metadata !1, metadata !128, i32 50, i32 0, i32 6} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!128 = metadata !{i32 786443, metadata !1, metadata !124, i32 47, i32 0, i32 5} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!129 = metadata !{i32 50, i32 0, metadata !127, null}
!130 = metadata !{i32 54, i32 0, metadata !128, null}
!131 = metadata !{i32 57, i32 0, metadata !21, null}
!132 = metadata !{i32 60, i32 0, metadata !31, null}
!133 = metadata !{i32 64, i32 0, metadata !31, null}
!134 = metadata !{i32 65, i32 0, metadata !31, null}
!135 = metadata !{i32 66, i32 0, metadata !31, null}
!136 = metadata !{i32 69, i32 0, metadata !31, null}
!137 = metadata !{i32 70, i32 0, metadata !31, null}
!138 = metadata !{i32 73, i32 0, metadata !31, null}
!139 = metadata !{i32 76, i32 0, metadata !140, null}
!140 = metadata !{i32 786443, metadata !1, metadata !141, i32 76, i32 0, i32 8} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!141 = metadata !{i32 786443, metadata !1, metadata !31, i32 74, i32 0, i32 7} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!142 = metadata !{i32 78, i32 0, metadata !140, null}
!143 = metadata !{i32 81, i32 0, metadata !144, null}
!144 = metadata !{i32 786443, metadata !1, metadata !141, i32 81, i32 0, i32 9} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!145 = metadata !{i32 88, i32 0, metadata !146, null}
!146 = metadata !{i32 786443, metadata !1, metadata !144, i32 81, i32 0, i32 10} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!147 = metadata !{i32 84, i32 0, metadata !148, null}
!148 = metadata !{i32 786443, metadata !1, metadata !146, i32 84, i32 0, i32 11} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!149 = metadata !{i32 86, i32 0, metadata !148, null}
!150 = metadata !{i32 91, i32 0, metadata !31, null}
!151 = metadata !{i32 94, i32 0, metadata !45, null}
!152 = metadata !{i32 95, i32 0, metadata !45, null}
!153 = metadata !{i32 96, i32 0, metadata !45, null}
!154 = metadata !{i32 97, i32 0, metadata !45, null}
!155 = metadata !{i32 98, i32 0, metadata !45, null}
!156 = metadata !{i32 99, i32 0, metadata !45, null}
!157 = metadata !{i32 100, i32 0, metadata !45, null}
!158 = metadata !{i32 103, i32 0, metadata !45, null}
!159 = metadata !{i32 104, i32 0, metadata !45, null}
!160 = metadata !{i32 105, i32 0, metadata !45, null}
!161 = metadata !{i32 106, i32 0, metadata !45, null}
!162 = metadata !{i32 108, i32 0, metadata !45, null}
!163 = metadata !{i32 110, i32 0, metadata !164, null}
!164 = metadata !{i32 786443, metadata !1, metadata !165, i32 110, i32 0, i32 13} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!165 = metadata !{i32 786443, metadata !1, metadata !45, i32 109, i32 0, i32 12} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!166 = metadata !{i32 111, i32 0, metadata !164, null}
!167 = metadata !{i32 113, i32 0, metadata !168, null}
!168 = metadata !{i32 786443, metadata !1, metadata !165, i32 113, i32 0, i32 14} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!169 = metadata !{i32 114, i32 0, metadata !170, null}
!170 = metadata !{i32 786443, metadata !1, metadata !171, i32 114, i32 0, i32 16} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!171 = metadata !{i32 786443, metadata !1, metadata !168, i32 113, i32 0, i32 15} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!172 = metadata !{i32 120, i32 0, metadata !171, null}
!173 = metadata !{i32 115, i32 0, metadata !170, null}
!174 = metadata !{i32 116, i32 0, metadata !171, null}
!175 = metadata !{float 0.000000e+00}
!176 = metadata !{i32 117, i32 0, metadata !171, null}
!177 = metadata !{i32 118, i32 0, metadata !178, null}
!178 = metadata !{i32 786443, metadata !1, metadata !171, i32 118, i32 0, i32 17} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!179 = metadata !{i32 119, i32 0, metadata !178, null}
!180 = metadata !{i32 123, i32 0, metadata !45, null}
!181 = metadata !{i32 148, i32 0, metadata !66, null}
!182 = metadata !{i32 149, i32 0, metadata !66, null}
!183 = metadata !{i32 150, i32 0, metadata !66, null}
!184 = metadata !{i32 153, i32 0, metadata !66, null}
!185 = metadata !{i32 154, i32 0, metadata !66, null}
!186 = metadata !{i32 157, i32 0, metadata !66, null}
!187 = metadata !{i32 158, i32 0, metadata !66, null}
!188 = metadata !{i32 161, i32 0, metadata !66, null}
!189 = metadata !{i32 164, i32 0, metadata !66, null}
!190 = metadata !{i32 16}
!191 = metadata !{i32 167, i32 0, metadata !66, null}
!192 = metadata !{i32 170, i32 0, metadata !66, null}
!193 = metadata !{i32 173, i32 0, metadata !66, null}
!194 = metadata !{i32 177, i32 0, metadata !66, null}
!195 = metadata !{i32 181, i32 0, metadata !89, null}
!196 = metadata !{i32 188, i32 0, metadata !93, null}
!197 = metadata !{i32 189, i32 0, metadata !93, null}
!198 = metadata !{i32 192, i32 0, metadata !93, null}
!199 = metadata !{i32 198, i32 0, metadata !92, null}
!200 = metadata !{i32 199, i32 0, metadata !92, null}
!201 = metadata !{i32 204, i32 0, metadata !93, null}
!202 = metadata !{i32 183, i32 0, metadata !89, null}
!203 = metadata !{i32 209, i32 0, metadata !66, null}
!204 = metadata !{i32 211, i32 0, metadata !66, null}

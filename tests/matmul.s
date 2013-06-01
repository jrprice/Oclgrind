; ModuleID = 'matmul.cl'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir"

; Function Attrs: nounwind
define void @matmul_elem(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !13), !dbg !102
  tail call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !14), !dbg !103
  tail call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !15), !dbg !104
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !16), !dbg !105
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !17), !dbg !106
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !18), !dbg !107
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !108
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !20), !dbg !108
  %call1 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 1) #4, !dbg !109
  tail call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !21), !dbg !109
  %cmp = icmp slt i32 %call, %Ndim, !dbg !110
  %cmp2 = icmp slt i32 %call1, %Mdim, !dbg !110
  %or.cond = and i1 %cmp, %cmp2, !dbg !110
  br i1 %or.cond, label %for.cond.preheader, label %if.end, !dbg !110

for.cond.preheader:                               ; preds = %entry
  %cmp323 = icmp sgt i32 %Pdim, 0, !dbg !111
  %mul = mul nsw i32 %call, %Ndim, !dbg !114
  br i1 %cmp323, label %for.body, label %for.end, !dbg !111

for.body:                                         ; preds = %for.cond.preheader, %for.body
  %tmp.025 = phi float [ %add8, %for.body ], [ 0.000000e+00, %for.cond.preheader ]
  %k.024 = phi i32 [ %inc, %for.body ], [ 0, %for.cond.preheader ]
  %add = add nsw i32 %k.024, %mul, !dbg !114
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !114
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !114, !tbaa !116
  %mul4 = mul nsw i32 %k.024, %Pdim, !dbg !114
  %add5 = add nsw i32 %mul4, %call1, !dbg !114
  %arrayidx6 = getelementptr inbounds float addrspace(1)* %B, i32 %add5, !dbg !114
  %1 = load float addrspace(1)* %arrayidx6, align 4, !dbg !114, !tbaa !116
  %mul7 = fmul float %0, %1, !dbg !114
  %add8 = fadd float %tmp.025, %mul7, !dbg !114
  tail call void @llvm.dbg.value(metadata !{float %add8}, i64 0, metadata !22), !dbg !114
  %inc = add nsw i32 %k.024, 1, !dbg !111
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !19), !dbg !111
  %cmp3 = icmp slt i32 %inc, %Pdim, !dbg !111
  br i1 %cmp3, label %for.body, label %for.end, !dbg !111

for.end:                                          ; preds = %for.body, %for.cond.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond.preheader ], [ %add8, %for.body ]
  %add10 = add nsw i32 %mul, %call1, !dbg !119
  %arrayidx11 = getelementptr inbounds float addrspace(1)* %C, i32 %add10, !dbg !119
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx11, align 4, !dbg !119, !tbaa !116
  br label %if.end, !dbg !120

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !121
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

declare i32 @get_global_id(...) #2

; Function Attrs: nounwind
define void @matmul_row(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !25), !dbg !122
  tail call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !26), !dbg !123
  tail call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !27), !dbg !124
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !28), !dbg !125
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !29), !dbg !126
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !30), !dbg !127
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !128
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !33), !dbg !128
  %cmp = icmp slt i32 %call, %Ndim, !dbg !129
  %cmp132 = icmp sgt i32 %Mdim, 0, !dbg !130
  %or.cond = and i1 %cmp, %cmp132, !dbg !129
  br i1 %or.cond, label %for.cond2.preheader.lr.ph, label %if.end, !dbg !129

for.cond2.preheader.lr.ph:                        ; preds = %entry
  %cmp328 = icmp sgt i32 %Pdim, 0, !dbg !133
  %mul = mul nsw i32 %call, %Ndim, !dbg !136
  br label %for.cond2.preheader, !dbg !130

for.cond2.preheader:                              ; preds = %for.cond2.preheader.lr.ph, %for.end
  %j.033 = phi i32 [ 0, %for.cond2.preheader.lr.ph ], [ %inc14, %for.end ]
  br i1 %cmp328, label %for.body4, label %for.end, !dbg !133

for.body4:                                        ; preds = %for.cond2.preheader, %for.body4
  %tmp.030 = phi float [ %add9, %for.body4 ], [ 0.000000e+00, %for.cond2.preheader ]
  %k.029 = phi i32 [ %inc, %for.body4 ], [ 0, %for.cond2.preheader ]
  %add = add nsw i32 %k.029, %mul, !dbg !136
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !136
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !136, !tbaa !116
  %mul5 = mul nsw i32 %k.029, %Pdim, !dbg !136
  %add6 = add nsw i32 %mul5, %j.033, !dbg !136
  %arrayidx7 = getelementptr inbounds float addrspace(1)* %B, i32 %add6, !dbg !136
  %1 = load float addrspace(1)* %arrayidx7, align 4, !dbg !136, !tbaa !116
  %mul8 = fmul float %0, %1, !dbg !136
  %add9 = fadd float %tmp.030, %mul8, !dbg !136
  tail call void @llvm.dbg.value(metadata !{float %add9}, i64 0, metadata !34), !dbg !136
  %inc = add nsw i32 %k.029, 1, !dbg !133
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !31), !dbg !133
  %cmp3 = icmp slt i32 %inc, %Pdim, !dbg !133
  br i1 %cmp3, label %for.body4, label %for.end, !dbg !133

for.end:                                          ; preds = %for.body4, %for.cond2.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond2.preheader ], [ %add9, %for.body4 ]
  %add11 = add nsw i32 %j.033, %mul, !dbg !137
  %arrayidx12 = getelementptr inbounds float addrspace(1)* %C, i32 %add11, !dbg !137
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx12, align 4, !dbg !137, !tbaa !116
  %inc14 = add nsw i32 %j.033, 1, !dbg !130
  tail call void @llvm.dbg.value(metadata !{i32 %inc14}, i64 0, metadata !32), !dbg !130
  %cmp1 = icmp slt i32 %inc14, %Mdim, !dbg !130
  br i1 %cmp1, label %for.cond2.preheader, label %if.end, !dbg !130

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !138
}

; Function Attrs: nounwind
define void @matmul_row_priv(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  %Awrk = alloca [16 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !37), !dbg !139
  call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !38), !dbg !140
  call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !39), !dbg !141
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !40), !dbg !142
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !41), !dbg !143
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !42), !dbg !144
  %call = call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !145
  call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !45), !dbg !145
  %0 = bitcast [16 x float]* %Awrk to i8*, !dbg !146
  call void @llvm.lifetime.start(i64 64, i8* %0) #3, !dbg !146
  call void @llvm.dbg.declare(metadata !{[16 x float]* %Awrk}, metadata !46), !dbg !146
  %cmp = icmp slt i32 %call, %Ndim, !dbg !147
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !147

for.cond.preheader:                               ; preds = %entry
  %cmp146 = icmp sgt i32 %Pdim, 0, !dbg !148
  br i1 %cmp146, label %for.body.lr.ph, label %for.cond3.preheader, !dbg !148

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %call, %Ndim, !dbg !151
  br label %for.body, !dbg !148

for.cond3.preheader:                              ; preds = %for.body, %for.cond.preheader
  %cmp444 = icmp sgt i32 %Mdim, 0, !dbg !152
  br i1 %cmp444, label %for.cond6.preheader.lr.ph, label %if.end, !dbg !152

for.cond6.preheader.lr.ph:                        ; preds = %for.cond3.preheader
  %mul18 = mul nsw i32 %call, %Ndim, !dbg !154
  br label %for.cond6.preheader, !dbg !152

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.047 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.047, %mul, !dbg !151
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !151
  %1 = load float addrspace(1)* %arrayidx, align 4, !dbg !151, !tbaa !116
  %arrayidx2 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.047, !dbg !151
  store float %1, float* %arrayidx2, align 4, !dbg !151, !tbaa !116
  %inc = add nsw i32 %k.047, 1, !dbg !148
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !43), !dbg !148
  %cmp1 = icmp slt i32 %inc, %Pdim, !dbg !148
  br i1 %cmp1, label %for.body, label %for.cond3.preheader, !dbg !148

for.cond6.preheader:                              ; preds = %for.cond6.preheader.lr.ph, %for.end17
  %j.045 = phi i32 [ 0, %for.cond6.preheader.lr.ph ], [ %inc22, %for.end17 ]
  br i1 %cmp146, label %for.body8, label %for.end17, !dbg !156

for.body8:                                        ; preds = %for.cond6.preheader, %for.body8
  %tmp.043 = phi float [ %add14, %for.body8 ], [ 0.000000e+00, %for.cond6.preheader ]
  %k.142 = phi i32 [ %inc16, %for.body8 ], [ 0, %for.cond6.preheader ]
  %arrayidx9 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.142, !dbg !158
  %2 = load float* %arrayidx9, align 4, !dbg !158, !tbaa !116
  %mul10 = mul nsw i32 %k.142, %Pdim, !dbg !158
  %add11 = add nsw i32 %mul10, %j.045, !dbg !158
  %arrayidx12 = getelementptr inbounds float addrspace(1)* %B, i32 %add11, !dbg !158
  %3 = load float addrspace(1)* %arrayidx12, align 4, !dbg !158, !tbaa !116
  %mul13 = fmul float %2, %3, !dbg !158
  %add14 = fadd float %tmp.043, %mul13, !dbg !158
  call void @llvm.dbg.value(metadata !{float %add14}, i64 0, metadata !50), !dbg !158
  %inc16 = add nsw i32 %k.142, 1, !dbg !156
  call void @llvm.dbg.value(metadata !{i32 %inc16}, i64 0, metadata !43), !dbg !156
  %cmp7 = icmp slt i32 %inc16, %Pdim, !dbg !156
  br i1 %cmp7, label %for.body8, label %for.end17, !dbg !156

for.end17:                                        ; preds = %for.body8, %for.cond6.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond6.preheader ], [ %add14, %for.body8 ]
  %add19 = add nsw i32 %j.045, %mul18, !dbg !154
  %arrayidx20 = getelementptr inbounds float addrspace(1)* %C, i32 %add19, !dbg !154
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx20, align 4, !dbg !154, !tbaa !116
  %inc22 = add nsw i32 %j.045, 1, !dbg !152
  call void @llvm.dbg.value(metadata !{i32 %inc22}, i64 0, metadata !44), !dbg !152
  %cmp4 = icmp slt i32 %inc22, %Mdim, !dbg !152
  br i1 %cmp4, label %for.cond6.preheader, label %if.end, !dbg !152

if.end:                                           ; preds = %for.cond3.preheader, %for.end17, %entry
  call void @llvm.lifetime.end(i64 64, i8* %0) #3, !dbg !159
  ret void, !dbg !159
}

; Function Attrs: nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #3

; Function Attrs: nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #3

; Function Attrs: nounwind
define void @matmul_row_local(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C, float addrspace(3)* nocapture %Bwrk) #0 {
entry:
  %Awrk = alloca [16 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !55), !dbg !160
  call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !56), !dbg !161
  call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !57), !dbg !162
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !58), !dbg !163
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !59), !dbg !164
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !60), !dbg !165
  call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bwrk}, i64 0, metadata !61), !dbg !166
  %call = call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !167
  call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !64), !dbg !167
  %call1 = call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 0) #4, !dbg !168
  call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !65), !dbg !168
  %call2 = call i32 bitcast (i32 (...)* @get_local_size to i32 (i32)*)(i32 0) #4, !dbg !169
  call void @llvm.dbg.value(metadata !{i32 %call2}, i64 0, metadata !66), !dbg !169
  %0 = bitcast [16 x float]* %Awrk to i8*, !dbg !170
  call void @llvm.lifetime.start(i64 64, i8* %0) #3, !dbg !170
  call void @llvm.dbg.declare(metadata !{[16 x float]* %Awrk}, metadata !67), !dbg !170
  %cmp = icmp slt i32 %call, %Ndim, !dbg !171
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !171

for.cond.preheader:                               ; preds = %entry
  %cmp366 = icmp sgt i32 %Pdim, 0, !dbg !172
  br i1 %cmp366, label %for.body.lr.ph, label %for.cond5.preheader, !dbg !172

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %call, %Ndim, !dbg !175
  br label %for.body, !dbg !172

for.cond5.preheader:                              ; preds = %for.body, %for.cond.preheader
  %cmp664 = icmp sgt i32 %Mdim, 0, !dbg !176
  br i1 %cmp664, label %for.cond8.preheader.lr.ph, label %if.end, !dbg !176

for.cond8.preheader.lr.ph:                        ; preds = %for.cond5.preheader
  %cmp959 = icmp slt i32 %call1, %Pdim, !dbg !178
  %mul29 = mul nsw i32 %call, %Ndim, !dbg !181
  br label %for.cond8.preheader, !dbg !176

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.067 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.067, %mul, !dbg !175
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !175
  %1 = load float addrspace(1)* %arrayidx, align 4, !dbg !175, !tbaa !116
  %arrayidx4 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.067, !dbg !175
  store float %1, float* %arrayidx4, align 4, !dbg !175, !tbaa !116
  %inc = add nsw i32 %k.067, 1, !dbg !172
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !62), !dbg !172
  %cmp3 = icmp slt i32 %inc, %Pdim, !dbg !172
  br i1 %cmp3, label %for.body, label %for.cond5.preheader, !dbg !172

for.cond8.preheader:                              ; preds = %for.cond8.preheader.lr.ph, %for.end28
  %j.065 = phi i32 [ 0, %for.cond8.preheader.lr.ph ], [ %inc34, %for.end28 ]
  br i1 %cmp959, label %for.body10, label %for.end17, !dbg !178

for.body10:                                       ; preds = %for.cond8.preheader, %for.body10
  %k.160 = phi i32 [ %add16, %for.body10 ], [ %call1, %for.cond8.preheader ]
  %mul11 = mul nsw i32 %k.160, %Pdim, !dbg !182
  %add12 = add nsw i32 %mul11, %j.065, !dbg !182
  %arrayidx13 = getelementptr inbounds float addrspace(1)* %B, i32 %add12, !dbg !182
  %2 = load float addrspace(1)* %arrayidx13, align 4, !dbg !182, !tbaa !116
  %arrayidx14 = getelementptr inbounds float addrspace(3)* %Bwrk, i32 %k.160, !dbg !182
  store float %2, float addrspace(3)* %arrayidx14, align 4, !dbg !182, !tbaa !116
  %add16 = add nsw i32 %k.160, %call2, !dbg !178
  call void @llvm.dbg.value(metadata !{i32 %add16}, i64 0, metadata !62), !dbg !178
  %cmp9 = icmp slt i32 %add16, %Pdim, !dbg !178
  br i1 %cmp9, label %for.body10, label %for.end17, !dbg !178

for.end17:                                        ; preds = %for.body10, %for.cond8.preheader
  %call18 = call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !183
  call void @llvm.dbg.value(metadata !184, i64 0, metadata !68), !dbg !185
  call void @llvm.dbg.value(metadata !2, i64 0, metadata !62), !dbg !186
  br i1 %cmp366, label %for.body21, label %for.end28, !dbg !186

for.body21:                                       ; preds = %for.end17, %for.body21
  %tmp.063 = phi float [ %add25, %for.body21 ], [ 0.000000e+00, %for.end17 ]
  %k.262 = phi i32 [ %inc27, %for.body21 ], [ 0, %for.end17 ]
  %arrayidx22 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.262, !dbg !188
  %3 = load float* %arrayidx22, align 4, !dbg !188, !tbaa !116
  %arrayidx23 = getelementptr inbounds float addrspace(3)* %Bwrk, i32 %k.262, !dbg !188
  %4 = load float addrspace(3)* %arrayidx23, align 4, !dbg !188, !tbaa !116
  %mul24 = fmul float %3, %4, !dbg !188
  %add25 = fadd float %tmp.063, %mul24, !dbg !188
  call void @llvm.dbg.value(metadata !{float %add25}, i64 0, metadata !68), !dbg !188
  %inc27 = add nsw i32 %k.262, 1, !dbg !186
  call void @llvm.dbg.value(metadata !{i32 %inc27}, i64 0, metadata !62), !dbg !186
  %cmp20 = icmp slt i32 %inc27, %Pdim, !dbg !186
  br i1 %cmp20, label %for.body21, label %for.end28, !dbg !186

for.end28:                                        ; preds = %for.body21, %for.end17
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.end17 ], [ %add25, %for.body21 ]
  %add30 = add nsw i32 %j.065, %mul29, !dbg !181
  %arrayidx31 = getelementptr inbounds float addrspace(1)* %C, i32 %add30, !dbg !181
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx31, align 4, !dbg !181, !tbaa !116
  %call32 = call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !189
  %inc34 = add nsw i32 %j.065, 1, !dbg !176
  call void @llvm.dbg.value(metadata !{i32 %inc34}, i64 0, metadata !63), !dbg !176
  %cmp6 = icmp slt i32 %inc34, %Mdim, !dbg !176
  br i1 %cmp6, label %for.cond8.preheader, label %if.end, !dbg !176

if.end:                                           ; preds = %for.cond5.preheader, %for.end28, %entry
  call void @llvm.lifetime.end(i64 64, i8* %0) #3, !dbg !190
  ret void, !dbg !190
}

declare i32 @get_local_id(...) #2

declare i32 @get_local_size(...) #2

declare i32 @barrier(...) #2

; Function Attrs: nounwind
define void @matmul_block(i32 %uiWA, i32 %uiWB, i32 %uiWC, float addrspace(1)* nocapture %C, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(3)* nocapture %As, float addrspace(3)* nocapture %Bs) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %uiWA}, i64 0, metadata !73), !dbg !191
  tail call void @llvm.dbg.value(metadata !{i32 %uiWB}, i64 0, metadata !74), !dbg !191
  tail call void @llvm.dbg.value(metadata !{i32 %uiWC}, i64 0, metadata !75), !dbg !191
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !76), !dbg !192
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !77), !dbg !192
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !78), !dbg !192
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %As}, i64 0, metadata !79), !dbg !193
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bs}, i64 0, metadata !80), !dbg !193
  %call = tail call i32 bitcast (i32 (...)* @get_group_id to i32 (i32)*)(i32 0) #4, !dbg !194
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !81), !dbg !194
  %call1 = tail call i32 bitcast (i32 (...)* @get_group_id to i32 (i32)*)(i32 1) #4, !dbg !195
  tail call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !82), !dbg !195
  %call2 = tail call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 0) #4, !dbg !196
  tail call void @llvm.dbg.value(metadata !{i32 %call2}, i64 0, metadata !83), !dbg !196
  %call3 = tail call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 1) #4, !dbg !197
  tail call void @llvm.dbg.value(metadata !{i32 %call3}, i64 0, metadata !84), !dbg !197
  %mul = shl nsw i32 %uiWA, 4, !dbg !198
  %mul4 = mul nsw i32 %call1, %mul, !dbg !198
  tail call void @llvm.dbg.value(metadata !{i32 %mul4}, i64 0, metadata !85), !dbg !198
  %add = add i32 %uiWA, -1, !dbg !199
  %sub = add i32 %add, %mul4, !dbg !199
  tail call void @llvm.dbg.value(metadata !{i32 %sub}, i64 0, metadata !86), !dbg !199
  tail call void @llvm.dbg.value(metadata !200, i64 0, metadata !87), !dbg !201
  tail call void @llvm.dbg.value(metadata !{i32 %mul5}, i64 0, metadata !88), !dbg !202
  %mul6 = shl i32 %uiWB, 4, !dbg !203
  tail call void @llvm.dbg.value(metadata !{i32 %mul6}, i64 0, metadata !89), !dbg !203
  tail call void @llvm.dbg.value(metadata !184, i64 0, metadata !90), !dbg !204
  tail call void @llvm.dbg.value(metadata !{i32 %mul4}, i64 0, metadata !91), !dbg !205
  tail call void @llvm.dbg.value(metadata !{i32 %mul5}, i64 0, metadata !93), !dbg !205
  %cmp66 = icmp sgt i32 %mul4, %sub, !dbg !205
  br i1 %cmp66, label %for.end36, label %for.body.lr.ph, !dbg !205

for.body.lr.ph:                                   ; preds = %entry
  %mul5 = shl nsw i32 %call, 4, !dbg !202
  %mul7 = mul nsw i32 %call3, %uiWA, !dbg !206
  %add8 = add i32 %mul7, %call2, !dbg !206
  %mul10 = shl nsw i32 %call3, 4, !dbg !206
  %add11 = add nsw i32 %mul10, %call2, !dbg !206
  %arrayidx12 = getelementptr inbounds float addrspace(3)* %As, i32 %add11, !dbg !206
  %mul13 = mul nsw i32 %call3, %uiWB, !dbg !207
  %add14 = add i32 %mul13, %call2, !dbg !207
  %arrayidx19 = getelementptr inbounds float addrspace(3)* %Bs, i32 %add11, !dbg !207
  br label %for.body, !dbg !205

for.body:                                         ; preds = %for.body.lr.ph, %for.end
  %b.069 = phi i32 [ %mul5, %for.body.lr.ph ], [ %add35, %for.end ]
  %a.068 = phi i32 [ %mul4, %for.body.lr.ph ], [ %add34, %for.end ]
  %Csub.067 = phi float [ 0.000000e+00, %for.body.lr.ph ], [ %add31, %for.end ]
  %add9 = add i32 %add8, %a.068, !dbg !206
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add9, !dbg !206
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !206, !tbaa !116
  store float %0, float addrspace(3)* %arrayidx12, align 4, !dbg !206, !tbaa !116
  %add15 = add i32 %add14, %b.069, !dbg !207
  %arrayidx16 = getelementptr inbounds float addrspace(1)* %B, i32 %add15, !dbg !207
  %1 = load float addrspace(1)* %arrayidx16, align 4, !dbg !207, !tbaa !116
  store float %1, float addrspace(3)* %arrayidx19, align 4, !dbg !207, !tbaa !116
  %call20 = tail call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !208
  tail call void @llvm.dbg.value(metadata !2, i64 0, metadata !94), !dbg !209
  br label %for.body23, !dbg !209

for.body23:                                       ; preds = %for.body, %for.body23
  %k.065 = phi i32 [ 0, %for.body ], [ %inc, %for.body23 ]
  %Csub.164 = phi float [ %Csub.067, %for.body ], [ %add31, %for.body23 ]
  %add25 = add nsw i32 %k.065, %mul10, !dbg !210
  %arrayidx26 = getelementptr inbounds float addrspace(3)* %As, i32 %add25, !dbg !210
  %2 = load float addrspace(3)* %arrayidx26, align 4, !dbg !210, !tbaa !116
  %mul27 = shl i32 %k.065, 4, !dbg !210
  %add28 = add nsw i32 %mul27, %call2, !dbg !210
  %arrayidx29 = getelementptr inbounds float addrspace(3)* %Bs, i32 %add28, !dbg !210
  %3 = load float addrspace(3)* %arrayidx29, align 4, !dbg !210, !tbaa !116
  %mul30 = fmul float %2, %3, !dbg !210
  %add31 = fadd float %Csub.164, %mul30, !dbg !210
  tail call void @llvm.dbg.value(metadata !{float %add31}, i64 0, metadata !90), !dbg !210
  %inc = add nsw i32 %k.065, 1, !dbg !209
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !94), !dbg !209
  %cmp22 = icmp slt i32 %inc, 16, !dbg !209
  br i1 %cmp22, label %for.body23, label %for.end, !dbg !209

for.end:                                          ; preds = %for.body23
  %call32 = tail call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !211
  %add34 = add nsw i32 %a.068, 16, !dbg !212
  tail call void @llvm.dbg.value(metadata !{i32 %add34}, i64 0, metadata !91), !dbg !212
  %add35 = add nsw i32 %b.069, %mul6, !dbg !212
  tail call void @llvm.dbg.value(metadata !{i32 %add35}, i64 0, metadata !93), !dbg !212
  %cmp = icmp sgt i32 %add34, %sub, !dbg !205
  br i1 %cmp, label %for.end36, label %for.body, !dbg !205

for.end36:                                        ; preds = %for.end, %entry
  %Csub.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %add31, %for.end ]
  %call37 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 1) #4, !dbg !213
  %call38 = tail call i32 bitcast (i32 (...)* @get_global_size to i32 (i32)*)(i32 0) #4, !dbg !213
  %mul39 = mul nsw i32 %call38, %call37, !dbg !213
  %call40 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !213
  %add41 = add nsw i32 %mul39, %call40, !dbg !213
  %arrayidx42 = getelementptr inbounds float addrspace(1)* %C, i32 %add41, !dbg !213
  store float %Csub.0.lcssa, float addrspace(1)* %arrayidx42, align 4, !dbg !213, !tbaa !116
  ret void, !dbg !214
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
!opencl.kernels = !{!97, !98, !99, !100, !101}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.4 (trunk 182581)", i1 true, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !""} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/matmul.cl] [DW_LANG_C99]
!1 = metadata !{metadata !"matmul.cl", metadata !"/Users/james/projects/oclgrind/tests"}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4, metadata !23, metadata !35, metadata !51, metadata !69}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_elem", metadata !"matmul_elem", metadata !"", i32 3, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_elem, null, null, metadata !12, i32 10} ; [ DW_TAG_subprogram ] [line 3] [def] [scope 10] [matmul_elem]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!6 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{null, metadata !8, metadata !8, metadata !8, metadata !10, metadata !10, metadata !10}
!8 = metadata !{i32 786470, null, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, metadata !9} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from int]
!9 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!10 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, metadata !11} ; [ DW_TAG_pointer_type ] [line 0, size 32, align 32, offset 0] [from float]
!11 = metadata !{i32 786468, null, null, metadata !"float", i32 0, i64 32, i64 32, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [float] [line 0, size 32, align 32, offset 0, enc DW_ATE_float]
!12 = metadata !{metadata !13, metadata !14, metadata !15, metadata !16, metadata !17, metadata !18, metadata !19, metadata !20, metadata !21, metadata !22}
!13 = metadata !{i32 786689, metadata !4, metadata !"Mdim", metadata !5, i32 16777220, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 4]
!14 = metadata !{i32 786689, metadata !4, metadata !"Ndim", metadata !5, i32 33554437, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 5]
!15 = metadata !{i32 786689, metadata !4, metadata !"Pdim", metadata !5, i32 50331654, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 6]
!16 = metadata !{i32 786689, metadata !4, metadata !"A", metadata !5, i32 67108871, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 7]
!17 = metadata !{i32 786689, metadata !4, metadata !"B", metadata !5, i32 83886088, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 8]
!18 = metadata !{i32 786689, metadata !4, metadata !"C", metadata !5, i32 100663305, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 9]
!19 = metadata !{i32 786688, metadata !4, metadata !"k", metadata !5, i32 11, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 11]
!20 = metadata !{i32 786688, metadata !4, metadata !"i", metadata !5, i32 12, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 12]
!21 = metadata !{i32 786688, metadata !4, metadata !"j", metadata !5, i32 13, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 13]
!22 = metadata !{i32 786688, metadata !4, metadata !"tmp", metadata !5, i32 14, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 14]
!23 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_row", metadata !"matmul_row", metadata !"", i32 26, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row, null, null, metadata !24, i32 33} ; [ DW_TAG_subprogram ] [line 26] [def] [scope 33] [matmul_row]
!24 = metadata !{metadata !25, metadata !26, metadata !27, metadata !28, metadata !29, metadata !30, metadata !31, metadata !32, metadata !33, metadata !34}
!25 = metadata !{i32 786689, metadata !23, metadata !"Mdim", metadata !5, i32 16777243, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 27]
!26 = metadata !{i32 786689, metadata !23, metadata !"Ndim", metadata !5, i32 33554460, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 28]
!27 = metadata !{i32 786689, metadata !23, metadata !"Pdim", metadata !5, i32 50331677, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 29]
!28 = metadata !{i32 786689, metadata !23, metadata !"A", metadata !5, i32 67108894, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 30]
!29 = metadata !{i32 786689, metadata !23, metadata !"B", metadata !5, i32 83886111, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 31]
!30 = metadata !{i32 786689, metadata !23, metadata !"C", metadata !5, i32 100663328, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 32]
!31 = metadata !{i32 786688, metadata !23, metadata !"k", metadata !5, i32 34, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 34]
!32 = metadata !{i32 786688, metadata !23, metadata !"j", metadata !5, i32 34, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 34]
!33 = metadata !{i32 786688, metadata !23, metadata !"i", metadata !5, i32 35, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 35]
!34 = metadata !{i32 786688, metadata !23, metadata !"tmp", metadata !5, i32 36, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 36]
!35 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_row_priv", metadata !"matmul_row_priv", metadata !"", i32 48, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row_priv, null, null, metadata !36, i32 55} ; [ DW_TAG_subprogram ] [line 48] [def] [scope 55] [matmul_row_priv]
!36 = metadata !{metadata !37, metadata !38, metadata !39, metadata !40, metadata !41, metadata !42, metadata !43, metadata !44, metadata !45, metadata !46, metadata !50}
!37 = metadata !{i32 786689, metadata !35, metadata !"Mdim", metadata !5, i32 16777265, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 49]
!38 = metadata !{i32 786689, metadata !35, metadata !"Ndim", metadata !5, i32 33554482, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 50]
!39 = metadata !{i32 786689, metadata !35, metadata !"Pdim", metadata !5, i32 50331699, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 51]
!40 = metadata !{i32 786689, metadata !35, metadata !"A", metadata !5, i32 67108916, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 52]
!41 = metadata !{i32 786689, metadata !35, metadata !"B", metadata !5, i32 83886133, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 53]
!42 = metadata !{i32 786689, metadata !35, metadata !"C", metadata !5, i32 100663350, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 54]
!43 = metadata !{i32 786688, metadata !35, metadata !"k", metadata !5, i32 56, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 56]
!44 = metadata !{i32 786688, metadata !35, metadata !"j", metadata !5, i32 56, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 56]
!45 = metadata !{i32 786688, metadata !35, metadata !"i", metadata !5, i32 57, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 57]
!46 = metadata !{i32 786688, metadata !35, metadata !"Awrk", metadata !5, i32 58, metadata !47, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Awrk] [line 58]
!47 = metadata !{i32 786433, null, null, metadata !"", i32 0, i64 512, i64 32, i32 0, i32 0, metadata !11, metadata !48, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 512, align 32, offset 0] [from float]
!48 = metadata !{metadata !49}
!49 = metadata !{i32 786465, i64 0, i64 16}       ; [ DW_TAG_subrange_type ] [0, 15]
!50 = metadata !{i32 786688, metadata !35, metadata !"tmp", metadata !5, i32 59, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 59]
!51 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_row_local", metadata !"matmul_row_local", metadata !"", i32 74, metadata !52, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local, null, null, metadata !54, i32 82} ; [ DW_TAG_subprogram ] [line 74] [def] [scope 82] [matmul_row_local]
!52 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !53, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!53 = metadata !{null, metadata !8, metadata !8, metadata !8, metadata !10, metadata !10, metadata !10, metadata !10}
!54 = metadata !{metadata !55, metadata !56, metadata !57, metadata !58, metadata !59, metadata !60, metadata !61, metadata !62, metadata !63, metadata !64, metadata !65, metadata !66, metadata !67, metadata !68}
!55 = metadata !{i32 786689, metadata !51, metadata !"Mdim", metadata !5, i32 16777291, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 75]
!56 = metadata !{i32 786689, metadata !51, metadata !"Ndim", metadata !5, i32 33554508, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 76]
!57 = metadata !{i32 786689, metadata !51, metadata !"Pdim", metadata !5, i32 50331725, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 77]
!58 = metadata !{i32 786689, metadata !51, metadata !"A", metadata !5, i32 67108942, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 78]
!59 = metadata !{i32 786689, metadata !51, metadata !"B", metadata !5, i32 83886159, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 79]
!60 = metadata !{i32 786689, metadata !51, metadata !"C", metadata !5, i32 100663376, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 80]
!61 = metadata !{i32 786689, metadata !51, metadata !"Bwrk", metadata !5, i32 117440593, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bwrk] [line 81]
!62 = metadata !{i32 786688, metadata !51, metadata !"k", metadata !5, i32 83, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 83]
!63 = metadata !{i32 786688, metadata !51, metadata !"j", metadata !5, i32 83, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 83]
!64 = metadata !{i32 786688, metadata !51, metadata !"i", metadata !5, i32 84, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 84]
!65 = metadata !{i32 786688, metadata !51, metadata !"iloc", metadata !5, i32 85, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [iloc] [line 85]
!66 = metadata !{i32 786688, metadata !51, metadata !"nloc", metadata !5, i32 86, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [nloc] [line 86]
!67 = metadata !{i32 786688, metadata !51, metadata !"Awrk", metadata !5, i32 87, metadata !47, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Awrk] [line 87]
!68 = metadata !{i32 786688, metadata !51, metadata !"tmp", metadata !5, i32 88, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 88]
!69 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_block", metadata !"matmul_block", metadata !"", i32 130, metadata !70, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block, null, null, metadata !72, i32 133} ; [ DW_TAG_subprogram ] [line 130] [def] [scope 133] [matmul_block]
!70 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !71, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!71 = metadata !{null, metadata !9, metadata !9, metadata !9, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10}
!72 = metadata !{metadata !73, metadata !74, metadata !75, metadata !76, metadata !77, metadata !78, metadata !79, metadata !80, metadata !81, metadata !82, metadata !83, metadata !84, metadata !85, metadata !86, metadata !87, metadata !88, metadata !89, metadata !90, metadata !91, metadata !93, metadata !94}
!73 = metadata !{i32 786689, metadata !69, metadata !"uiWA", metadata !5, i32 16777346, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWA] [line 130]
!74 = metadata !{i32 786689, metadata !69, metadata !"uiWB", metadata !5, i32 33554562, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWB] [line 130]
!75 = metadata !{i32 786689, metadata !69, metadata !"uiWC", metadata !5, i32 50331778, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWC] [line 130]
!76 = metadata !{i32 786689, metadata !69, metadata !"C", metadata !5, i32 67108995, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 131]
!77 = metadata !{i32 786689, metadata !69, metadata !"A", metadata !5, i32 83886211, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 131]
!78 = metadata !{i32 786689, metadata !69, metadata !"B", metadata !5, i32 100663427, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 131]
!79 = metadata !{i32 786689, metadata !69, metadata !"As", metadata !5, i32 117440644, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [As] [line 132]
!80 = metadata !{i32 786689, metadata !69, metadata !"Bs", metadata !5, i32 134217860, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bs] [line 132]
!81 = metadata !{i32 786688, metadata !69, metadata !"bx", metadata !5, i32 135, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bx] [line 135]
!82 = metadata !{i32 786688, metadata !69, metadata !"by", metadata !5, i32 136, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [by] [line 136]
!83 = metadata !{i32 786688, metadata !69, metadata !"tx", metadata !5, i32 139, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tx] [line 139]
!84 = metadata !{i32 786688, metadata !69, metadata !"ty", metadata !5, i32 140, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [ty] [line 140]
!85 = metadata !{i32 786688, metadata !69, metadata !"aBegin", metadata !5, i32 143, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aBegin] [line 143]
!86 = metadata !{i32 786688, metadata !69, metadata !"aEnd", metadata !5, i32 146, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aEnd] [line 146]
!87 = metadata !{i32 786688, metadata !69, metadata !"aStep", metadata !5, i32 149, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aStep] [line 149]
!88 = metadata !{i32 786688, metadata !69, metadata !"bBegin", metadata !5, i32 152, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bBegin] [line 152]
!89 = metadata !{i32 786688, metadata !69, metadata !"bStep", metadata !5, i32 155, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bStep] [line 155]
!90 = metadata !{i32 786688, metadata !69, metadata !"Csub", metadata !5, i32 159, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Csub] [line 159]
!91 = metadata !{i32 786688, metadata !92, metadata !"a", metadata !5, i32 163, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [a] [line 163]
!92 = metadata !{i32 786443, metadata !1, metadata !69, i32 163, i32 0, i32 18} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!93 = metadata !{i32 786688, metadata !92, metadata !"b", metadata !5, i32 163, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [b] [line 163]
!94 = metadata !{i32 786688, metadata !95, metadata !"k", metadata !5, i32 180, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 180]
!95 = metadata !{i32 786443, metadata !1, metadata !96, i32 180, i32 0, i32 20} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!96 = metadata !{i32 786443, metadata !1, metadata !92, i32 165, i32 0, i32 19} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!97 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_elem}
!98 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row}
!99 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row_priv}
!100 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local}
!101 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block}
!102 = metadata !{i32 4, i32 0, metadata !4, null}
!103 = metadata !{i32 5, i32 0, metadata !4, null}
!104 = metadata !{i32 6, i32 0, metadata !4, null}
!105 = metadata !{i32 7, i32 0, metadata !4, null}
!106 = metadata !{i32 8, i32 0, metadata !4, null} ; [ DW_TAG_imported_declaration ]
!107 = metadata !{i32 9, i32 0, metadata !4, null}
!108 = metadata !{i32 12, i32 0, metadata !4, null}
!109 = metadata !{i32 13, i32 0, metadata !4, null}
!110 = metadata !{i32 15, i32 0, metadata !4, null}
!111 = metadata !{i32 18, i32 0, metadata !112, null}
!112 = metadata !{i32 786443, metadata !1, metadata !113, i32 18, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!113 = metadata !{i32 786443, metadata !1, metadata !4, i32 16, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!114 = metadata !{i32 20, i32 0, metadata !115, null}
!115 = metadata !{i32 786443, metadata !1, metadata !112, i32 19, i32 0, i32 2} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!116 = metadata !{metadata !"float", metadata !117}
!117 = metadata !{metadata !"omnipotent char", metadata !118}
!118 = metadata !{metadata !"Simple C/C++ TBAA"}
!119 = metadata !{i32 22, i32 0, metadata !113, null}
!120 = metadata !{i32 23, i32 0, metadata !113, null}
!121 = metadata !{i32 24, i32 0, metadata !4, null}
!122 = metadata !{i32 27, i32 0, metadata !23, null}
!123 = metadata !{i32 28, i32 0, metadata !23, null}
!124 = metadata !{i32 29, i32 0, metadata !23, null}
!125 = metadata !{i32 30, i32 0, metadata !23, null}
!126 = metadata !{i32 31, i32 0, metadata !23, null}
!127 = metadata !{i32 32, i32 0, metadata !23, null}
!128 = metadata !{i32 35, i32 0, metadata !23, null}
!129 = metadata !{i32 37, i32 0, metadata !23, null}
!130 = metadata !{i32 39, i32 0, metadata !131, null}
!131 = metadata !{i32 786443, metadata !1, metadata !132, i32 39, i32 0, i32 4} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!132 = metadata !{i32 786443, metadata !1, metadata !23, i32 38, i32 0, i32 3} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!133 = metadata !{i32 41, i32 0, metadata !134, null}
!134 = metadata !{i32 786443, metadata !1, metadata !135, i32 41, i32 0, i32 6} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!135 = metadata !{i32 786443, metadata !1, metadata !131, i32 39, i32 0, i32 5} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!136 = metadata !{i32 42, i32 0, metadata !134, null}
!137 = metadata !{i32 43, i32 0, metadata !135, null}
!138 = metadata !{i32 46, i32 0, metadata !23, null}
!139 = metadata !{i32 49, i32 0, metadata !35, null}
!140 = metadata !{i32 50, i32 0, metadata !35, null}
!141 = metadata !{i32 51, i32 0, metadata !35, null}
!142 = metadata !{i32 52, i32 0, metadata !35, null}
!143 = metadata !{i32 53, i32 0, metadata !35, null}
!144 = metadata !{i32 54, i32 0, metadata !35, null}
!145 = metadata !{i32 57, i32 0, metadata !35, null}
!146 = metadata !{i32 58, i32 0, metadata !35, null} ; [ DW_TAG_imported_module ]
!147 = metadata !{i32 60, i32 0, metadata !35, null}
!148 = metadata !{i32 62, i32 0, metadata !149, null}
!149 = metadata !{i32 786443, metadata !1, metadata !150, i32 62, i32 0, i32 8} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!150 = metadata !{i32 786443, metadata !1, metadata !35, i32 61, i32 0, i32 7} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!151 = metadata !{i32 63, i32 0, metadata !149, null}
!152 = metadata !{i32 65, i32 0, metadata !153, null}
!153 = metadata !{i32 786443, metadata !1, metadata !150, i32 65, i32 0, i32 9} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!154 = metadata !{i32 69, i32 0, metadata !155, null}
!155 = metadata !{i32 786443, metadata !1, metadata !153, i32 65, i32 0, i32 10} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!156 = metadata !{i32 67, i32 0, metadata !157, null}
!157 = metadata !{i32 786443, metadata !1, metadata !155, i32 67, i32 0, i32 11} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!158 = metadata !{i32 68, i32 0, metadata !157, null}
!159 = metadata !{i32 72, i32 0, metadata !35, null}
!160 = metadata !{i32 75, i32 0, metadata !51, null}
!161 = metadata !{i32 76, i32 0, metadata !51, null}
!162 = metadata !{i32 77, i32 0, metadata !51, null}
!163 = metadata !{i32 78, i32 0, metadata !51, null}
!164 = metadata !{i32 79, i32 0, metadata !51, null}
!165 = metadata !{i32 80, i32 0, metadata !51, null}
!166 = metadata !{i32 81, i32 0, metadata !51, null}
!167 = metadata !{i32 84, i32 0, metadata !51, null}
!168 = metadata !{i32 85, i32 0, metadata !51, null}
!169 = metadata !{i32 86, i32 0, metadata !51, null}
!170 = metadata !{i32 87, i32 0, metadata !51, null}
!171 = metadata !{i32 89, i32 0, metadata !51, null}
!172 = metadata !{i32 91, i32 0, metadata !173, null}
!173 = metadata !{i32 786443, metadata !1, metadata !174, i32 91, i32 0, i32 13} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!174 = metadata !{i32 786443, metadata !1, metadata !51, i32 90, i32 0, i32 12} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!175 = metadata !{i32 92, i32 0, metadata !173, null}
!176 = metadata !{i32 94, i32 0, metadata !177, null}
!177 = metadata !{i32 786443, metadata !1, metadata !174, i32 94, i32 0, i32 14} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!178 = metadata !{i32 95, i32 0, metadata !179, null}
!179 = metadata !{i32 786443, metadata !1, metadata !180, i32 95, i32 0, i32 16} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!180 = metadata !{i32 786443, metadata !1, metadata !177, i32 94, i32 0, i32 15} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!181 = metadata !{i32 101, i32 0, metadata !180, null}
!182 = metadata !{i32 96, i32 0, metadata !179, null}
!183 = metadata !{i32 97, i32 0, metadata !180, null}
!184 = metadata !{float 0.000000e+00}
!185 = metadata !{i32 98, i32 0, metadata !180, null}
!186 = metadata !{i32 99, i32 0, metadata !187, null}
!187 = metadata !{i32 786443, metadata !1, metadata !180, i32 99, i32 0, i32 17} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!188 = metadata !{i32 100, i32 0, metadata !187, null}
!189 = metadata !{i32 102, i32 0, metadata !180, null}
!190 = metadata !{i32 105, i32 0, metadata !51, null}
!191 = metadata !{i32 130, i32 0, metadata !69, null}
!192 = metadata !{i32 131, i32 0, metadata !69, null}
!193 = metadata !{i32 132, i32 0, metadata !69, null}
!194 = metadata !{i32 135, i32 0, metadata !69, null}
!195 = metadata !{i32 136, i32 0, metadata !69, null}
!196 = metadata !{i32 139, i32 0, metadata !69, null}
!197 = metadata !{i32 140, i32 0, metadata !69, null}
!198 = metadata !{i32 143, i32 0, metadata !69, null}
!199 = metadata !{i32 146, i32 0, metadata !69, null}
!200 = metadata !{i32 16}
!201 = metadata !{i32 149, i32 0, metadata !69, null}
!202 = metadata !{i32 152, i32 0, metadata !69, null}
!203 = metadata !{i32 155, i32 0, metadata !69, null}
!204 = metadata !{i32 159, i32 0, metadata !69, null}
!205 = metadata !{i32 163, i32 0, metadata !92, null}
!206 = metadata !{i32 170, i32 0, metadata !96, null}
!207 = metadata !{i32 171, i32 0, metadata !96, null}
!208 = metadata !{i32 174, i32 0, metadata !96, null}
!209 = metadata !{i32 180, i32 0, metadata !95, null}
!210 = metadata !{i32 181, i32 0, metadata !95, null}
!211 = metadata !{i32 186, i32 0, metadata !96, null}
!212 = metadata !{i32 165, i32 0, metadata !92, null}
!213 = metadata !{i32 191, i32 0, metadata !69, null}
!214 = metadata !{i32 193, i32 0, metadata !69, null}

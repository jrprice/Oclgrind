; ModuleID = 'matmul.cl'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir"

; Function Attrs: nounwind
define void @matmul_elem(i32 %dim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %dim}, i64 0, metadata !13), !dbg !97
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !14), !dbg !98
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !15), !dbg !99
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !16), !dbg !100
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !101
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !18), !dbg !101
  %call1 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 1) #4, !dbg !102
  tail call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !19), !dbg !102
  %cmp = icmp slt i32 %call, %dim, !dbg !103
  %cmp2 = icmp slt i32 %call1, %dim, !dbg !103
  %or.cond = and i1 %cmp, %cmp2, !dbg !103
  br i1 %or.cond, label %for.cond.preheader, label %if.end, !dbg !103

for.cond.preheader:                               ; preds = %entry
  %cmp325 = icmp sgt i32 %dim, 0, !dbg !104
  %mul = mul nsw i32 %call, %dim, !dbg !107
  br i1 %cmp325, label %for.body, label %for.end, !dbg !104

for.body:                                         ; preds = %for.cond.preheader, %for.body
  %tmp.027 = phi float [ %add8, %for.body ], [ 0.000000e+00, %for.cond.preheader ]
  %k.026 = phi i32 [ %inc, %for.body ], [ 0, %for.cond.preheader ]
  %add = add nsw i32 %k.026, %mul, !dbg !107
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !107
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !107, !tbaa !109
  %mul4 = mul nsw i32 %k.026, %dim, !dbg !107
  %add5 = add nsw i32 %mul4, %call1, !dbg !107
  %arrayidx6 = getelementptr inbounds float addrspace(1)* %B, i32 %add5, !dbg !107
  %1 = load float addrspace(1)* %arrayidx6, align 4, !dbg !107, !tbaa !109
  %mul7 = fmul float %0, %1, !dbg !107
  %add8 = fadd float %tmp.027, %mul7, !dbg !107
  tail call void @llvm.dbg.value(metadata !{float %add8}, i64 0, metadata !20), !dbg !107
  %inc = add nsw i32 %k.026, 1, !dbg !104
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !17), !dbg !104
  %cmp3 = icmp slt i32 %inc, %dim, !dbg !104
  br i1 %cmp3, label %for.body, label %for.end, !dbg !104

for.end:                                          ; preds = %for.body, %for.cond.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond.preheader ], [ %add8, %for.body ]
  %add10 = add nsw i32 %mul, %call1, !dbg !112
  %arrayidx11 = getelementptr inbounds float addrspace(1)* %C, i32 %add10, !dbg !112
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx11, align 4, !dbg !112, !tbaa !109
  br label %if.end, !dbg !113

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !114
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

declare i32 @get_global_id(...) #2

; Function Attrs: nounwind
define void @matmul_row(i32 %dim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %dim}, i64 0, metadata !23), !dbg !115
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !24), !dbg !116
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !25), !dbg !117
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !26), !dbg !118
  %call = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !119
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !29), !dbg !119
  %cmp = icmp slt i32 %call, %dim, !dbg !120
  %cmp134 = icmp sgt i32 %dim, 0, !dbg !121
  %or.cond = and i1 %cmp, %cmp134, !dbg !120
  br i1 %or.cond, label %for.cond2.preheader.lr.ph, label %if.end, !dbg !120

for.cond2.preheader.lr.ph:                        ; preds = %entry
  %mul = mul nsw i32 %call, %dim, !dbg !124
  br label %for.body4.lr.ph, !dbg !121

for.body4.lr.ph:                                  ; preds = %for.end, %for.cond2.preheader.lr.ph
  %j.035 = phi i32 [ 0, %for.cond2.preheader.lr.ph ], [ %inc14, %for.end ]
  br label %for.body4, !dbg !127

for.body4:                                        ; preds = %for.body4.lr.ph, %for.body4
  %tmp.032 = phi float [ 0.000000e+00, %for.body4.lr.ph ], [ %add9, %for.body4 ]
  %k.031 = phi i32 [ 0, %for.body4.lr.ph ], [ %inc, %for.body4 ]
  %add = add nsw i32 %k.031, %mul, !dbg !124
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !124
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !124, !tbaa !109
  %mul5 = mul nsw i32 %k.031, %dim, !dbg !124
  %add6 = add nsw i32 %mul5, %j.035, !dbg !124
  %arrayidx7 = getelementptr inbounds float addrspace(1)* %B, i32 %add6, !dbg !124
  %1 = load float addrspace(1)* %arrayidx7, align 4, !dbg !124, !tbaa !109
  %mul8 = fmul float %0, %1, !dbg !124
  %add9 = fadd float %tmp.032, %mul8, !dbg !124
  tail call void @llvm.dbg.value(metadata !{float %add9}, i64 0, metadata !30), !dbg !124
  %inc = add nsw i32 %k.031, 1, !dbg !127
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !27), !dbg !127
  %cmp3 = icmp slt i32 %inc, %dim, !dbg !127
  br i1 %cmp3, label %for.body4, label %for.end, !dbg !127

for.end:                                          ; preds = %for.body4
  %add11 = add nsw i32 %j.035, %mul, !dbg !128
  %arrayidx12 = getelementptr inbounds float addrspace(1)* %C, i32 %add11, !dbg !128
  store float %add9, float addrspace(1)* %arrayidx12, align 4, !dbg !128, !tbaa !109
  %inc14 = add nsw i32 %j.035, 1, !dbg !121
  tail call void @llvm.dbg.value(metadata !{i32 %inc14}, i64 0, metadata !28), !dbg !121
  %cmp1 = icmp slt i32 %inc14, %dim, !dbg !121
  br i1 %cmp1, label %for.body4.lr.ph, label %if.end, !dbg !121

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !129
}

; Function Attrs: nounwind
define void @matmul_row_priv(i32 %dim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) #0 {
entry:
  %Awrk = alloca [16 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %dim}, i64 0, metadata !33), !dbg !130
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !34), !dbg !131
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !35), !dbg !132
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !36), !dbg !133
  %call = call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !134
  call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !39), !dbg !134
  %0 = bitcast [16 x float]* %Awrk to i8*, !dbg !135
  call void @llvm.lifetime.start(i64 64, i8* %0) #3, !dbg !135
  call void @llvm.dbg.declare(metadata !{[16 x float]* %Awrk}, metadata !40), !dbg !135
  %cmp = icmp slt i32 %call, %dim, !dbg !136
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !136

for.cond.preheader:                               ; preds = %entry
  %cmp148 = icmp sgt i32 %dim, 0, !dbg !137
  br i1 %cmp148, label %for.body.lr.ph, label %if.end, !dbg !137

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %call, %dim, !dbg !140
  br label %for.body, !dbg !137

for.cond3.preheader:                              ; preds = %for.body
  br i1 %cmp148, label %for.cond6.preheader.lr.ph, label %if.end, !dbg !141

for.cond6.preheader.lr.ph:                        ; preds = %for.cond3.preheader
  %mul18 = mul nsw i32 %call, %dim, !dbg !143
  br label %for.body8.lr.ph, !dbg !141

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.049 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.049, %mul, !dbg !140
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !140
  %1 = load float addrspace(1)* %arrayidx, align 4, !dbg !140, !tbaa !109
  %arrayidx2 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.049, !dbg !140
  store float %1, float* %arrayidx2, align 4, !dbg !140, !tbaa !109
  %inc = add nsw i32 %k.049, 1, !dbg !137
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !37), !dbg !137
  %cmp1 = icmp slt i32 %inc, %dim, !dbg !137
  br i1 %cmp1, label %for.body, label %for.cond3.preheader, !dbg !137

for.body8.lr.ph:                                  ; preds = %for.end17, %for.cond6.preheader.lr.ph
  %j.047 = phi i32 [ 0, %for.cond6.preheader.lr.ph ], [ %inc22, %for.end17 ]
  br label %for.body8, !dbg !145

for.body8:                                        ; preds = %for.body8.lr.ph, %for.body8
  %tmp.045 = phi float [ 0.000000e+00, %for.body8.lr.ph ], [ %add14, %for.body8 ]
  %k.144 = phi i32 [ 0, %for.body8.lr.ph ], [ %inc16, %for.body8 ]
  %arrayidx9 = getelementptr inbounds [16 x float]* %Awrk, i32 0, i32 %k.144, !dbg !147
  %2 = load float* %arrayidx9, align 4, !dbg !147, !tbaa !109
  %mul10 = mul nsw i32 %k.144, %dim, !dbg !147
  %add11 = add nsw i32 %mul10, %j.047, !dbg !147
  %arrayidx12 = getelementptr inbounds float addrspace(1)* %B, i32 %add11, !dbg !147
  %3 = load float addrspace(1)* %arrayidx12, align 4, !dbg !147, !tbaa !109
  %mul13 = fmul float %2, %3, !dbg !147
  %add14 = fadd float %tmp.045, %mul13, !dbg !147
  call void @llvm.dbg.value(metadata !{float %add14}, i64 0, metadata !44), !dbg !147
  %inc16 = add nsw i32 %k.144, 1, !dbg !145
  call void @llvm.dbg.value(metadata !{i32 %inc16}, i64 0, metadata !37), !dbg !145
  %cmp7 = icmp slt i32 %inc16, %dim, !dbg !145
  br i1 %cmp7, label %for.body8, label %for.end17, !dbg !145

for.end17:                                        ; preds = %for.body8
  %add19 = add nsw i32 %j.047, %mul18, !dbg !143
  %arrayidx20 = getelementptr inbounds float addrspace(1)* %C, i32 %add19, !dbg !143
  store float %add14, float addrspace(1)* %arrayidx20, align 4, !dbg !143, !tbaa !109
  %inc22 = add nsw i32 %j.047, 1, !dbg !141
  call void @llvm.dbg.value(metadata !{i32 %inc22}, i64 0, metadata !38), !dbg !141
  %cmp4 = icmp slt i32 %inc22, %dim, !dbg !141
  br i1 %cmp4, label %for.body8.lr.ph, label %if.end, !dbg !141

if.end:                                           ; preds = %for.cond.preheader, %for.cond3.preheader, %for.end17, %entry
  call void @llvm.lifetime.end(i64 64, i8* %0) #3, !dbg !148
  ret void, !dbg !148
}

; Function Attrs: nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #3

; Function Attrs: nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #3

; Function Attrs: nounwind
define void @matmul_row_local(i32 %dim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C, float addrspace(3)* nocapture %Bwrk) #0 {
entry:
  %Awrk = alloca [1024 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %dim}, i64 0, metadata !49), !dbg !149
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !50), !dbg !150
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !51), !dbg !151
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !52), !dbg !152
  call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bwrk}, i64 0, metadata !53), !dbg !153
  %call = call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !154
  call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !56), !dbg !154
  %call1 = call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 0) #4, !dbg !155
  call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !57), !dbg !155
  call void @llvm.dbg.value(metadata !156, i64 0, metadata !58), !dbg !157
  %0 = bitcast [1024 x float]* %Awrk to i8*, !dbg !158
  call void @llvm.lifetime.start(i64 4096, i8* %0) #3, !dbg !158
  call void @llvm.dbg.declare(metadata !{[1024 x float]* %Awrk}, metadata !59), !dbg !158
  %cmp = icmp slt i32 %call, %dim, !dbg !159
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !159

for.cond.preheader:                               ; preds = %entry
  %cmp266 = icmp sgt i32 %dim, 0, !dbg !160
  br i1 %cmp266, label %for.body.lr.ph, label %if.end, !dbg !160

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %call, %dim, !dbg !163
  br label %for.body, !dbg !160

for.cond4.preheader:                              ; preds = %for.body
  br i1 %cmp266, label %for.cond7.preheader.lr.ph, label %if.end, !dbg !164

for.cond7.preheader.lr.ph:                        ; preds = %for.cond4.preheader
  %cmp859 = icmp slt i32 %call1, %dim, !dbg !166
  %mul28 = mul nsw i32 %call, %dim, !dbg !169
  br label %for.cond7.preheader, !dbg !164

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.067 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.067, %mul, !dbg !163
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add, !dbg !163
  %1 = load float addrspace(1)* %arrayidx, align 4, !dbg !163, !tbaa !109
  %arrayidx3 = getelementptr inbounds [1024 x float]* %Awrk, i32 0, i32 %k.067, !dbg !163
  store float %1, float* %arrayidx3, align 4, !dbg !163, !tbaa !109
  %inc = add nsw i32 %k.067, 1, !dbg !160
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !54), !dbg !160
  %cmp2 = icmp slt i32 %inc, %dim, !dbg !160
  br i1 %cmp2, label %for.body, label %for.cond4.preheader, !dbg !160

for.cond7.preheader:                              ; preds = %for.cond7.preheader.lr.ph, %for.end27
  %j.065 = phi i32 [ 0, %for.cond7.preheader.lr.ph ], [ %inc32, %for.end27 ]
  br i1 %cmp859, label %for.body9, label %for.body20.lr.ph, !dbg !166

for.body9:                                        ; preds = %for.cond7.preheader, %for.body9
  %k.160 = phi i32 [ %add15, %for.body9 ], [ %call1, %for.cond7.preheader ]
  %mul10 = mul nsw i32 %k.160, %dim, !dbg !170
  %add11 = add nsw i32 %mul10, %j.065, !dbg !170
  %arrayidx12 = getelementptr inbounds float addrspace(1)* %B, i32 %add11, !dbg !170
  %2 = load float addrspace(1)* %arrayidx12, align 4, !dbg !170, !tbaa !109
  %arrayidx13 = getelementptr inbounds float addrspace(3)* %Bwrk, i32 %k.160, !dbg !170
  store float %2, float addrspace(3)* %arrayidx13, align 4, !dbg !170, !tbaa !109
  %add15 = add nsw i32 %k.160, 2, !dbg !166
  call void @llvm.dbg.value(metadata !{i32 %add15}, i64 0, metadata !54), !dbg !166
  %cmp8 = icmp slt i32 %add15, %dim, !dbg !166
  br i1 %cmp8, label %for.body9, label %for.body20.lr.ph, !dbg !166

for.body20.lr.ph:                                 ; preds = %for.cond7.preheader, %for.body9
  %call17 = call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !171
  call void @llvm.dbg.value(metadata !172, i64 0, metadata !63), !dbg !173
  call void @llvm.dbg.value(metadata !2, i64 0, metadata !54), !dbg !174
  br label %for.body20, !dbg !174

for.body20:                                       ; preds = %for.body20.lr.ph, %for.body20
  %tmp.063 = phi float [ 0.000000e+00, %for.body20.lr.ph ], [ %add24, %for.body20 ]
  %k.262 = phi i32 [ 0, %for.body20.lr.ph ], [ %inc26, %for.body20 ]
  %arrayidx21 = getelementptr inbounds [1024 x float]* %Awrk, i32 0, i32 %k.262, !dbg !176
  %3 = load float* %arrayidx21, align 4, !dbg !176, !tbaa !109
  %arrayidx22 = getelementptr inbounds float addrspace(3)* %Bwrk, i32 %k.262, !dbg !176
  %4 = load float addrspace(3)* %arrayidx22, align 4, !dbg !176, !tbaa !109
  %mul23 = fmul float %3, %4, !dbg !176
  %add24 = fadd float %tmp.063, %mul23, !dbg !176
  call void @llvm.dbg.value(metadata !{float %add24}, i64 0, metadata !63), !dbg !176
  %inc26 = add nsw i32 %k.262, 1, !dbg !174
  call void @llvm.dbg.value(metadata !{i32 %inc26}, i64 0, metadata !54), !dbg !174
  %cmp19 = icmp slt i32 %inc26, %dim, !dbg !174
  br i1 %cmp19, label %for.body20, label %for.end27, !dbg !174

for.end27:                                        ; preds = %for.body20
  %add29 = add nsw i32 %j.065, %mul28, !dbg !169
  %arrayidx30 = getelementptr inbounds float addrspace(1)* %C, i32 %add29, !dbg !169
  store float %add24, float addrspace(1)* %arrayidx30, align 4, !dbg !169, !tbaa !109
  %inc32 = add nsw i32 %j.065, 1, !dbg !164
  call void @llvm.dbg.value(metadata !{i32 %inc32}, i64 0, metadata !55), !dbg !164
  %cmp5 = icmp slt i32 %inc32, %dim, !dbg !164
  br i1 %cmp5, label %for.cond7.preheader, label %if.end, !dbg !164

if.end:                                           ; preds = %for.cond.preheader, %for.cond4.preheader, %for.end27, %entry
  call void @llvm.lifetime.end(i64 4096, i8* %0) #3, !dbg !177
  ret void, !dbg !177
}

declare i32 @get_local_id(...) #2

declare i32 @barrier(...) #2

; Function Attrs: nounwind
define void @matmul_block(i32 %uiWA, i32 %uiWB, i32 %uiWC, float addrspace(1)* nocapture %C, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(3)* nocapture %As, float addrspace(3)* nocapture %Bs) #0 {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %uiWA}, i64 0, metadata !68), !dbg !178
  tail call void @llvm.dbg.value(metadata !{i32 %uiWB}, i64 0, metadata !69), !dbg !178
  tail call void @llvm.dbg.value(metadata !{i32 %uiWC}, i64 0, metadata !70), !dbg !178
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !71), !dbg !179
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !72), !dbg !179
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !73), !dbg !179
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %As}, i64 0, metadata !74), !dbg !180
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bs}, i64 0, metadata !75), !dbg !180
  %call = tail call i32 bitcast (i32 (...)* @get_group_id to i32 (i32)*)(i32 0) #4, !dbg !181
  tail call void @llvm.dbg.value(metadata !{i32 %call}, i64 0, metadata !76), !dbg !181
  %call1 = tail call i32 bitcast (i32 (...)* @get_group_id to i32 (i32)*)(i32 1) #4, !dbg !182
  tail call void @llvm.dbg.value(metadata !{i32 %call1}, i64 0, metadata !77), !dbg !182
  %call2 = tail call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 0) #4, !dbg !183
  tail call void @llvm.dbg.value(metadata !{i32 %call2}, i64 0, metadata !78), !dbg !183
  %call3 = tail call i32 bitcast (i32 (...)* @get_local_id to i32 (i32)*)(i32 1) #4, !dbg !184
  tail call void @llvm.dbg.value(metadata !{i32 %call3}, i64 0, metadata !79), !dbg !184
  %mul = shl nsw i32 %uiWA, 4, !dbg !185
  %mul4 = mul nsw i32 %call1, %mul, !dbg !185
  tail call void @llvm.dbg.value(metadata !{i32 %mul4}, i64 0, metadata !80), !dbg !185
  %add = add i32 %uiWA, -1, !dbg !186
  %sub = add i32 %add, %mul4, !dbg !186
  tail call void @llvm.dbg.value(metadata !{i32 %sub}, i64 0, metadata !81), !dbg !186
  tail call void @llvm.dbg.value(metadata !187, i64 0, metadata !82), !dbg !188
  tail call void @llvm.dbg.value(metadata !{i32 %mul5}, i64 0, metadata !83), !dbg !189
  %mul6 = shl i32 %uiWB, 4, !dbg !190
  tail call void @llvm.dbg.value(metadata !{i32 %mul6}, i64 0, metadata !84), !dbg !190
  tail call void @llvm.dbg.value(metadata !172, i64 0, metadata !85), !dbg !191
  tail call void @llvm.dbg.value(metadata !{i32 %mul4}, i64 0, metadata !86), !dbg !192
  tail call void @llvm.dbg.value(metadata !{i32 %mul5}, i64 0, metadata !88), !dbg !192
  %cmp66 = icmp sgt i32 %mul4, %sub, !dbg !192
  br i1 %cmp66, label %for.end36, label %for.body.lr.ph, !dbg !192

for.body.lr.ph:                                   ; preds = %entry
  %mul5 = shl nsw i32 %call, 4, !dbg !189
  %mul7 = mul nsw i32 %call3, %uiWA, !dbg !193
  %add8 = add i32 %mul7, %call2, !dbg !193
  %mul10 = shl nsw i32 %call3, 4, !dbg !193
  %add11 = add nsw i32 %mul10, %call2, !dbg !193
  %arrayidx12 = getelementptr inbounds float addrspace(3)* %As, i32 %add11, !dbg !193
  %mul13 = mul nsw i32 %call3, %uiWB, !dbg !194
  %add14 = add i32 %mul13, %call2, !dbg !194
  %arrayidx19 = getelementptr inbounds float addrspace(3)* %Bs, i32 %add11, !dbg !194
  br label %for.body, !dbg !192

for.body:                                         ; preds = %for.body.lr.ph, %for.end
  %b.069 = phi i32 [ %mul5, %for.body.lr.ph ], [ %add35, %for.end ]
  %a.068 = phi i32 [ %mul4, %for.body.lr.ph ], [ %add34, %for.end ]
  %Csub.067 = phi float [ 0.000000e+00, %for.body.lr.ph ], [ %add31, %for.end ]
  %add9 = add i32 %add8, %a.068, !dbg !193
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i32 %add9, !dbg !193
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !193, !tbaa !109
  store float %0, float addrspace(3)* %arrayidx12, align 4, !dbg !193, !tbaa !109
  %add15 = add i32 %add14, %b.069, !dbg !194
  %arrayidx16 = getelementptr inbounds float addrspace(1)* %B, i32 %add15, !dbg !194
  %1 = load float addrspace(1)* %arrayidx16, align 4, !dbg !194, !tbaa !109
  store float %1, float addrspace(3)* %arrayidx19, align 4, !dbg !194, !tbaa !109
  %call20 = tail call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !195
  tail call void @llvm.dbg.value(metadata !2, i64 0, metadata !89), !dbg !196
  br label %for.body23, !dbg !196

for.body23:                                       ; preds = %for.body, %for.body23
  %k.065 = phi i32 [ 0, %for.body ], [ %inc, %for.body23 ]
  %Csub.164 = phi float [ %Csub.067, %for.body ], [ %add31, %for.body23 ]
  %add25 = add nsw i32 %k.065, %mul10, !dbg !197
  %arrayidx26 = getelementptr inbounds float addrspace(3)* %As, i32 %add25, !dbg !197
  %2 = load float addrspace(3)* %arrayidx26, align 4, !dbg !197, !tbaa !109
  %mul27 = shl i32 %k.065, 4, !dbg !197
  %add28 = add nsw i32 %mul27, %call2, !dbg !197
  %arrayidx29 = getelementptr inbounds float addrspace(3)* %Bs, i32 %add28, !dbg !197
  %3 = load float addrspace(3)* %arrayidx29, align 4, !dbg !197, !tbaa !109
  %mul30 = fmul float %2, %3, !dbg !197
  %add31 = fadd float %Csub.164, %mul30, !dbg !197
  tail call void @llvm.dbg.value(metadata !{float %add31}, i64 0, metadata !85), !dbg !197
  %inc = add nsw i32 %k.065, 1, !dbg !196
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !89), !dbg !196
  %cmp22 = icmp slt i32 %inc, 16, !dbg !196
  br i1 %cmp22, label %for.body23, label %for.end, !dbg !196

for.end:                                          ; preds = %for.body23
  %call32 = tail call i32 bitcast (i32 (...)* @barrier to i32 (i32)*)(i32 0) #4, !dbg !198
  %add34 = add nsw i32 %a.068, 16, !dbg !199
  tail call void @llvm.dbg.value(metadata !{i32 %add34}, i64 0, metadata !86), !dbg !199
  %add35 = add nsw i32 %b.069, %mul6, !dbg !199
  tail call void @llvm.dbg.value(metadata !{i32 %add35}, i64 0, metadata !88), !dbg !199
  %cmp = icmp sgt i32 %add34, %sub, !dbg !192
  br i1 %cmp, label %for.end36, label %for.body, !dbg !192

for.end36:                                        ; preds = %for.end, %entry
  %Csub.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %add31, %for.end ]
  %call37 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 1) #4, !dbg !200
  %call38 = tail call i32 bitcast (i32 (...)* @get_global_size to i32 (i32)*)(i32 0) #4, !dbg !200
  %mul39 = mul nsw i32 %call38, %call37, !dbg !200
  %call40 = tail call i32 bitcast (i32 (...)* @get_global_id to i32 (i32)*)(i32 0) #4, !dbg !200
  %add41 = add nsw i32 %mul39, %call40, !dbg !200
  %arrayidx42 = getelementptr inbounds float addrspace(1)* %C, i32 %add41, !dbg !200
  store float %Csub.0.lcssa, float addrspace(1)* %arrayidx42, align 4, !dbg !200, !tbaa !109
  ret void, !dbg !201
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
!opencl.kernels = !{!92, !93, !94, !95, !96}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.4 (trunk 182581)", i1 true, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !""} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/matmul.cl] [DW_LANG_C99]
!1 = metadata !{metadata !"matmul.cl", metadata !"/Users/james/projects/oclgrind/tests"}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4, metadata !21, metadata !31, metadata !45, metadata !64}
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
!45 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_row_local", metadata !"matmul_row_local", metadata !"", i32 93, metadata !46, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local, null, null, metadata !48, i32 102} ; [ DW_TAG_subprogram ] [line 93] [def] [scope 102] [matmul_row_local]
!46 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !47, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!47 = metadata !{null, metadata !8, metadata !10, metadata !10, metadata !10, metadata !10}
!48 = metadata !{metadata !49, metadata !50, metadata !51, metadata !52, metadata !53, metadata !54, metadata !55, metadata !56, metadata !57, metadata !58, metadata !59, metadata !63}
!49 = metadata !{i32 786689, metadata !45, metadata !"dim", metadata !5, i32 16777310, metadata !8, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [dim] [line 94]
!50 = metadata !{i32 786689, metadata !45, metadata !"A", metadata !5, i32 33554530, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 98]
!51 = metadata !{i32 786689, metadata !45, metadata !"B", metadata !5, i32 50331747, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 99]
!52 = metadata !{i32 786689, metadata !45, metadata !"C", metadata !5, i32 67108964, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 100]
!53 = metadata !{i32 786689, metadata !45, metadata !"Bwrk", metadata !5, i32 83886181, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bwrk] [line 101]
!54 = metadata !{i32 786688, metadata !45, metadata !"k", metadata !5, i32 103, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 103]
!55 = metadata !{i32 786688, metadata !45, metadata !"j", metadata !5, i32 103, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 103]
!56 = metadata !{i32 786688, metadata !45, metadata !"i", metadata !5, i32 104, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 104]
!57 = metadata !{i32 786688, metadata !45, metadata !"iloc", metadata !5, i32 105, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [iloc] [line 105]
!58 = metadata !{i32 786688, metadata !45, metadata !"nloc", metadata !5, i32 106, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [nloc] [line 106]
!59 = metadata !{i32 786688, metadata !45, metadata !"Awrk", metadata !5, i32 107, metadata !60, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Awrk] [line 107]
!60 = metadata !{i32 786433, null, null, metadata !"", i32 0, i64 32768, i64 32, i32 0, i32 0, metadata !11, metadata !61, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 32768, align 32, offset 0] [from float]
!61 = metadata !{metadata !62}
!62 = metadata !{i32 786465, i64 0, i64 1024}     ; [ DW_TAG_subrange_type ] [0, 1023]
!63 = metadata !{i32 786688, metadata !45, metadata !"tmp", metadata !5, i32 108, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 108]
!64 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"matmul_block", metadata !"matmul_block", metadata !"", i32 157, metadata !65, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block, null, null, metadata !67, i32 160} ; [ DW_TAG_subprogram ] [line 157] [def] [scope 160] [matmul_block]
!65 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !66, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!66 = metadata !{null, metadata !9, metadata !9, metadata !9, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10}
!67 = metadata !{metadata !68, metadata !69, metadata !70, metadata !71, metadata !72, metadata !73, metadata !74, metadata !75, metadata !76, metadata !77, metadata !78, metadata !79, metadata !80, metadata !81, metadata !82, metadata !83, metadata !84, metadata !85, metadata !86, metadata !88, metadata !89}
!68 = metadata !{i32 786689, metadata !64, metadata !"uiWA", metadata !5, i32 16777373, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWA] [line 157]
!69 = metadata !{i32 786689, metadata !64, metadata !"uiWB", metadata !5, i32 33554589, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWB] [line 157]
!70 = metadata !{i32 786689, metadata !64, metadata !"uiWC", metadata !5, i32 50331805, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWC] [line 157]
!71 = metadata !{i32 786689, metadata !64, metadata !"C", metadata !5, i32 67109022, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 158]
!72 = metadata !{i32 786689, metadata !64, metadata !"A", metadata !5, i32 83886238, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 158]
!73 = metadata !{i32 786689, metadata !64, metadata !"B", metadata !5, i32 100663454, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 158]
!74 = metadata !{i32 786689, metadata !64, metadata !"As", metadata !5, i32 117440671, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [As] [line 159]
!75 = metadata !{i32 786689, metadata !64, metadata !"Bs", metadata !5, i32 134217887, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bs] [line 159]
!76 = metadata !{i32 786688, metadata !64, metadata !"bx", metadata !5, i32 162, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bx] [line 162]
!77 = metadata !{i32 786688, metadata !64, metadata !"by", metadata !5, i32 163, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [by] [line 163]
!78 = metadata !{i32 786688, metadata !64, metadata !"tx", metadata !5, i32 166, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tx] [line 166]
!79 = metadata !{i32 786688, metadata !64, metadata !"ty", metadata !5, i32 167, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [ty] [line 167]
!80 = metadata !{i32 786688, metadata !64, metadata !"aBegin", metadata !5, i32 170, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aBegin] [line 170]
!81 = metadata !{i32 786688, metadata !64, metadata !"aEnd", metadata !5, i32 173, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aEnd] [line 173]
!82 = metadata !{i32 786688, metadata !64, metadata !"aStep", metadata !5, i32 176, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aStep] [line 176]
!83 = metadata !{i32 786688, metadata !64, metadata !"bBegin", metadata !5, i32 179, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bBegin] [line 179]
!84 = metadata !{i32 786688, metadata !64, metadata !"bStep", metadata !5, i32 182, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bStep] [line 182]
!85 = metadata !{i32 786688, metadata !64, metadata !"Csub", metadata !5, i32 186, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Csub] [line 186]
!86 = metadata !{i32 786688, metadata !87, metadata !"a", metadata !5, i32 190, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [a] [line 190]
!87 = metadata !{i32 786443, metadata !1, metadata !64, i32 190, i32 0, i32 18} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!88 = metadata !{i32 786688, metadata !87, metadata !"b", metadata !5, i32 190, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [b] [line 190]
!89 = metadata !{i32 786688, metadata !90, metadata !"k", metadata !5, i32 207, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 207]
!90 = metadata !{i32 786443, metadata !1, metadata !91, i32 207, i32 0, i32 20} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!91 = metadata !{i32 786443, metadata !1, metadata !87, i32 192, i32 0, i32 19} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!92 = metadata !{void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_elem}
!93 = metadata !{void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row}
!94 = metadata !{void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row_priv}
!95 = metadata !{void (i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local}
!96 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block}
!97 = metadata !{i32 4, i32 0, metadata !4, null}
!98 = metadata !{i32 8, i32 0, metadata !4, null} ; [ DW_TAG_imported_declaration ]
!99 = metadata !{i32 9, i32 0, metadata !4, null}
!100 = metadata !{i32 10, i32 0, metadata !4, null}
!101 = metadata !{i32 13, i32 0, metadata !4, null}
!102 = metadata !{i32 14, i32 0, metadata !4, null}
!103 = metadata !{i32 17, i32 0, metadata !4, null}
!104 = metadata !{i32 21, i32 0, metadata !105, null}
!105 = metadata !{i32 786443, metadata !1, metadata !106, i32 21, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!106 = metadata !{i32 786443, metadata !1, metadata !4, i32 18, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!107 = metadata !{i32 24, i32 0, metadata !108, null}
!108 = metadata !{i32 786443, metadata !1, metadata !105, i32 22, i32 0, i32 2} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!109 = metadata !{metadata !"float", metadata !110}
!110 = metadata !{metadata !"omnipotent char", metadata !111}
!111 = metadata !{metadata !"Simple C/C++ TBAA"}
!112 = metadata !{i32 27, i32 0, metadata !106, null}
!113 = metadata !{i32 28, i32 0, metadata !106, null}
!114 = metadata !{i32 29, i32 0, metadata !4, null}
!115 = metadata !{i32 32, i32 0, metadata !21, null}
!116 = metadata !{i32 36, i32 0, metadata !21, null}
!117 = metadata !{i32 37, i32 0, metadata !21, null}
!118 = metadata !{i32 38, i32 0, metadata !21, null}
!119 = metadata !{i32 41, i32 0, metadata !21, null}
!120 = metadata !{i32 44, i32 0, metadata !21, null}
!121 = metadata !{i32 47, i32 0, metadata !122, null}
!122 = metadata !{i32 786443, metadata !1, metadata !123, i32 47, i32 0, i32 4} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!123 = metadata !{i32 786443, metadata !1, metadata !21, i32 45, i32 0, i32 3} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!124 = metadata !{i32 52, i32 0, metadata !125, null}
!125 = metadata !{i32 786443, metadata !1, metadata !126, i32 50, i32 0, i32 6} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!126 = metadata !{i32 786443, metadata !1, metadata !122, i32 47, i32 0, i32 5} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!127 = metadata !{i32 50, i32 0, metadata !125, null}
!128 = metadata !{i32 54, i32 0, metadata !126, null}
!129 = metadata !{i32 57, i32 0, metadata !21, null}
!130 = metadata !{i32 60, i32 0, metadata !31, null}
!131 = metadata !{i32 64, i32 0, metadata !31, null}
!132 = metadata !{i32 65, i32 0, metadata !31, null}
!133 = metadata !{i32 66, i32 0, metadata !31, null}
!134 = metadata !{i32 69, i32 0, metadata !31, null}
!135 = metadata !{i32 70, i32 0, metadata !31, null}
!136 = metadata !{i32 73, i32 0, metadata !31, null}
!137 = metadata !{i32 76, i32 0, metadata !138, null}
!138 = metadata !{i32 786443, metadata !1, metadata !139, i32 76, i32 0, i32 8} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!139 = metadata !{i32 786443, metadata !1, metadata !31, i32 74, i32 0, i32 7} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!140 = metadata !{i32 78, i32 0, metadata !138, null}
!141 = metadata !{i32 81, i32 0, metadata !142, null}
!142 = metadata !{i32 786443, metadata !1, metadata !139, i32 81, i32 0, i32 9} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!143 = metadata !{i32 88, i32 0, metadata !144, null}
!144 = metadata !{i32 786443, metadata !1, metadata !142, i32 81, i32 0, i32 10} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!145 = metadata !{i32 84, i32 0, metadata !146, null}
!146 = metadata !{i32 786443, metadata !1, metadata !144, i32 84, i32 0, i32 11} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!147 = metadata !{i32 86, i32 0, metadata !146, null}
!148 = metadata !{i32 91, i32 0, metadata !31, null}
!149 = metadata !{i32 94, i32 0, metadata !45, null}
!150 = metadata !{i32 98, i32 0, metadata !45, null}
!151 = metadata !{i32 99, i32 0, metadata !45, null}
!152 = metadata !{i32 100, i32 0, metadata !45, null}
!153 = metadata !{i32 101, i32 0, metadata !45, null}
!154 = metadata !{i32 104, i32 0, metadata !45, null}
!155 = metadata !{i32 105, i32 0, metadata !45, null}
!156 = metadata !{i32 2}
!157 = metadata !{i32 106, i32 0, metadata !45, null}
!158 = metadata !{i32 107, i32 0, metadata !45, null}
!159 = metadata !{i32 109, i32 0, metadata !45, null}
!160 = metadata !{i32 112, i32 0, metadata !161, null}
!161 = metadata !{i32 786443, metadata !1, metadata !162, i32 112, i32 0, i32 13} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!162 = metadata !{i32 786443, metadata !1, metadata !45, i32 111, i32 0, i32 12} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!163 = metadata !{i32 114, i32 0, metadata !161, null}
!164 = metadata !{i32 117, i32 0, metadata !165, null}
!165 = metadata !{i32 786443, metadata !1, metadata !162, i32 117, i32 0, i32 14} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!166 = metadata !{i32 119, i32 0, metadata !167, null}
!167 = metadata !{i32 786443, metadata !1, metadata !168, i32 119, i32 0, i32 16} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!168 = metadata !{i32 786443, metadata !1, metadata !165, i32 117, i32 0, i32 15} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!169 = metadata !{i32 128, i32 0, metadata !168, null}
!170 = metadata !{i32 121, i32 0, metadata !167, null}
!171 = metadata !{i32 123, i32 0, metadata !168, null}
!172 = metadata !{float 0.000000e+00}
!173 = metadata !{i32 124, i32 0, metadata !168, null}
!174 = metadata !{i32 125, i32 0, metadata !175, null}
!175 = metadata !{i32 786443, metadata !1, metadata !168, i32 125, i32 0, i32 17} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!176 = metadata !{i32 127, i32 0, metadata !175, null}
!177 = metadata !{i32 132, i32 0, metadata !45, null}
!178 = metadata !{i32 157, i32 0, metadata !64, null}
!179 = metadata !{i32 158, i32 0, metadata !64, null}
!180 = metadata !{i32 159, i32 0, metadata !64, null}
!181 = metadata !{i32 162, i32 0, metadata !64, null}
!182 = metadata !{i32 163, i32 0, metadata !64, null}
!183 = metadata !{i32 166, i32 0, metadata !64, null}
!184 = metadata !{i32 167, i32 0, metadata !64, null}
!185 = metadata !{i32 170, i32 0, metadata !64, null}
!186 = metadata !{i32 173, i32 0, metadata !64, null}
!187 = metadata !{i32 16}
!188 = metadata !{i32 176, i32 0, metadata !64, null}
!189 = metadata !{i32 179, i32 0, metadata !64, null}
!190 = metadata !{i32 182, i32 0, metadata !64, null}
!191 = metadata !{i32 186, i32 0, metadata !64, null}
!192 = metadata !{i32 190, i32 0, metadata !87, null}
!193 = metadata !{i32 197, i32 0, metadata !91, null}
!194 = metadata !{i32 198, i32 0, metadata !91, null}
!195 = metadata !{i32 201, i32 0, metadata !91, null}
!196 = metadata !{i32 207, i32 0, metadata !90, null}
!197 = metadata !{i32 208, i32 0, metadata !90, null}
!198 = metadata !{i32 213, i32 0, metadata !91, null}
!199 = metadata !{i32 192, i32 0, metadata !87, null}
!200 = metadata !{i32 218, i32 0, metadata !64, null}
!201 = metadata !{i32 220, i32 0, metadata !64, null}

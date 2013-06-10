; ModuleID = 'matmul.cl'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @matmul_elem(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !15), !dbg !128
  tail call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !16), !dbg !129
  tail call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !17), !dbg !130
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !18), !dbg !131
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !19), !dbg !132
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !20), !dbg !133
  %call = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !134
  %conv = trunc i64 %call to i32, !dbg !134
  tail call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !23), !dbg !134
  %call1 = tail call spir_func i64 @get_global_id(i32 1) nounwind, !dbg !135
  %conv2 = trunc i64 %call1 to i32, !dbg !135
  tail call void @llvm.dbg.value(metadata !{i32 %conv2}, i64 0, metadata !24), !dbg !135
  %cmp = icmp slt i32 %conv, %Ndim, !dbg !136
  %cmp4 = icmp slt i32 %conv2, %Mdim, !dbg !136
  %or.cond = and i1 %cmp, %cmp4, !dbg !136
  br i1 %or.cond, label %for.cond.preheader, label %if.end, !dbg !136

for.cond.preheader:                               ; preds = %entry
  %cmp629 = icmp sgt i32 %Pdim, 0, !dbg !137
  %mul = mul nsw i32 %conv, %Ndim, !dbg !140
  br i1 %cmp629, label %for.body, label %for.end, !dbg !137

for.body:                                         ; preds = %for.cond.preheader, %for.body
  %tmp.031 = phi float [ %add13, %for.body ], [ 0.000000e+00, %for.cond.preheader ]
  %k.030 = phi i32 [ %inc, %for.body ], [ 0, %for.cond.preheader ]
  %add = add nsw i32 %k.030, %mul, !dbg !140
  %idxprom = sext i32 %add to i64, !dbg !140
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i64 %idxprom, !dbg !140
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !140, !tbaa !142
  %mul8 = mul nsw i32 %k.030, %Pdim, !dbg !140
  %add9 = add nsw i32 %mul8, %conv2, !dbg !140
  %idxprom10 = sext i32 %add9 to i64, !dbg !140
  %arrayidx11 = getelementptr inbounds float addrspace(1)* %B, i64 %idxprom10, !dbg !140
  %1 = load float addrspace(1)* %arrayidx11, align 4, !dbg !140, !tbaa !142
  %mul12 = fmul float %0, %1, !dbg !140
  %add13 = fadd float %tmp.031, %mul12, !dbg !140
  tail call void @llvm.dbg.value(metadata !{float %add13}, i64 0, metadata !25), !dbg !140
  %inc = add nsw i32 %k.030, 1, !dbg !137
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !21), !dbg !137
  %cmp6 = icmp slt i32 %inc, %Pdim, !dbg !137
  br i1 %cmp6, label %for.body, label %for.end, !dbg !137

for.end:                                          ; preds = %for.body, %for.cond.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond.preheader ], [ %add13, %for.body ]
  %add15 = add nsw i32 %mul, %conv2, !dbg !145
  %idxprom16 = sext i32 %add15 to i64, !dbg !145
  %arrayidx17 = getelementptr inbounds float addrspace(1)* %C, i64 %idxprom16, !dbg !145
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx17, align 4, !dbg !145, !tbaa !142
  br label %if.end, !dbg !146

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !147
}

declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone

declare spir_func i64 @get_global_id(i32)

define spir_kernel void @matmul_row(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !29), !dbg !148
  tail call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !30), !dbg !149
  tail call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !31), !dbg !150
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !32), !dbg !151
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !33), !dbg !152
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !34), !dbg !153
  %call = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !154
  %conv = trunc i64 %call to i32, !dbg !154
  tail call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !38), !dbg !154
  %cmp = icmp slt i32 %conv, %Ndim, !dbg !155
  %cmp237 = icmp sgt i32 %Mdim, 0, !dbg !156
  %or.cond = and i1 %cmp, %cmp237, !dbg !155
  br i1 %or.cond, label %for.cond4.preheader.lr.ph, label %if.end, !dbg !155

for.cond4.preheader.lr.ph:                        ; preds = %entry
  %cmp533 = icmp sgt i32 %Pdim, 0, !dbg !159
  %mul = mul nsw i32 %conv, %Ndim, !dbg !162
  br label %for.cond4.preheader, !dbg !156

for.cond4.preheader:                              ; preds = %for.cond4.preheader.lr.ph, %for.end
  %j.038 = phi i32 [ 0, %for.cond4.preheader.lr.ph ], [ %inc19, %for.end ]
  br i1 %cmp533, label %for.body7, label %for.end, !dbg !159

for.body7:                                        ; preds = %for.cond4.preheader, %for.body7
  %tmp.035 = phi float [ %add13, %for.body7 ], [ 0.000000e+00, %for.cond4.preheader ]
  %k.034 = phi i32 [ %inc, %for.body7 ], [ 0, %for.cond4.preheader ]
  %add = add nsw i32 %k.034, %mul, !dbg !162
  %idxprom = sext i32 %add to i64, !dbg !162
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i64 %idxprom, !dbg !162
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !162, !tbaa !142
  %mul8 = mul nsw i32 %k.034, %Pdim, !dbg !162
  %add9 = add nsw i32 %mul8, %j.038, !dbg !162
  %idxprom10 = sext i32 %add9 to i64, !dbg !162
  %arrayidx11 = getelementptr inbounds float addrspace(1)* %B, i64 %idxprom10, !dbg !162
  %1 = load float addrspace(1)* %arrayidx11, align 4, !dbg !162, !tbaa !142
  %mul12 = fmul float %0, %1, !dbg !162
  %add13 = fadd float %tmp.035, %mul12, !dbg !162
  tail call void @llvm.dbg.value(metadata !{float %add13}, i64 0, metadata !39), !dbg !162
  %inc = add nsw i32 %k.034, 1, !dbg !159
  tail call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !35), !dbg !159
  %cmp5 = icmp slt i32 %inc, %Pdim, !dbg !159
  br i1 %cmp5, label %for.body7, label %for.end, !dbg !159

for.end:                                          ; preds = %for.body7, %for.cond4.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond4.preheader ], [ %add13, %for.body7 ]
  %add15 = add nsw i32 %j.038, %mul, !dbg !163
  %idxprom16 = sext i32 %add15 to i64, !dbg !163
  %arrayidx17 = getelementptr inbounds float addrspace(1)* %C, i64 %idxprom16, !dbg !163
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx17, align 4, !dbg !163, !tbaa !142
  %inc19 = add nsw i32 %j.038, 1, !dbg !156
  tail call void @llvm.dbg.value(metadata !{i32 %inc19}, i64 0, metadata !37), !dbg !156
  %cmp2 = icmp slt i32 %inc19, %Mdim, !dbg !156
  br i1 %cmp2, label %for.cond4.preheader, label %if.end, !dbg !156

if.end:                                           ; preds = %for.end, %entry
  ret void, !dbg !164
}

define spir_kernel void @matmul_row_priv(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C) nounwind {
entry:
  %Awrk = alloca [16 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !43), !dbg !165
  call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !44), !dbg !166
  call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !45), !dbg !167
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !46), !dbg !168
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !47), !dbg !169
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !48), !dbg !170
  %call = call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !171
  %conv = trunc i64 %call to i32, !dbg !171
  call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !52), !dbg !171
  call void @llvm.dbg.declare(metadata !{[16 x float]* %Awrk}, metadata !53), !dbg !172
  %cmp = icmp slt i32 %conv, %Ndim, !dbg !173
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !173

for.cond.preheader:                               ; preds = %entry
  %cmp254 = icmp sgt i32 %Pdim, 0, !dbg !174
  br i1 %cmp254, label %for.body.lr.ph, label %for.cond6.preheader, !dbg !174

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %conv, %Ndim, !dbg !177
  br label %for.body, !dbg !174

for.cond6.preheader:                              ; preds = %for.body, %for.cond.preheader
  %cmp752 = icmp sgt i32 %Mdim, 0, !dbg !178
  br i1 %cmp752, label %for.cond10.preheader.lr.ph, label %if.end, !dbg !178

for.cond10.preheader.lr.ph:                       ; preds = %for.cond6.preheader
  %mul25 = mul nsw i32 %conv, %Ndim, !dbg !180
  br label %for.cond10.preheader, !dbg !178

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.055 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.055, %mul, !dbg !177
  %idxprom = sext i32 %add to i64, !dbg !177
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i64 %idxprom, !dbg !177
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !177, !tbaa !142
  %idxprom4 = sext i32 %k.055 to i64, !dbg !177
  %arrayidx5 = getelementptr inbounds [16 x float]* %Awrk, i64 0, i64 %idxprom4, !dbg !177
  store float %0, float* %arrayidx5, align 4, !dbg !177, !tbaa !142
  %inc = add nsw i32 %k.055, 1, !dbg !174
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !49), !dbg !174
  %cmp2 = icmp slt i32 %inc, %Pdim, !dbg !174
  br i1 %cmp2, label %for.body, label %for.cond6.preheader, !dbg !174

for.cond10.preheader:                             ; preds = %for.cond10.preheader.lr.ph, %for.end24
  %j.053 = phi i32 [ 0, %for.cond10.preheader.lr.ph ], [ %inc30, %for.end24 ]
  br i1 %cmp254, label %for.body13, label %for.end24, !dbg !182

for.body13:                                       ; preds = %for.cond10.preheader, %for.body13
  %tmp.051 = phi float [ %add21, %for.body13 ], [ 0.000000e+00, %for.cond10.preheader ]
  %k.150 = phi i32 [ %inc23, %for.body13 ], [ 0, %for.cond10.preheader ]
  %idxprom14 = sext i32 %k.150 to i64, !dbg !184
  %arrayidx15 = getelementptr inbounds [16 x float]* %Awrk, i64 0, i64 %idxprom14, !dbg !184
  %1 = load float* %arrayidx15, align 4, !dbg !184, !tbaa !142
  %mul16 = mul nsw i32 %k.150, %Pdim, !dbg !184
  %add17 = add nsw i32 %mul16, %j.053, !dbg !184
  %idxprom18 = sext i32 %add17 to i64, !dbg !184
  %arrayidx19 = getelementptr inbounds float addrspace(1)* %B, i64 %idxprom18, !dbg !184
  %2 = load float addrspace(1)* %arrayidx19, align 4, !dbg !184, !tbaa !142
  %mul20 = fmul float %1, %2, !dbg !184
  %add21 = fadd float %tmp.051, %mul20, !dbg !184
  call void @llvm.dbg.value(metadata !{float %add21}, i64 0, metadata !57), !dbg !184
  %inc23 = add nsw i32 %k.150, 1, !dbg !182
  call void @llvm.dbg.value(metadata !{i32 %inc23}, i64 0, metadata !49), !dbg !182
  %cmp11 = icmp slt i32 %inc23, %Pdim, !dbg !182
  br i1 %cmp11, label %for.body13, label %for.end24, !dbg !182

for.end24:                                        ; preds = %for.body13, %for.cond10.preheader
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.cond10.preheader ], [ %add21, %for.body13 ]
  %add26 = add nsw i32 %j.053, %mul25, !dbg !180
  %idxprom27 = sext i32 %add26 to i64, !dbg !180
  %arrayidx28 = getelementptr inbounds float addrspace(1)* %C, i64 %idxprom27, !dbg !180
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx28, align 4, !dbg !180, !tbaa !142
  %inc30 = add nsw i32 %j.053, 1, !dbg !178
  call void @llvm.dbg.value(metadata !{i32 %inc30}, i64 0, metadata !51), !dbg !178
  %cmp7 = icmp slt i32 %inc30, %Mdim, !dbg !178
  br i1 %cmp7, label %for.cond10.preheader, label %if.end, !dbg !178

if.end:                                           ; preds = %for.cond6.preheader, %for.end24, %entry
  ret void, !dbg !185
}

define spir_kernel void @matmul_row_local(i32 %Mdim, i32 %Ndim, i32 %Pdim, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C, float addrspace(3)* nocapture %Bwrk) nounwind {
entry:
  %Awrk = alloca [16 x float], align 4
  call void @llvm.dbg.value(metadata !{i32 %Mdim}, i64 0, metadata !63), !dbg !186
  call void @llvm.dbg.value(metadata !{i32 %Ndim}, i64 0, metadata !64), !dbg !187
  call void @llvm.dbg.value(metadata !{i32 %Pdim}, i64 0, metadata !65), !dbg !188
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !66), !dbg !189
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !67), !dbg !190
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !68), !dbg !191
  call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bwrk}, i64 0, metadata !69), !dbg !192
  %call = call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !193
  %conv = trunc i64 %call to i32, !dbg !193
  call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !73), !dbg !193
  %call1 = call spir_func i64 @get_local_id(i32 0) nounwind, !dbg !194
  %conv2 = trunc i64 %call1 to i32, !dbg !194
  call void @llvm.dbg.value(metadata !{i32 %conv2}, i64 0, metadata !74), !dbg !194
  %call3 = call spir_func i64 @get_local_size(i32 0) nounwind, !dbg !195
  %conv4 = trunc i64 %call3 to i32, !dbg !195
  call void @llvm.dbg.value(metadata !{i32 %conv4}, i64 0, metadata !75), !dbg !195
  call void @llvm.dbg.declare(metadata !{[16 x float]* %Awrk}, metadata !76), !dbg !196
  %cmp = icmp slt i32 %conv, %Ndim, !dbg !197
  br i1 %cmp, label %for.cond.preheader, label %if.end, !dbg !197

for.cond.preheader:                               ; preds = %entry
  %cmp677 = icmp sgt i32 %Pdim, 0, !dbg !198
  br i1 %cmp677, label %for.body.lr.ph, label %for.cond10.preheader, !dbg !198

for.body.lr.ph:                                   ; preds = %for.cond.preheader
  %mul = mul nsw i32 %conv, %Ndim, !dbg !201
  br label %for.body, !dbg !198

for.cond10.preheader:                             ; preds = %for.body, %for.cond.preheader
  %cmp1175 = icmp sgt i32 %Mdim, 0, !dbg !202
  br i1 %cmp1175, label %for.cond14.preheader.lr.ph, label %if.end, !dbg !202

for.cond14.preheader.lr.ph:                       ; preds = %for.cond10.preheader
  %cmp1570 = icmp slt i32 %conv2, %Pdim, !dbg !204
  %mul40 = mul nsw i32 %conv, %Ndim, !dbg !207
  br label %for.cond14.preheader, !dbg !202

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %k.078 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %k.078, %mul, !dbg !201
  %idxprom = sext i32 %add to i64, !dbg !201
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i64 %idxprom, !dbg !201
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !201, !tbaa !142
  %idxprom8 = sext i32 %k.078 to i64, !dbg !201
  %arrayidx9 = getelementptr inbounds [16 x float]* %Awrk, i64 0, i64 %idxprom8, !dbg !201
  store float %0, float* %arrayidx9, align 4, !dbg !201, !tbaa !142
  %inc = add nsw i32 %k.078, 1, !dbg !198
  call void @llvm.dbg.value(metadata !{i32 %inc}, i64 0, metadata !70), !dbg !198
  %cmp6 = icmp slt i32 %inc, %Pdim, !dbg !198
  br i1 %cmp6, label %for.body, label %for.cond10.preheader, !dbg !198

for.cond14.preheader:                             ; preds = %for.cond14.preheader.lr.ph, %for.end39
  %j.076 = phi i32 [ 0, %for.cond14.preheader.lr.ph ], [ %inc45, %for.end39 ]
  br i1 %cmp1570, label %for.body17, label %for.end26, !dbg !204

for.body17:                                       ; preds = %for.cond14.preheader, %for.body17
  %k.171 = phi i32 [ %add25, %for.body17 ], [ %conv2, %for.cond14.preheader ]
  %mul18 = mul nsw i32 %k.171, %Pdim, !dbg !208
  %add19 = add nsw i32 %mul18, %j.076, !dbg !208
  %idxprom20 = sext i32 %add19 to i64, !dbg !208
  %arrayidx21 = getelementptr inbounds float addrspace(1)* %B, i64 %idxprom20, !dbg !208
  %1 = load float addrspace(1)* %arrayidx21, align 4, !dbg !208, !tbaa !142
  %idxprom22 = sext i32 %k.171 to i64, !dbg !208
  %arrayidx23 = getelementptr inbounds float addrspace(3)* %Bwrk, i64 %idxprom22, !dbg !208
  store float %1, float addrspace(3)* %arrayidx23, align 4, !dbg !208, !tbaa !142
  %add25 = add nsw i32 %k.171, %conv4, !dbg !204
  call void @llvm.dbg.value(metadata !{i32 %add25}, i64 0, metadata !70), !dbg !204
  %cmp15 = icmp slt i32 %add25, %Pdim, !dbg !204
  br i1 %cmp15, label %for.body17, label %for.end26, !dbg !204

for.end26:                                        ; preds = %for.body17, %for.cond14.preheader
  call spir_func void @barrier(i32 1) nounwind, !dbg !209
  call void @llvm.dbg.value(metadata !210, i64 0, metadata !77), !dbg !211
  call void @llvm.dbg.value(metadata !2, i64 0, metadata !70), !dbg !212
  br i1 %cmp677, label %for.body30, label %for.end39, !dbg !212

for.body30:                                       ; preds = %for.end26, %for.body30
  %tmp.074 = phi float [ %add36, %for.body30 ], [ 0.000000e+00, %for.end26 ]
  %k.273 = phi i32 [ %inc38, %for.body30 ], [ 0, %for.end26 ]
  %idxprom31 = sext i32 %k.273 to i64, !dbg !214
  %arrayidx32 = getelementptr inbounds [16 x float]* %Awrk, i64 0, i64 %idxprom31, !dbg !214
  %2 = load float* %arrayidx32, align 4, !dbg !214, !tbaa !142
  %arrayidx34 = getelementptr inbounds float addrspace(3)* %Bwrk, i64 %idxprom31, !dbg !214
  %3 = load float addrspace(3)* %arrayidx34, align 4, !dbg !214, !tbaa !142
  %mul35 = fmul float %2, %3, !dbg !214
  %add36 = fadd float %tmp.074, %mul35, !dbg !214
  call void @llvm.dbg.value(metadata !{float %add36}, i64 0, metadata !77), !dbg !214
  %inc38 = add nsw i32 %k.273, 1, !dbg !212
  call void @llvm.dbg.value(metadata !{i32 %inc38}, i64 0, metadata !70), !dbg !212
  %cmp28 = icmp slt i32 %inc38, %Pdim, !dbg !212
  br i1 %cmp28, label %for.body30, label %for.end39, !dbg !212

for.end39:                                        ; preds = %for.body30, %for.end26
  %tmp.0.lcssa = phi float [ 0.000000e+00, %for.end26 ], [ %add36, %for.body30 ]
  %add41 = add nsw i32 %j.076, %mul40, !dbg !207
  %idxprom42 = sext i32 %add41 to i64, !dbg !207
  %arrayidx43 = getelementptr inbounds float addrspace(1)* %C, i64 %idxprom42, !dbg !207
  store float %tmp.0.lcssa, float addrspace(1)* %arrayidx43, align 4, !dbg !207, !tbaa !142
  call spir_func void @barrier(i32 1) nounwind, !dbg !215
  %inc45 = add nsw i32 %j.076, 1, !dbg !202
  call void @llvm.dbg.value(metadata !{i32 %inc45}, i64 0, metadata !72), !dbg !202
  %cmp11 = icmp slt i32 %inc45, %Mdim, !dbg !202
  br i1 %cmp11, label %for.cond14.preheader, label %if.end, !dbg !202

if.end:                                           ; preds = %for.cond10.preheader, %for.end39, %entry
  ret void, !dbg !216
}

declare spir_func i64 @get_local_id(i32)

declare spir_func i64 @get_local_size(i32)

declare spir_func void @barrier(i32)

define spir_kernel void @matmul_block(i32 %uiWA, i32 %uiWB, i32 %uiWC, float addrspace(1)* nocapture %A, float addrspace(1)* nocapture %B, float addrspace(1)* nocapture %C, float addrspace(3)* nocapture %As, float addrspace(3)* nocapture %Bs) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %uiWA}, i64 0, metadata !83), !dbg !217
  tail call void @llvm.dbg.value(metadata !{i32 %uiWB}, i64 0, metadata !84), !dbg !217
  tail call void @llvm.dbg.value(metadata !{i32 %uiWC}, i64 0, metadata !85), !dbg !217
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %A}, i64 0, metadata !86), !dbg !218
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %B}, i64 0, metadata !87), !dbg !218
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %C}, i64 0, metadata !88), !dbg !218
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %As}, i64 0, metadata !89), !dbg !219
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %Bs}, i64 0, metadata !90), !dbg !219
  %call = tail call spir_func i64 @get_group_id(i32 0) nounwind, !dbg !220
  tail call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !91), !dbg !220
  %call1 = tail call spir_func i64 @get_group_id(i32 1) nounwind, !dbg !221
  %conv2 = trunc i64 %call1 to i32, !dbg !221
  tail call void @llvm.dbg.value(metadata !{i32 %conv2}, i64 0, metadata !93), !dbg !221
  %call3 = tail call spir_func i64 @get_local_id(i32 0) nounwind, !dbg !222
  %conv4 = trunc i64 %call3 to i32, !dbg !222
  tail call void @llvm.dbg.value(metadata !{i32 %conv4}, i64 0, metadata !94), !dbg !222
  %call5 = tail call spir_func i64 @get_local_id(i32 1) nounwind, !dbg !223
  %conv6 = trunc i64 %call5 to i32, !dbg !223
  tail call void @llvm.dbg.value(metadata !{i32 %conv6}, i64 0, metadata !95), !dbg !223
  %mul = shl nsw i32 %uiWA, 1, !dbg !224
  %mul7 = mul nsw i32 %conv2, %mul, !dbg !224
  tail call void @llvm.dbg.value(metadata !{i32 %mul7}, i64 0, metadata !96), !dbg !224
  %add = add i32 %uiWA, -1, !dbg !225
  %sub = add i32 %add, %mul7, !dbg !225
  tail call void @llvm.dbg.value(metadata !{i32 %sub}, i64 0, metadata !97), !dbg !225
  tail call void @llvm.dbg.value(metadata !226, i64 0, metadata !98), !dbg !227
  tail call void @llvm.dbg.value(metadata !{i32 %mul8}, i64 0, metadata !99), !dbg !228
  %mul9 = shl i32 %uiWB, 1, !dbg !229
  tail call void @llvm.dbg.value(metadata !{i32 %mul9}, i64 0, metadata !100), !dbg !229
  tail call void @llvm.dbg.value(metadata !210, i64 0, metadata !101), !dbg !230
  tail call void @llvm.dbg.value(metadata !{i32 %mul7}, i64 0, metadata !102), !dbg !231
  tail call void @llvm.dbg.value(metadata !{i32 %mul8}, i64 0, metadata !104), !dbg !231
  %cmp74 = icmp sgt i32 %mul7, %sub, !dbg !231
  br i1 %cmp74, label %for.end44, label %for.body.lr.ph, !dbg !231

for.body.lr.ph:                                   ; preds = %entry
  %conv = trunc i64 %call to i32, !dbg !220
  %mul8 = shl nsw i32 %conv, 1, !dbg !228
  %mul11 = mul nsw i32 %conv6, %uiWA, !dbg !232
  %add12 = add i32 %mul11, %conv4, !dbg !232
  %mul14 = shl nsw i32 %conv6, 1, !dbg !232
  %add15 = add nsw i32 %mul14, %conv4, !dbg !232
  %idxprom16 = sext i32 %add15 to i64, !dbg !232
  %arrayidx17 = getelementptr inbounds float addrspace(3)* %As, i64 %idxprom16, !dbg !232
  %mul18 = mul nsw i32 %conv6, %uiWB, !dbg !233
  %add19 = add i32 %mul18, %conv4, !dbg !233
  %arrayidx26 = getelementptr inbounds float addrspace(3)* %Bs, i64 %idxprom16, !dbg !233
  %idxprom33 = sext i32 %mul14 to i64, !dbg !234
  %arrayidx34 = getelementptr inbounds float addrspace(3)* %As, i64 %idxprom33, !dbg !234
  br label %for.body, !dbg !231

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %b.077 = phi i32 [ %mul8, %for.body.lr.ph ], [ %add43, %for.body ]
  %a.076 = phi i32 [ %mul7, %for.body.lr.ph ], [ %add42, %for.body ]
  %Csub.075 = phi float [ 0.000000e+00, %for.body.lr.ph ], [ %add40.1, %for.body ]
  %add13 = add i32 %add12, %a.076, !dbg !232
  %idxprom = sext i32 %add13 to i64, !dbg !232
  %arrayidx = getelementptr inbounds float addrspace(1)* %A, i64 %idxprom, !dbg !232
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !232, !tbaa !142
  store float %0, float addrspace(3)* %arrayidx17, align 4, !dbg !232, !tbaa !142
  %add20 = add i32 %add19, %b.077, !dbg !233
  %idxprom21 = sext i32 %add20 to i64, !dbg !233
  %arrayidx22 = getelementptr inbounds float addrspace(1)* %B, i64 %idxprom21, !dbg !233
  %1 = load float addrspace(1)* %arrayidx22, align 4, !dbg !233, !tbaa !142
  store float %1, float addrspace(3)* %arrayidx26, align 4, !dbg !233, !tbaa !142
  tail call spir_func void @barrier(i32 1) nounwind, !dbg !235
  tail call void @llvm.dbg.value(metadata !2, i64 0, metadata !105), !dbg !236
  %2 = load float addrspace(3)* %arrayidx34, align 4, !dbg !234, !tbaa !142
  %idxprom37 = sext i32 %conv4 to i64, !dbg !234
  %arrayidx38 = getelementptr inbounds float addrspace(3)* %Bs, i64 %idxprom37, !dbg !234
  %3 = load float addrspace(3)* %arrayidx38, align 4, !dbg !234, !tbaa !142
  %mul39 = fmul float %2, %3, !dbg !234
  %add40 = fadd float %Csub.075, %mul39, !dbg !234
  tail call void @llvm.dbg.value(metadata !{float %add40.1}, i64 0, metadata !101), !dbg !234
  tail call void @llvm.dbg.value(metadata !237, i64 0, metadata !105), !dbg !236
  %add32.178 = or i32 %mul14, 1, !dbg !234
  %idxprom33.1 = sext i32 %add32.178 to i64, !dbg !234
  %arrayidx34.1 = getelementptr inbounds float addrspace(3)* %As, i64 %idxprom33.1, !dbg !234
  %4 = load float addrspace(3)* %arrayidx34.1, align 4, !dbg !234, !tbaa !142
  %add36.1 = add nsw i32 %conv4, 2, !dbg !234
  %idxprom37.1 = sext i32 %add36.1 to i64, !dbg !234
  %arrayidx38.1 = getelementptr inbounds float addrspace(3)* %Bs, i64 %idxprom37.1, !dbg !234
  %5 = load float addrspace(3)* %arrayidx38.1, align 4, !dbg !234, !tbaa !142
  %mul39.1 = fmul float %4, %5, !dbg !234
  %add40.1 = fadd float %add40, %mul39.1, !dbg !234
  tail call void @llvm.dbg.value(metadata !{float %add40.1}, i64 0, metadata !101), !dbg !234
  tail call void @llvm.dbg.value(metadata !237, i64 0, metadata !105), !dbg !236
  tail call spir_func void @barrier(i32 1) nounwind, !dbg !238
  %add42 = add nsw i32 %a.076, 2, !dbg !239
  tail call void @llvm.dbg.value(metadata !{i32 %add42}, i64 0, metadata !102), !dbg !239
  %add43 = add nsw i32 %b.077, %mul9, !dbg !239
  tail call void @llvm.dbg.value(metadata !{i32 %add43}, i64 0, metadata !104), !dbg !239
  %cmp = icmp sgt i32 %add42, %sub, !dbg !231
  br i1 %cmp, label %for.end44, label %for.body, !dbg !231

for.end44:                                        ; preds = %for.body, %entry
  %Csub.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %add40.1, %for.body ]
  %call45 = tail call spir_func i64 @get_global_id(i32 1) nounwind, !dbg !240
  %call46 = tail call spir_func i64 @get_global_size(i32 0) nounwind, !dbg !240
  %mul47 = mul i64 %call46, %call45, !dbg !240
  %call48 = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !240
  %add49 = add i64 %mul47, %call48, !dbg !240
  %arrayidx50 = getelementptr inbounds float addrspace(1)* %C, i64 %add49, !dbg !240
  store float %Csub.0.lcssa, float addrspace(1)* %arrayidx50, align 4, !dbg !240, !tbaa !142
  ret void, !dbg !241
}

declare spir_func i64 @get_group_id(i32)

declare spir_func i64 @get_global_size(i32)

declare void @llvm.dbg.value(metadata, i64, metadata) nounwind readnone

!llvm.dbg.cu = !{!0}
!opencl.kernels = !{!108, !114, !115, !116, !122}
!opencl.enable.FP_CONTRACT = !{}

!0 = metadata !{i32 786449, i32 0, i32 12, metadata !"<unknown>", metadata !"/Users/james/projects/oclgrind/tests", metadata !"clang version 3.2 (tags/RELEASE_32/final 183304)", i1 true, i1 true, metadata !"", i32 0, metadata !1, metadata !1, metadata !3, metadata !1} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/<unknown>] [DW_LANG_C99]
!1 = metadata !{metadata !2}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4}
!4 = metadata !{metadata !5, metadata !26, metadata !40, metadata !58, metadata !78}
!5 = metadata !{i32 786478, i32 0, metadata !6, metadata !"matmul_elem", metadata !"matmul_elem", metadata !"", metadata !6, i32 3, metadata !7, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_elem, null, null, metadata !13, i32 10} ; [ DW_TAG_subprogram ] [line 3] [def] [scope 10] [matmul_elem]
!6 = metadata !{i32 786473, metadata !"matmul.cl", metadata !"/Users/james/projects/oclgrind/tests", null} ; [ DW_TAG_file_type ]
!7 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !8, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!8 = metadata !{null, metadata !9, metadata !9, metadata !9, metadata !11, metadata !11, metadata !11}
!9 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !10} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from int]
!10 = metadata !{i32 786468, null, metadata !"int", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!11 = metadata !{i32 786447, null, metadata !"", null, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !12} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from float]
!12 = metadata !{i32 786468, null, metadata !"float", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [float] [line 0, size 32, align 32, offset 0, enc DW_ATE_float]
!13 = metadata !{metadata !14}
!14 = metadata !{metadata !15, metadata !16, metadata !17, metadata !18, metadata !19, metadata !20, metadata !21, metadata !23, metadata !24, metadata !25}
!15 = metadata !{i32 786689, metadata !5, metadata !"Mdim", metadata !6, i32 16777220, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 4]
!16 = metadata !{i32 786689, metadata !5, metadata !"Ndim", metadata !6, i32 33554437, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 5]
!17 = metadata !{i32 786689, metadata !5, metadata !"Pdim", metadata !6, i32 50331654, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 6]
!18 = metadata !{i32 786689, metadata !5, metadata !"A", metadata !6, i32 67108871, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 7]
!19 = metadata !{i32 786689, metadata !5, metadata !"B", metadata !6, i32 83886088, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 8]
!20 = metadata !{i32 786689, metadata !5, metadata !"C", metadata !6, i32 100663305, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 9]
!21 = metadata !{i32 786688, metadata !22, metadata !"k", metadata !6, i32 11, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 11]
!22 = metadata !{i32 786443, metadata !5, i32 10, i32 0, metadata !6, i32 0} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!23 = metadata !{i32 786688, metadata !22, metadata !"i", metadata !6, i32 12, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 12]
!24 = metadata !{i32 786688, metadata !22, metadata !"j", metadata !6, i32 13, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 13]
!25 = metadata !{i32 786688, metadata !22, metadata !"tmp", metadata !6, i32 14, metadata !12, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 14]
!26 = metadata !{i32 786478, i32 0, metadata !6, metadata !"matmul_row", metadata !"matmul_row", metadata !"", metadata !6, i32 26, metadata !7, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row, null, null, metadata !27, i32 33} ; [ DW_TAG_subprogram ] [line 26] [def] [scope 33] [matmul_row]
!27 = metadata !{metadata !28}
!28 = metadata !{metadata !29, metadata !30, metadata !31, metadata !32, metadata !33, metadata !34, metadata !35, metadata !37, metadata !38, metadata !39}
!29 = metadata !{i32 786689, metadata !26, metadata !"Mdim", metadata !6, i32 16777243, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 27]
!30 = metadata !{i32 786689, metadata !26, metadata !"Ndim", metadata !6, i32 33554460, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 28]
!31 = metadata !{i32 786689, metadata !26, metadata !"Pdim", metadata !6, i32 50331677, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 29]
!32 = metadata !{i32 786689, metadata !26, metadata !"A", metadata !6, i32 67108894, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 30]
!33 = metadata !{i32 786689, metadata !26, metadata !"B", metadata !6, i32 83886111, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 31]
!34 = metadata !{i32 786689, metadata !26, metadata !"C", metadata !6, i32 100663328, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 32]
!35 = metadata !{i32 786688, metadata !36, metadata !"k", metadata !6, i32 34, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 34]
!36 = metadata !{i32 786443, metadata !26, i32 33, i32 0, metadata !6, i32 4} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!37 = metadata !{i32 786688, metadata !36, metadata !"j", metadata !6, i32 34, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 34]
!38 = metadata !{i32 786688, metadata !36, metadata !"i", metadata !6, i32 35, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 35]
!39 = metadata !{i32 786688, metadata !36, metadata !"tmp", metadata !6, i32 36, metadata !12, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 36]
!40 = metadata !{i32 786478, i32 0, metadata !6, metadata !"matmul_row_priv", metadata !"matmul_row_priv", metadata !"", metadata !6, i32 48, metadata !7, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row_priv, null, null, metadata !41, i32 55} ; [ DW_TAG_subprogram ] [line 48] [def] [scope 55] [matmul_row_priv]
!41 = metadata !{metadata !42}
!42 = metadata !{metadata !43, metadata !44, metadata !45, metadata !46, metadata !47, metadata !48, metadata !49, metadata !51, metadata !52, metadata !53, metadata !57}
!43 = metadata !{i32 786689, metadata !40, metadata !"Mdim", metadata !6, i32 16777265, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 49]
!44 = metadata !{i32 786689, metadata !40, metadata !"Ndim", metadata !6, i32 33554482, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 50]
!45 = metadata !{i32 786689, metadata !40, metadata !"Pdim", metadata !6, i32 50331699, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 51]
!46 = metadata !{i32 786689, metadata !40, metadata !"A", metadata !6, i32 67108916, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 52]
!47 = metadata !{i32 786689, metadata !40, metadata !"B", metadata !6, i32 83886133, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 53]
!48 = metadata !{i32 786689, metadata !40, metadata !"C", metadata !6, i32 100663350, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 54]
!49 = metadata !{i32 786688, metadata !50, metadata !"k", metadata !6, i32 56, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 56]
!50 = metadata !{i32 786443, metadata !40, i32 55, i32 0, metadata !6, i32 9} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!51 = metadata !{i32 786688, metadata !50, metadata !"j", metadata !6, i32 56, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 56]
!52 = metadata !{i32 786688, metadata !50, metadata !"i", metadata !6, i32 57, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 57]
!53 = metadata !{i32 786688, metadata !50, metadata !"Awrk", metadata !6, i32 58, metadata !54, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Awrk] [line 58]
!54 = metadata !{i32 786433, null, metadata !"", null, i32 0, i64 512, i64 32, i32 0, i32 0, metadata !12, metadata !55, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 512, align 32, offset 0] [from float]
!55 = metadata !{metadata !56}
!56 = metadata !{i32 786465, i64 0, i64 15}       ; [ DW_TAG_subrange_type ] [0, 15]
!57 = metadata !{i32 786688, metadata !50, metadata !"tmp", metadata !6, i32 59, metadata !12, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 59]
!58 = metadata !{i32 786478, i32 0, metadata !6, metadata !"matmul_row_local", metadata !"matmul_row_local", metadata !"", metadata !6, i32 74, metadata !59, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local, null, null, metadata !61, i32 82} ; [ DW_TAG_subprogram ] [line 74] [def] [scope 82] [matmul_row_local]
!59 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !60, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!60 = metadata !{null, metadata !9, metadata !9, metadata !9, metadata !11, metadata !11, metadata !11, metadata !11}
!61 = metadata !{metadata !62}
!62 = metadata !{metadata !63, metadata !64, metadata !65, metadata !66, metadata !67, metadata !68, metadata !69, metadata !70, metadata !72, metadata !73, metadata !74, metadata !75, metadata !76, metadata !77}
!63 = metadata !{i32 786689, metadata !58, metadata !"Mdim", metadata !6, i32 16777291, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Mdim] [line 75]
!64 = metadata !{i32 786689, metadata !58, metadata !"Ndim", metadata !6, i32 33554508, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Ndim] [line 76]
!65 = metadata !{i32 786689, metadata !58, metadata !"Pdim", metadata !6, i32 50331725, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Pdim] [line 77]
!66 = metadata !{i32 786689, metadata !58, metadata !"A", metadata !6, i32 67108942, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 78]
!67 = metadata !{i32 786689, metadata !58, metadata !"B", metadata !6, i32 83886159, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 79]
!68 = metadata !{i32 786689, metadata !58, metadata !"C", metadata !6, i32 100663376, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 80]
!69 = metadata !{i32 786689, metadata !58, metadata !"Bwrk", metadata !6, i32 117440593, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bwrk] [line 81]
!70 = metadata !{i32 786688, metadata !71, metadata !"k", metadata !6, i32 83, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 83]
!71 = metadata !{i32 786443, metadata !58, i32 82, i32 0, metadata !6, i32 15} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!72 = metadata !{i32 786688, metadata !71, metadata !"j", metadata !6, i32 83, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 83]
!73 = metadata !{i32 786688, metadata !71, metadata !"i", metadata !6, i32 84, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 84]
!74 = metadata !{i32 786688, metadata !71, metadata !"iloc", metadata !6, i32 85, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [iloc] [line 85]
!75 = metadata !{i32 786688, metadata !71, metadata !"nloc", metadata !6, i32 86, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [nloc] [line 86]
!76 = metadata !{i32 786688, metadata !71, metadata !"Awrk", metadata !6, i32 87, metadata !54, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Awrk] [line 87]
!77 = metadata !{i32 786688, metadata !71, metadata !"tmp", metadata !6, i32 88, metadata !12, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tmp] [line 88]
!78 = metadata !{i32 786478, i32 0, metadata !6, metadata !"matmul_block", metadata !"matmul_block", metadata !"", metadata !6, i32 130, metadata !79, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block, null, null, metadata !81, i32 133} ; [ DW_TAG_subprogram ] [line 130] [def] [scope 133] [matmul_block]
!79 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !80, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!80 = metadata !{null, metadata !10, metadata !10, metadata !10, metadata !11, metadata !11, metadata !11, metadata !11, metadata !11}
!81 = metadata !{metadata !82}
!82 = metadata !{metadata !83, metadata !84, metadata !85, metadata !86, metadata !87, metadata !88, metadata !89, metadata !90, metadata !91, metadata !93, metadata !94, metadata !95, metadata !96, metadata !97, metadata !98, metadata !99, metadata !100, metadata !101, metadata !102, metadata !104, metadata !105}
!83 = metadata !{i32 786689, metadata !78, metadata !"uiWA", metadata !6, i32 16777346, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWA] [line 130]
!84 = metadata !{i32 786689, metadata !78, metadata !"uiWB", metadata !6, i32 33554562, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWB] [line 130]
!85 = metadata !{i32 786689, metadata !78, metadata !"uiWC", metadata !6, i32 50331778, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [uiWC] [line 130]
!86 = metadata !{i32 786689, metadata !78, metadata !"A", metadata !6, i32 67108995, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [A] [line 131]
!87 = metadata !{i32 786689, metadata !78, metadata !"B", metadata !6, i32 83886211, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [B] [line 131]
!88 = metadata !{i32 786689, metadata !78, metadata !"C", metadata !6, i32 100663427, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [C] [line 131]
!89 = metadata !{i32 786689, metadata !78, metadata !"As", metadata !6, i32 117440644, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [As] [line 132]
!90 = metadata !{i32 786689, metadata !78, metadata !"Bs", metadata !6, i32 134217860, metadata !11, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [Bs] [line 132]
!91 = metadata !{i32 786688, metadata !92, metadata !"bx", metadata !6, i32 135, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bx] [line 135]
!92 = metadata !{i32 786443, metadata !78, i32 133, i32 0, metadata !6, i32 22} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!93 = metadata !{i32 786688, metadata !92, metadata !"by", metadata !6, i32 136, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [by] [line 136]
!94 = metadata !{i32 786688, metadata !92, metadata !"tx", metadata !6, i32 139, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [tx] [line 139]
!95 = metadata !{i32 786688, metadata !92, metadata !"ty", metadata !6, i32 140, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [ty] [line 140]
!96 = metadata !{i32 786688, metadata !92, metadata !"aBegin", metadata !6, i32 143, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aBegin] [line 143]
!97 = metadata !{i32 786688, metadata !92, metadata !"aEnd", metadata !6, i32 146, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aEnd] [line 146]
!98 = metadata !{i32 786688, metadata !92, metadata !"aStep", metadata !6, i32 149, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [aStep] [line 149]
!99 = metadata !{i32 786688, metadata !92, metadata !"bBegin", metadata !6, i32 152, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bBegin] [line 152]
!100 = metadata !{i32 786688, metadata !92, metadata !"bStep", metadata !6, i32 155, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [bStep] [line 155]
!101 = metadata !{i32 786688, metadata !92, metadata !"Csub", metadata !6, i32 159, metadata !12, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [Csub] [line 159]
!102 = metadata !{i32 786688, metadata !103, metadata !"a", metadata !6, i32 163, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [a] [line 163]
!103 = metadata !{i32 786443, metadata !92, i32 163, i32 0, metadata !6, i32 23} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!104 = metadata !{i32 786688, metadata !103, metadata !"b", metadata !6, i32 163, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [b] [line 163]
!105 = metadata !{i32 786688, metadata !106, metadata !"k", metadata !6, i32 180, metadata !10, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [k] [line 180]
!106 = metadata !{i32 786443, metadata !107, i32 180, i32 0, metadata !6, i32 25} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!107 = metadata !{i32 786443, metadata !103, i32 165, i32 0, metadata !6, i32 24} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!108 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_elem, metadata !109, metadata !110, metadata !111, metadata !112, metadata !113}
!109 = metadata !{metadata !"kernel_arg_addr_space", i32 0, i32 0, i32 0, i32 1, i32 1, i32 1}
!110 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none"}
!111 = metadata !{metadata !"kernel_arg_type", metadata !"int", metadata !"int", metadata !"int", metadata !"float*", metadata !"float*", metadata !"float*"}
!112 = metadata !{metadata !"kernel_arg_type_qual", metadata !"const", metadata !"const", metadata !"const", metadata !"", metadata !"", metadata !""}
!113 = metadata !{metadata !"kernel_arg_name", metadata !"Mdim", metadata !"Ndim", metadata !"Pdim", metadata !"A", metadata !"B", metadata !"C"}
!114 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row, metadata !109, metadata !110, metadata !111, metadata !112, metadata !113}
!115 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @matmul_row_priv, metadata !109, metadata !110, metadata !111, metadata !112, metadata !113}
!116 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @matmul_row_local, metadata !117, metadata !118, metadata !119, metadata !120, metadata !121}
!117 = metadata !{metadata !"kernel_arg_addr_space", i32 0, i32 0, i32 0, i32 1, i32 1, i32 1, i32 3}
!118 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none"}
!119 = metadata !{metadata !"kernel_arg_type", metadata !"int", metadata !"int", metadata !"int", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*"}
!120 = metadata !{metadata !"kernel_arg_type_qual", metadata !"const", metadata !"const", metadata !"const", metadata !"", metadata !"", metadata !"", metadata !""}
!121 = metadata !{metadata !"kernel_arg_name", metadata !"Mdim", metadata !"Ndim", metadata !"Pdim", metadata !"A", metadata !"B", metadata !"C", metadata !"Bwrk"}
!122 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*, float addrspace(3)*)* @matmul_block, metadata !123, metadata !124, metadata !125, metadata !126, metadata !127}
!123 = metadata !{metadata !"kernel_arg_addr_space", i32 0, i32 0, i32 0, i32 1, i32 1, i32 1, i32 3, i32 3}
!124 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none"}
!125 = metadata !{metadata !"kernel_arg_type", metadata !"int", metadata !"int", metadata !"int", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*"}
!126 = metadata !{metadata !"kernel_arg_type_qual", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !""}
!127 = metadata !{metadata !"kernel_arg_name", metadata !"uiWA", metadata !"uiWB", metadata !"uiWC", metadata !"A", metadata !"B", metadata !"C", metadata !"As", metadata !"Bs"}
!128 = metadata !{i32 4, i32 0, metadata !5, null}
!129 = metadata !{i32 5, i32 0, metadata !5, null}
!130 = metadata !{i32 6, i32 0, metadata !5, null}
!131 = metadata !{i32 7, i32 0, metadata !5, null}
!132 = metadata !{i32 8, i32 0, metadata !5, null}
!133 = metadata !{i32 9, i32 0, metadata !5, null}
!134 = metadata !{i32 12, i32 0, metadata !22, null}
!135 = metadata !{i32 13, i32 0, metadata !22, null}
!136 = metadata !{i32 15, i32 0, metadata !22, null}
!137 = metadata !{i32 18, i32 0, metadata !138, null}
!138 = metadata !{i32 786443, metadata !139, i32 18, i32 0, metadata !6, i32 2} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!139 = metadata !{i32 786443, metadata !22, i32 16, i32 0, metadata !6, i32 1} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!140 = metadata !{i32 20, i32 0, metadata !141, null}
!141 = metadata !{i32 786443, metadata !138, i32 19, i32 0, metadata !6, i32 3} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!142 = metadata !{metadata !"float", metadata !143}
!143 = metadata !{metadata !"omnipotent char", metadata !144}
!144 = metadata !{metadata !"Simple C/C++ TBAA"}
!145 = metadata !{i32 22, i32 0, metadata !139, null}
!146 = metadata !{i32 23, i32 0, metadata !139, null}
!147 = metadata !{i32 24, i32 0, metadata !22, null}
!148 = metadata !{i32 27, i32 0, metadata !26, null}
!149 = metadata !{i32 28, i32 0, metadata !26, null}
!150 = metadata !{i32 29, i32 0, metadata !26, null}
!151 = metadata !{i32 30, i32 0, metadata !26, null}
!152 = metadata !{i32 31, i32 0, metadata !26, null}
!153 = metadata !{i32 32, i32 0, metadata !26, null}
!154 = metadata !{i32 35, i32 0, metadata !36, null}
!155 = metadata !{i32 37, i32 0, metadata !36, null}
!156 = metadata !{i32 39, i32 0, metadata !157, null}
!157 = metadata !{i32 786443, metadata !158, i32 39, i32 0, metadata !6, i32 6} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!158 = metadata !{i32 786443, metadata !36, i32 38, i32 0, metadata !6, i32 5} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!159 = metadata !{i32 41, i32 0, metadata !160, null}
!160 = metadata !{i32 786443, metadata !161, i32 41, i32 0, metadata !6, i32 8} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!161 = metadata !{i32 786443, metadata !157, i32 39, i32 0, metadata !6, i32 7} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!162 = metadata !{i32 42, i32 0, metadata !160, null}
!163 = metadata !{i32 43, i32 0, metadata !161, null}
!164 = metadata !{i32 46, i32 0, metadata !36, null}
!165 = metadata !{i32 49, i32 0, metadata !40, null}
!166 = metadata !{i32 50, i32 0, metadata !40, null}
!167 = metadata !{i32 51, i32 0, metadata !40, null}
!168 = metadata !{i32 52, i32 0, metadata !40, null}
!169 = metadata !{i32 53, i32 0, metadata !40, null}
!170 = metadata !{i32 54, i32 0, metadata !40, null}
!171 = metadata !{i32 57, i32 0, metadata !50, null}
!172 = metadata !{i32 58, i32 0, metadata !50, null}
!173 = metadata !{i32 60, i32 0, metadata !50, null}
!174 = metadata !{i32 62, i32 0, metadata !175, null}
!175 = metadata !{i32 786443, metadata !176, i32 62, i32 0, metadata !6, i32 11} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!176 = metadata !{i32 786443, metadata !50, i32 61, i32 0, metadata !6, i32 10} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!177 = metadata !{i32 63, i32 0, metadata !175, null}
!178 = metadata !{i32 65, i32 0, metadata !179, null}
!179 = metadata !{i32 786443, metadata !176, i32 65, i32 0, metadata !6, i32 12} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!180 = metadata !{i32 69, i32 0, metadata !181, null}
!181 = metadata !{i32 786443, metadata !179, i32 65, i32 0, metadata !6, i32 13} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!182 = metadata !{i32 67, i32 0, metadata !183, null}
!183 = metadata !{i32 786443, metadata !181, i32 67, i32 0, metadata !6, i32 14} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!184 = metadata !{i32 68, i32 0, metadata !183, null}
!185 = metadata !{i32 72, i32 0, metadata !50, null}
!186 = metadata !{i32 75, i32 0, metadata !58, null}
!187 = metadata !{i32 76, i32 0, metadata !58, null}
!188 = metadata !{i32 77, i32 0, metadata !58, null}
!189 = metadata !{i32 78, i32 0, metadata !58, null}
!190 = metadata !{i32 79, i32 0, metadata !58, null}
!191 = metadata !{i32 80, i32 0, metadata !58, null}
!192 = metadata !{i32 81, i32 0, metadata !58, null}
!193 = metadata !{i32 84, i32 0, metadata !71, null}
!194 = metadata !{i32 85, i32 0, metadata !71, null}
!195 = metadata !{i32 86, i32 0, metadata !71, null}
!196 = metadata !{i32 87, i32 0, metadata !71, null}
!197 = metadata !{i32 89, i32 0, metadata !71, null}
!198 = metadata !{i32 91, i32 0, metadata !199, null}
!199 = metadata !{i32 786443, metadata !200, i32 91, i32 0, metadata !6, i32 17} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!200 = metadata !{i32 786443, metadata !71, i32 90, i32 0, metadata !6, i32 16} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!201 = metadata !{i32 92, i32 0, metadata !199, null}
!202 = metadata !{i32 94, i32 0, metadata !203, null}
!203 = metadata !{i32 786443, metadata !200, i32 94, i32 0, metadata !6, i32 18} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!204 = metadata !{i32 95, i32 0, metadata !205, null}
!205 = metadata !{i32 786443, metadata !206, i32 95, i32 0, metadata !6, i32 20} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!206 = metadata !{i32 786443, metadata !203, i32 94, i32 0, metadata !6, i32 19} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!207 = metadata !{i32 101, i32 0, metadata !206, null}
!208 = metadata !{i32 96, i32 0, metadata !205, null}
!209 = metadata !{i32 97, i32 0, metadata !206, null}
!210 = metadata !{float 0.000000e+00}
!211 = metadata !{i32 98, i32 0, metadata !206, null}
!212 = metadata !{i32 99, i32 0, metadata !213, null}
!213 = metadata !{i32 786443, metadata !206, i32 99, i32 0, metadata !6, i32 21} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/matmul.cl]
!214 = metadata !{i32 100, i32 0, metadata !213, null}
!215 = metadata !{i32 102, i32 0, metadata !206, null}
!216 = metadata !{i32 105, i32 0, metadata !71, null}
!217 = metadata !{i32 130, i32 0, metadata !78, null}
!218 = metadata !{i32 131, i32 0, metadata !78, null}
!219 = metadata !{i32 132, i32 0, metadata !78, null}
!220 = metadata !{i32 135, i32 0, metadata !92, null}
!221 = metadata !{i32 136, i32 0, metadata !92, null}
!222 = metadata !{i32 139, i32 0, metadata !92, null}
!223 = metadata !{i32 140, i32 0, metadata !92, null}
!224 = metadata !{i32 143, i32 0, metadata !92, null}
!225 = metadata !{i32 146, i32 0, metadata !92, null}
!226 = metadata !{i32 2}
!227 = metadata !{i32 149, i32 0, metadata !92, null}
!228 = metadata !{i32 152, i32 0, metadata !92, null}
!229 = metadata !{i32 155, i32 0, metadata !92, null}
!230 = metadata !{i32 159, i32 0, metadata !92, null}
!231 = metadata !{i32 163, i32 0, metadata !103, null}
!232 = metadata !{i32 170, i32 0, metadata !107, null}
!233 = metadata !{i32 171, i32 0, metadata !107, null}
!234 = metadata !{i32 181, i32 0, metadata !106, null}
!235 = metadata !{i32 174, i32 0, metadata !107, null}
!236 = metadata !{i32 180, i32 0, metadata !106, null}
!237 = metadata !{i32 1}
!238 = metadata !{i32 186, i32 0, metadata !107, null}
!239 = metadata !{i32 165, i32 0, metadata !103, null}
!240 = metadata !{i32 191, i32 0, metadata !92, null}
!241 = metadata !{i32 193, i32 0, metadata !92, null}

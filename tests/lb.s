; ModuleID = 'lb.cl'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @timestep(i32 %nx, i32 %ny, float addrspace(1)* nocapture %cells_0, float addrspace(1)* nocapture %cells_in1, float addrspace(1)* nocapture %cells_in2, float addrspace(1)* nocapture %cells_in3, float addrspace(1)* nocapture %cells_in4, float addrspace(1)* nocapture %cells_out1, float addrspace(1)* nocapture %cells_out2, float addrspace(1)* nocapture %cells_out3, float addrspace(1)* nocapture %cells_out4, float addrspace(1)* nocapture %av_velocities, float addrspace(3)* nocapture %l_velocities) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %nx}, i64 0, metadata !14), !dbg !101
  tail call void @llvm.dbg.value(metadata !{i32 %ny}, i64 0, metadata !15), !dbg !101
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_0}, i64 0, metadata !16), !dbg !102
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_in1}, i64 0, metadata !17), !dbg !103
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_in2}, i64 0, metadata !18), !dbg !104
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_in3}, i64 0, metadata !19), !dbg !105
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_in4}, i64 0, metadata !20), !dbg !106
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_out1}, i64 0, metadata !21), !dbg !107
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_out2}, i64 0, metadata !22), !dbg !108
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_out3}, i64 0, metadata !23), !dbg !109
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %cells_out4}, i64 0, metadata !24), !dbg !110
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %av_velocities}, i64 0, metadata !25), !dbg !111
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %l_velocities}, i64 0, metadata !26), !dbg !112
  %call = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !113
  %conv = sext i32 %nx to i64, !dbg !113
  %rem = urem i64 %call, %conv, !dbg !113
  %conv1 = trunc i64 %rem to i32, !dbg !113
  tail call void @llvm.dbg.value(metadata !{i32 %conv1}, i64 0, metadata !27), !dbg !113
  %call2 = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !114
  %div = udiv i64 %call2, %conv, !dbg !114
  %conv4 = trunc i64 %div to i32, !dbg !114
  tail call void @llvm.dbg.value(metadata !{i32 %conv4}, i64 0, metadata !29), !dbg !114
  %sub = add nsw i32 %ny, -1, !dbg !115
  %call5 = tail call spir_func i32 @min(i32 %conv4, i32 %sub) nounwind, !dbg !115
  tail call void @llvm.dbg.value(metadata !{i32 %call5}, i64 0, metadata !29), !dbg !115
  tail call void @llvm.dbg.value(metadata !116, i64 0, metadata !30), !dbg !117
  %add = add nsw i32 %call5, 1, !dbg !118
  %rem6 = srem i32 %add, %ny, !dbg !118
  tail call void @llvm.dbg.value(metadata !{i32 %rem6}, i64 0, metadata !31), !dbg !118
  %add7 = add nsw i32 %conv1, 1, !dbg !119
  %rem8 = srem i32 %add7, %nx, !dbg !119
  tail call void @llvm.dbg.value(metadata !{i32 %rem8}, i64 0, metadata !33), !dbg !119
  %cmp = icmp eq i32 %call5, 0, !dbg !120
  %add10 = select i1 %cmp, i32 %ny, i32 0, !dbg !120
  %add10.call5 = add i32 %call5, -1, !dbg !120
  %sub11 = add i32 %add10.call5, %add10, !dbg !120
  tail call void @llvm.dbg.value(metadata !{i32 %sub11}, i64 0, metadata !34), !dbg !120
  %cmp13 = icmp eq i32 %conv1, 0, !dbg !121
  %add16 = select i1 %cmp13, i32 %nx, i32 0, !dbg !121
  tail call void @llvm.dbg.value(metadata !{i32 %sub17}, i64 0, metadata !35), !dbg !121
  %mul = mul nsw i32 %call5, %nx, !dbg !122
  %add22 = add nsw i32 %mul, %conv1, !dbg !122
  %idxprom = sext i32 %add22 to i64, !dbg !122
  %arrayidx = getelementptr inbounds float addrspace(1)* %cells_0, i64 %idxprom, !dbg !122
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !122, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %0}, i64 0, metadata !36), !dbg !122
  %add16.sink = add i32 %conv1, -1, !dbg !121
  %sub17 = add i32 %add16.sink, %add16, !dbg !121
  %add24 = add i32 %sub17, %mul, !dbg !126
  %idxprom25 = sext i32 %add24 to i64, !dbg !126
  %arrayidx26 = getelementptr inbounds float addrspace(1)* %cells_in1, i64 %idxprom25, !dbg !126
  %1 = load float addrspace(1)* %arrayidx26, align 4, !dbg !126, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %1}, i64 0, metadata !38), !dbg !126
  %mul27 = mul nsw i32 %sub11, %nx, !dbg !127
  %add28 = add nsw i32 %mul27, %conv1, !dbg !127
  %idxprom29 = sext i32 %add28 to i64, !dbg !127
  %arrayidx30 = getelementptr inbounds float addrspace(1)* %cells_in2, i64 %idxprom29, !dbg !127
  %2 = load float addrspace(1)* %arrayidx30, align 4, !dbg !127, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %2}, i64 0, metadata !39), !dbg !127
  %add32 = add nsw i32 %rem8, %mul, !dbg !128
  %idxprom33 = sext i32 %add32 to i64, !dbg !128
  %arrayidx34 = getelementptr inbounds float addrspace(1)* %cells_in3, i64 %idxprom33, !dbg !128
  %3 = load float addrspace(1)* %arrayidx34, align 4, !dbg !128, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %3}, i64 0, metadata !40), !dbg !128
  %mul35 = mul nsw i32 %rem6, %nx, !dbg !129
  %add36 = add nsw i32 %mul35, %conv1, !dbg !129
  %idxprom37 = sext i32 %add36 to i64, !dbg !129
  %arrayidx38 = getelementptr inbounds float addrspace(1)* %cells_in4, i64 %idxprom37, !dbg !129
  %4 = load float addrspace(1)* %arrayidx38, align 4, !dbg !129, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %4}, i64 0, metadata !41), !dbg !129
  %cmp39 = fcmp oge float %0, 0.000000e+00, !dbg !130
  br i1 %cmp39, label %land.rhs, label %land.end, !dbg !131

land.rhs:                                         ; preds = %entry
  %phitmp = select i1 %cmp13, float 0x3F25D867E0000000, float 0.000000e+00
  br label %land.end

land.end:                                         ; preds = %land.rhs, %entry
  %5 = phi float [ 0.000000e+00, %entry ], [ %phitmp, %land.rhs ]
  tail call void @llvm.dbg.value(metadata !{float %5}, i64 0, metadata !45), !dbg !131
  %add45 = fadd float %0, %1, !dbg !132
  %add46 = fadd float %add45, %2, !dbg !132
  %add47 = fadd float %add46, %3, !dbg !132
  %add48 = fadd float %add47, %4, !dbg !132
  tail call void @llvm.dbg.value(metadata !{float %add48}, i64 0, metadata !46), !dbg !132
  %sub49 = fsub float %1, %3, !dbg !133
  %div50 = fdiv float %sub49, %add48, !dbg !133, !fpmath !134
  tail call void @llvm.dbg.value(metadata !{float %div50}, i64 0, metadata !47), !dbg !133
  %sub51 = fsub float %2, %4, !dbg !135
  %div52 = fdiv float %sub51, %add48, !dbg !135, !fpmath !134
  tail call void @llvm.dbg.value(metadata !{float %div52}, i64 0, metadata !48), !dbg !135
  %mul53 = fmul float %div50, %div50, !dbg !136
  %mul54 = fmul float %div52, %div52, !dbg !136
  %add55 = fadd float %mul53, %mul54, !dbg !136
  tail call void @llvm.dbg.value(metadata !{float %add55}, i64 0, metadata !49), !dbg !136
  %mul57 = fmul float %add55, 1.500000e+00, !dbg !137
  tail call void @llvm.dbg.value(metadata !{float %mul59}, i64 0, metadata !50), !dbg !137
  br i1 %cmp39, label %cond.true61, label %cond.end66, !dbg !138

cond.true61:                                      ; preds = %land.end
  %sub58 = fsub float 1.000000e+00, %mul57, !dbg !137
  %mul56 = fmul float %add48, 0x3FD5555560000000, !dbg !137
  %mul59 = fmul float %mul56, %sub58, !dbg !137
  %sub62 = fsub float %mul59, %0, !dbg !138
  %mul63 = fmul float %sub62, 0x3FFD9999A0000000, !dbg !138
  %add64 = fadd float %0, %mul63, !dbg !138
  br label %cond.end66, !dbg !138

cond.end66:                                       ; preds = %land.end, %cond.true61
  %cond67 = phi float [ %add64, %cond.true61 ], [ -1.000000e+00, %land.end ], !dbg !138
  tail call void @llvm.dbg.value(metadata !{float %cond67}, i64 0, metadata !51), !dbg !138
  store float %cond67, float addrspace(1)* %arrayidx, align 4, !dbg !139, !tbaa !123
  %mul72 = fmul float %add48, 0x3FC5555560000000, !dbg !140
  tail call void @llvm.dbg.value(metadata !{float %mul72}, i64 0, metadata !52), !dbg !140
  %mul74 = fmul float %mul53, 0x4011FFFFE0000000, !dbg !141
  %sub76 = fsub float %mul74, %mul57, !dbg !141
  %mul77 = fmul float %mul72, %sub76, !dbg !141
  %add78 = fadd float %mul72, %mul77, !dbg !141
  tail call void @llvm.dbg.value(metadata !{float %add78}, i64 0, metadata !53), !dbg !141
  %mul79 = fmul float %mul72, %div50, !dbg !142
  tail call void @llvm.dbg.value(metadata !{float %add81}, i64 0, metadata !54), !dbg !142
  br i1 %cmp39, label %cond.true99, label %cond.end89.thread297, !dbg !143

cond.end89.thread297:                             ; preds = %cond.end66
  tail call void @llvm.dbg.value(metadata !{float %cond90295}, i64 0, metadata !55), !dbg !143
  %arrayidx94298 = getelementptr inbounds float addrspace(1)* %cells_out1, i64 %idxprom, !dbg !144
  store float %3, float addrspace(1)* %arrayidx94298, align 4, !dbg !144, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %add97}, i64 0, metadata !56), !dbg !145
  br label %cond.end105, !dbg !146

cond.true99:                                      ; preds = %cond.end66
  %mul80 = fmul float %mul79, 3.000000e+00, !dbg !142
  %add81 = fadd float %mul80, %add78, !dbg !142
  %sub84 = fsub float %add81, %1, !dbg !143
  %mul85 = fmul float %sub84, 0x3FFD9999A0000000, !dbg !143
  %add86 = fadd float %1, %mul85, !dbg !143
  %add87 = fadd float %5, %add86, !dbg !143
  tail call void @llvm.dbg.value(metadata !{float %cond90295}, i64 0, metadata !55), !dbg !143
  %arrayidx94294 = getelementptr inbounds float addrspace(1)* %cells_out1, i64 %idxprom, !dbg !144
  store float %add87, float addrspace(1)* %arrayidx94294, align 4, !dbg !144, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %add97}, i64 0, metadata !56), !dbg !145
  %mul96 = fmul float %mul79, -3.000000e+00, !dbg !145
  %add97 = fadd float %mul96, %add78, !dbg !145
  %sub100 = fsub float %add97, %3, !dbg !146
  %mul101 = fmul float %sub100, 0x3FFD9999A0000000, !dbg !146
  %add102 = fadd float %3, %mul101, !dbg !146
  %sub103 = fsub float %add102, %5, !dbg !146
  br label %cond.end105, !dbg !146

cond.end105:                                      ; preds = %cond.end89.thread297, %cond.true99
  %cond90295 = phi float [ %add87, %cond.true99 ], [ %3, %cond.end89.thread297 ]
  %cond106 = phi float [ %sub103, %cond.true99 ], [ %1, %cond.end89.thread297 ], !dbg !146
  tail call void @llvm.dbg.value(metadata !{float %cond106}, i64 0, metadata !57), !dbg !146
  %arrayidx110 = getelementptr inbounds float addrspace(1)* %cells_out3, i64 %idxprom, !dbg !147
  store float %cond106, float addrspace(1)* %arrayidx110, align 4, !dbg !147, !tbaa !123
  %mul112 = fmul float %mul54, 0x4011FFFFE0000000, !dbg !148
  %sub114 = fsub float %mul112, %mul57, !dbg !148
  %mul115 = fmul float %mul72, %sub114, !dbg !148
  %add116 = fadd float %mul72, %mul115, !dbg !148
  tail call void @llvm.dbg.value(metadata !{float %add116}, i64 0, metadata !58), !dbg !148
  %mul117 = fmul float %mul72, %div52, !dbg !149
  tail call void @llvm.dbg.value(metadata !{float %add119}, i64 0, metadata !59), !dbg !149
  br i1 %cmp39, label %cond.end141.thread, label %cond.end141, !dbg !150

cond.end141.thread:                               ; preds = %cond.end105
  %mul118 = fmul float %mul117, 3.000000e+00, !dbg !149
  %add119 = fadd float %mul118, %add116, !dbg !149
  %sub122 = fsub float %add119, %2, !dbg !150
  %mul123 = fmul float %sub122, 0x3FFD9999A0000000, !dbg !150
  %add124 = fadd float %2, %mul123, !dbg !150
  tail call void @llvm.dbg.value(metadata !{float %4}, i64 0, metadata !60), !dbg !150
  %arrayidx131299 = getelementptr inbounds float addrspace(1)* %cells_out2, i64 %idxprom, !dbg !151
  store float %add124, float addrspace(1)* %arrayidx131299, align 4, !dbg !151, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %add134}, i64 0, metadata !61), !dbg !152
  %mul133 = fmul float %mul117, -3.000000e+00, !dbg !152
  %add134 = fadd float %mul133, %add116, !dbg !152
  %sub137 = fsub float %add134, %4, !dbg !153
  %mul138 = fmul float %sub137, 0x3FFD9999A0000000, !dbg !153
  %add139 = fadd float %4, %mul138, !dbg !153
  tail call void @llvm.dbg.value(metadata !{float %2}, i64 0, metadata !62), !dbg !153
  %arrayidx146304 = getelementptr inbounds float addrspace(1)* %cells_out4, i64 %idxprom, !dbg !154
  store float %add139, float addrspace(1)* %arrayidx146304, align 4, !dbg !154, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %cond67}, i64 0, metadata !46), !dbg !155
  %add147305 = fadd float %cond67, %cond90295, !dbg !156
  tail call void @llvm.dbg.value(metadata !{float %add147}, i64 0, metadata !46), !dbg !156
  %add148306 = fadd float %add124, %add147305, !dbg !157
  tail call void @llvm.dbg.value(metadata !{float %add148}, i64 0, metadata !46), !dbg !157
  %add149307 = fadd float %cond106, %add148306, !dbg !158
  tail call void @llvm.dbg.value(metadata !{float %add149}, i64 0, metadata !46), !dbg !158
  %add150308 = fadd float %add139, %add149307, !dbg !159
  tail call void @llvm.dbg.value(metadata !{float %add150309}, i64 0, metadata !46), !dbg !159
  %sub153 = fsub float %cond90295, %cond106, !dbg !160
  br label %cond.end155, !dbg !160

cond.end141:                                      ; preds = %cond.end105
  tail call void @llvm.dbg.value(metadata !{float %4}, i64 0, metadata !60), !dbg !150
  %arrayidx131303 = getelementptr inbounds float addrspace(1)* %cells_out2, i64 %idxprom, !dbg !151
  store float %4, float addrspace(1)* %arrayidx131303, align 4, !dbg !151, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %add134}, i64 0, metadata !61), !dbg !152
  tail call void @llvm.dbg.value(metadata !{float %2}, i64 0, metadata !62), !dbg !153
  %arrayidx146 = getelementptr inbounds float addrspace(1)* %cells_out4, i64 %idxprom, !dbg !154
  store float %2, float addrspace(1)* %arrayidx146, align 4, !dbg !154, !tbaa !123
  tail call void @llvm.dbg.value(metadata !{float %cond67}, i64 0, metadata !46), !dbg !155
  %add147 = fadd float %cond67, %cond90295, !dbg !156
  tail call void @llvm.dbg.value(metadata !{float %add147}, i64 0, metadata !46), !dbg !156
  %add148 = fadd float %4, %add147, !dbg !157
  tail call void @llvm.dbg.value(metadata !{float %add148}, i64 0, metadata !46), !dbg !157
  %add149 = fadd float %cond106, %add148, !dbg !158
  tail call void @llvm.dbg.value(metadata !{float %add149}, i64 0, metadata !46), !dbg !158
  %add150 = fadd float %2, %add149, !dbg !159
  tail call void @llvm.dbg.value(metadata !{float %add150309}, i64 0, metadata !46), !dbg !159
  br label %cond.end155, !dbg !160

cond.end155:                                      ; preds = %cond.end141, %cond.end141.thread
  %add150309 = phi float [ %add150308, %cond.end141.thread ], [ %add150, %cond.end141 ]
  %cond156 = phi float [ %sub153, %cond.end141.thread ], [ 0.000000e+00, %cond.end141 ], !dbg !160
  %div157 = fdiv float %cond156, %add150309, !dbg !160, !fpmath !134
  tail call void @llvm.dbg.value(metadata !{float %div157}, i64 0, metadata !30), !dbg !160
  %call158 = tail call spir_func i64 @get_local_size(i32 0) nounwind, !dbg !161
  %conv159 = trunc i64 %call158 to i32, !dbg !161
  tail call void @llvm.dbg.value(metadata !{i32 %conv159}, i64 0, metadata !63), !dbg !161
  %call160 = tail call spir_func i64 @get_local_id(i32 0) nounwind, !dbg !162
  %conv161 = trunc i64 %call160 to i32, !dbg !162
  tail call void @llvm.dbg.value(metadata !{i32 %conv161}, i64 0, metadata !64), !dbg !162
  %idxprom162 = sext i32 %conv161 to i64, !dbg !163
  %arrayidx163 = getelementptr inbounds float addrspace(3)* %l_velocities, i64 %idxprom162, !dbg !163
  store float %div157, float addrspace(3)* %arrayidx163, align 4, !dbg !163, !tbaa !123
  %offset.0311 = ashr i32 %conv159, 1, !dbg !164
  %cmp164312 = icmp sgt i32 %offset.0311, 0, !dbg !164
  br i1 %cmp164312, label %for.body, label %for.end, !dbg !164

for.body:                                         ; preds = %cond.end155, %for.cond.backedge
  %offset.0313 = phi i32 [ %offset.0, %for.cond.backedge ], [ %offset.0311, %cond.end155 ]
  tail call spir_func void @barrier(i32 1) nounwind, !dbg !165
  %cmp166 = icmp slt i32 %conv161, %offset.0313, !dbg !167
  br i1 %cmp166, label %if.then, label %for.cond.backedge, !dbg !167

for.cond.backedge:                                ; preds = %for.body, %if.then
  %offset.0 = ashr i32 %offset.0313, 1, !dbg !164
  %cmp164 = icmp sgt i32 %offset.0, 0, !dbg !164
  br i1 %cmp164, label %for.body, label %for.end, !dbg !164

if.then:                                          ; preds = %for.body
  %add168 = add nsw i32 %offset.0313, %conv161, !dbg !168
  %idxprom169 = sext i32 %add168 to i64, !dbg !168
  %arrayidx170 = getelementptr inbounds float addrspace(3)* %l_velocities, i64 %idxprom169, !dbg !168
  %6 = load float addrspace(3)* %arrayidx170, align 4, !dbg !168, !tbaa !123
  %7 = load float addrspace(3)* %arrayidx163, align 4, !dbg !168, !tbaa !123
  %add173 = fadd float %6, %7, !dbg !168
  store float %add173, float addrspace(3)* %arrayidx163, align 4, !dbg !168, !tbaa !123
  br label %for.cond.backedge, !dbg !170

for.end:                                          ; preds = %for.cond.backedge, %cond.end155
  %cmp175 = icmp eq i32 %conv161, 0, !dbg !171
  br i1 %cmp175, label %if.then177, label %if.end183, !dbg !171

if.then177:                                       ; preds = %for.end
  %call178 = tail call spir_func i64 @get_group_id(i32 0) nounwind, !dbg !172
  %8 = load float addrspace(3)* %l_velocities, align 4, !dbg !173, !tbaa !123
  %sext = shl i64 %call178, 32, !dbg !173
  %idxprom181 = ashr exact i64 %sext, 32, !dbg !173
  %arrayidx182 = getelementptr inbounds float addrspace(1)* %av_velocities, i64 %idxprom181, !dbg !173
  store float %8, float addrspace(1)* %arrayidx182, align 4, !dbg !173, !tbaa !123
  br label %if.end183, !dbg !174

if.end183:                                        ; preds = %if.then177, %for.end
  ret void, !dbg !175
}

declare spir_func i64 @get_global_id(i32)

declare spir_func i32 @min(i32, i32)

declare spir_func i64 @get_local_size(i32)

declare spir_func i64 @get_local_id(i32)

declare spir_func void @barrier(i32)

declare spir_func i64 @get_group_id(i32)

define spir_kernel void @reduce(i32 %n, i32 %step, i32 %ncells, float addrspace(1)* nocapture %av_velocities, float addrspace(1)* nocapture %velocities, float addrspace(3)* nocapture %l_velocities) nounwind {
entry:
  tail call void @llvm.dbg.value(metadata !{i32 %n}, i64 0, metadata !74), !dbg !176
  tail call void @llvm.dbg.value(metadata !{i32 %step}, i64 0, metadata !75), !dbg !176
  tail call void @llvm.dbg.value(metadata !{i32 %ncells}, i64 0, metadata !76), !dbg !176
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %av_velocities}, i64 0, metadata !77), !dbg !177
  tail call void @llvm.dbg.value(metadata !{float addrspace(1)* %velocities}, i64 0, metadata !78), !dbg !178
  tail call void @llvm.dbg.value(metadata !{float addrspace(3)* %l_velocities}, i64 0, metadata !79), !dbg !179
  %call = tail call spir_func i64 @get_global_id(i32 0) nounwind, !dbg !180
  %conv = trunc i64 %call to i32, !dbg !180
  tail call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !80), !dbg !180
  %call1 = tail call spir_func i64 @get_local_id(i32 0) nounwind, !dbg !181
  %conv2 = trunc i64 %call1 to i32, !dbg !181
  tail call void @llvm.dbg.value(metadata !{i32 %conv2}, i64 0, metadata !82), !dbg !181
  %call3 = tail call spir_func i64 @get_local_size(i32 0) nounwind, !dbg !182
  %conv4 = trunc i64 %call3 to i32, !dbg !182
  tail call void @llvm.dbg.value(metadata !{i32 %conv4}, i64 0, metadata !83), !dbg !182
  tail call void @llvm.dbg.value(metadata !116, i64 0, metadata !84), !dbg !183
  tail call void @llvm.dbg.value(metadata !{i32 %conv}, i64 0, metadata !85), !dbg !184
  %cmp49 = icmp slt i32 %conv, %n, !dbg !184
  br i1 %cmp49, label %for.body, label %for.end, !dbg !184

for.body:                                         ; preds = %entry, %for.body
  %j.051 = phi i32 [ %add6, %for.body ], [ %conv, %entry ]
  %velocity.050 = phi float [ %add, %for.body ], [ 0.000000e+00, %entry ]
  %idxprom = sext i32 %j.051 to i64, !dbg !185
  %arrayidx = getelementptr inbounds float addrspace(1)* %velocities, i64 %idxprom, !dbg !185
  %0 = load float addrspace(1)* %arrayidx, align 4, !dbg !185, !tbaa !123
  %add = fadd float %velocity.050, %0, !dbg !185
  tail call void @llvm.dbg.value(metadata !{float %add}, i64 0, metadata !84), !dbg !185
  %add6 = add nsw i32 %j.051, %conv4, !dbg !184
  tail call void @llvm.dbg.value(metadata !{i32 %add6}, i64 0, metadata !85), !dbg !184
  %cmp = icmp slt i32 %add6, %n, !dbg !184
  br i1 %cmp, label %for.body, label %for.end, !dbg !184

for.end:                                          ; preds = %for.body, %entry
  %velocity.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %add, %for.body ]
  %idxprom7 = sext i32 %conv2 to i64, !dbg !187
  %arrayidx8 = getelementptr inbounds float addrspace(3)* %l_velocities, i64 %idxprom7, !dbg !187
  store float %velocity.0.lcssa, float addrspace(3)* %arrayidx8, align 4, !dbg !187, !tbaa !123
  %offset.046 = ashr i32 %conv4, 1, !dbg !188
  %cmp1047 = icmp sgt i32 %offset.046, 0, !dbg !188
  br i1 %cmp1047, label %for.body12, label %for.end23, !dbg !188

for.body12:                                       ; preds = %for.end, %for.cond9.backedge
  %offset.048 = phi i32 [ %offset.0, %for.cond9.backedge ], [ %offset.046, %for.end ]
  tail call spir_func void @barrier(i32 1) nounwind, !dbg !189
  %cmp13 = icmp slt i32 %conv2, %offset.048, !dbg !191
  br i1 %cmp13, label %if.then, label %for.cond9.backedge, !dbg !191

for.cond9.backedge:                               ; preds = %for.body12, %if.then
  %offset.0 = ashr i32 %offset.048, 1, !dbg !188
  %cmp10 = icmp sgt i32 %offset.0, 0, !dbg !188
  br i1 %cmp10, label %for.body12, label %for.end23, !dbg !188

if.then:                                          ; preds = %for.body12
  %add15 = add nsw i32 %offset.048, %conv2, !dbg !192
  %idxprom16 = sext i32 %add15 to i64, !dbg !192
  %arrayidx17 = getelementptr inbounds float addrspace(3)* %l_velocities, i64 %idxprom16, !dbg !192
  %1 = load float addrspace(3)* %arrayidx17, align 4, !dbg !192, !tbaa !123
  %2 = load float addrspace(3)* %arrayidx8, align 4, !dbg !192, !tbaa !123
  %add20 = fadd float %1, %2, !dbg !192
  store float %add20, float addrspace(3)* %arrayidx8, align 4, !dbg !192, !tbaa !123
  br label %for.cond9.backedge, !dbg !194

for.end23:                                        ; preds = %for.cond9.backedge, %for.end
  %cmp24 = icmp eq i32 %conv2, 0, !dbg !195
  br i1 %cmp24, label %if.then26, label %if.end31, !dbg !195

if.then26:                                        ; preds = %for.end23
  %3 = load float addrspace(3)* %l_velocities, align 4, !dbg !196, !tbaa !123
  %conv28 = sitofp i32 %ncells to float, !dbg !196
  %div = fdiv float %3, %conv28, !dbg !196, !fpmath !134
  %idxprom29 = sext i32 %step to i64, !dbg !196
  %arrayidx30 = getelementptr inbounds float addrspace(1)* %av_velocities, i64 %idxprom29, !dbg !196
  store float %div, float addrspace(1)* %arrayidx30, align 4, !dbg !196, !tbaa !123
  br label %if.end31, !dbg !198

if.end31:                                         ; preds = %if.then26, %for.end23
  ret void, !dbg !199
}

declare void @llvm.dbg.value(metadata, i64, metadata) nounwind readnone

!llvm.dbg.cu = !{!0}
!opencl.kernels = !{!89, !95}
!opencl.enable.FP_CONTRACT = !{}

!0 = metadata !{i32 786449, i32 0, i32 12, metadata !"<unknown>", metadata !"/Users/james/projects/oclgrind/tests", metadata !"clang version 3.2 (tags/RELEASE_32/final 183304)", i1 true, i1 true, metadata !"", i32 0, metadata !1, metadata !1, metadata !3, metadata !1} ; [ DW_TAG_compile_unit ] [/Users/james/projects/oclgrind/tests/<unknown>] [DW_LANG_C99]
!1 = metadata !{metadata !2}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4}
!4 = metadata !{metadata !5, metadata !69}
!5 = metadata !{i32 786478, i32 0, metadata !6, metadata !"timestep", metadata !"timestep", metadata !"", metadata !6, i32 19, metadata !7, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @timestep, null, null, metadata !12, i32 31} ; [ DW_TAG_subprogram ] [line 19] [def] [scope 31] [timestep]
!6 = metadata !{i32 786473, metadata !"lb.cl", metadata !"/Users/james/projects/oclgrind/tests", null} ; [ DW_TAG_file_type ]
!7 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !8, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!8 = metadata !{null, metadata !9, metadata !9, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10, metadata !10}
!9 = metadata !{i32 786468, null, metadata !"int", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!10 = metadata !{i32 786447, null, metadata !"", null, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !11} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from float]
!11 = metadata !{i32 786468, null, metadata !"float", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [float] [line 0, size 32, align 32, offset 0, enc DW_ATE_float]
!12 = metadata !{metadata !13}
!13 = metadata !{metadata !14, metadata !15, metadata !16, metadata !17, metadata !18, metadata !19, metadata !20, metadata !21, metadata !22, metadata !23, metadata !24, metadata !25, metadata !26, metadata !27, metadata !29, metadata !30, metadata !31, metadata !33, metadata !34, metadata !35, metadata !36, metadata !38, metadata !39, metadata !40, metadata !41, metadata !42, metadata !45, metadata !46, metadata !47, metadata !48, metadata !49, metadata !50, metadata !51, metadata !52, metadata !53, metadata !54, metadata !55, metadata !56, metadata !57, metadata !58, metadata !59, metadata !60, metadata !61, metadata !62, metadata !63, metadata !64, metadata !65, metadata !67}
!14 = metadata !{i32 786689, metadata !5, metadata !"nx", metadata !6, i32 16777235, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [nx] [line 19]
!15 = metadata !{i32 786689, metadata !5, metadata !"ny", metadata !6, i32 33554451, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [ny] [line 19]
!16 = metadata !{i32 786689, metadata !5, metadata !"cells_0", metadata !6, i32 50331668, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_0] [line 20]
!17 = metadata !{i32 786689, metadata !5, metadata !"cells_in1", metadata !6, i32 67108885, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_in1] [line 21]
!18 = metadata !{i32 786689, metadata !5, metadata !"cells_in2", metadata !6, i32 83886102, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_in2] [line 22]
!19 = metadata !{i32 786689, metadata !5, metadata !"cells_in3", metadata !6, i32 100663319, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_in3] [line 23]
!20 = metadata !{i32 786689, metadata !5, metadata !"cells_in4", metadata !6, i32 117440536, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_in4] [line 24]
!21 = metadata !{i32 786689, metadata !5, metadata !"cells_out1", metadata !6, i32 134217753, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_out1] [line 25]
!22 = metadata !{i32 786689, metadata !5, metadata !"cells_out2", metadata !6, i32 150994970, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_out2] [line 26]
!23 = metadata !{i32 786689, metadata !5, metadata !"cells_out3", metadata !6, i32 167772187, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_out3] [line 27]
!24 = metadata !{i32 786689, metadata !5, metadata !"cells_out4", metadata !6, i32 184549404, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [cells_out4] [line 28]
!25 = metadata !{i32 786689, metadata !5, metadata !"av_velocities", metadata !6, i32 201326621, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [av_velocities] [line 29]
!26 = metadata !{i32 786689, metadata !5, metadata !"l_velocities", metadata !6, i32 218103838, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [l_velocities] [line 30]
!27 = metadata !{i32 786688, metadata !28, metadata !"x", metadata !6, i32 32, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [x] [line 32]
!28 = metadata !{i32 786443, metadata !5, i32 31, i32 0, metadata !6, i32 0} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!29 = metadata !{i32 786688, metadata !28, metadata !"y", metadata !6, i32 33, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [y] [line 33]
!30 = metadata !{i32 786688, metadata !28, metadata !"av_velocity", metadata !6, i32 39, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [av_velocity] [line 39]
!31 = metadata !{i32 786688, metadata !28, metadata !"y_n", metadata !6, i32 42, metadata !32, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [y_n] [line 42]
!32 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !9} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from int]
!33 = metadata !{i32 786688, metadata !28, metadata !"x_e", metadata !6, i32 43, metadata !32, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [x_e] [line 43]
!34 = metadata !{i32 786688, metadata !28, metadata !"y_s", metadata !6, i32 44, metadata !32, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [y_s] [line 44]
!35 = metadata !{i32 786688, metadata !28, metadata !"x_w", metadata !6, i32 45, metadata !32, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [x_w] [line 45]
!36 = metadata !{i32 786688, metadata !28, metadata !"speed0", metadata !6, i32 46, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [speed0] [line 46]
!37 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !11} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from float]
!38 = metadata !{i32 786688, metadata !28, metadata !"speed1", metadata !6, i32 47, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [speed1] [line 47]
!39 = metadata !{i32 786688, metadata !28, metadata !"speed2", metadata !6, i32 48, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [speed2] [line 48]
!40 = metadata !{i32 786688, metadata !28, metadata !"speed3", metadata !6, i32 49, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [speed3] [line 49]
!41 = metadata !{i32 786688, metadata !28, metadata !"speed4", metadata !6, i32 50, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [speed4] [line 50]
!42 = metadata !{i32 786688, metadata !28, metadata !"collide", metadata !6, i32 52, metadata !43, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [collide] [line 52]
!43 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !44} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from bool]
!44 = metadata !{i32 786468, null, metadata !"bool", null, i32 0, i64 8, i64 8, i64 0, i32 0, i32 2} ; [ DW_TAG_base_type ] [bool] [line 0, size 8, align 8, offset 0, enc DW_ATE_boolean]
!45 = metadata !{i32 786688, metadata !28, metadata !"accel", metadata !6, i32 53, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [accel] [line 53]
!46 = metadata !{i32 786688, metadata !28, metadata !"local_density", metadata !6, i32 56, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [local_density] [line 56]
!47 = metadata !{i32 786688, metadata !28, metadata !"u_x", metadata !6, i32 59, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [u_x] [line 59]
!48 = metadata !{i32 786688, metadata !28, metadata !"u_y", metadata !6, i32 60, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [u_y] [line 60]
!49 = metadata !{i32 786688, metadata !28, metadata !"u_sq", metadata !6, i32 61, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [u_sq] [line 61]
!50 = metadata !{i32 786688, metadata !28, metadata !"d_equ", metadata !6, i32 65, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [d_equ] [line 65]
!51 = metadata !{i32 786688, metadata !28, metadata !"_speed0", metadata !6, i32 66, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [_speed0] [line 66]
!52 = metadata !{i32 786688, metadata !28, metadata !"w1ld", metadata !6, i32 69, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [w1ld] [line 69]
!53 = metadata !{i32 786688, metadata !28, metadata !"xterm", metadata !6, i32 70, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [xterm] [line 70]
!54 = metadata !{i32 786688, metadata !28, metadata !"d_equ1", metadata !6, i32 72, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [d_equ1] [line 72]
!55 = metadata !{i32 786688, metadata !28, metadata !"_speed1", metadata !6, i32 73, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [_speed1] [line 73]
!56 = metadata !{i32 786688, metadata !28, metadata !"d_equ3", metadata !6, i32 78, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [d_equ3] [line 78]
!57 = metadata !{i32 786688, metadata !28, metadata !"_speed3", metadata !6, i32 79, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [_speed3] [line 79]
!58 = metadata !{i32 786688, metadata !28, metadata !"yterm", metadata !6, i32 84, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [yterm] [line 84]
!59 = metadata !{i32 786688, metadata !28, metadata !"d_equ2", metadata !6, i32 86, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [d_equ2] [line 86]
!60 = metadata !{i32 786688, metadata !28, metadata !"_speed2", metadata !6, i32 87, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [_speed2] [line 87]
!61 = metadata !{i32 786688, metadata !28, metadata !"d_equ4", metadata !6, i32 92, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [d_equ4] [line 92]
!62 = metadata !{i32 786688, metadata !28, metadata !"_speed4", metadata !6, i32 93, metadata !37, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [_speed4] [line 93]
!63 = metadata !{i32 786688, metadata !28, metadata !"l_size", metadata !6, i32 106, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [l_size] [line 106]
!64 = metadata !{i32 786688, metadata !28, metadata !"l", metadata !6, i32 107, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [l] [line 107]
!65 = metadata !{i32 786688, metadata !66, metadata !"offset", metadata !6, i32 109, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [offset] [line 109]
!66 = metadata !{i32 786443, metadata !28, i32 109, i32 0, metadata !6, i32 1} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!67 = metadata !{i32 786688, metadata !68, metadata !"group", metadata !6, i32 119, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [group] [line 119]
!68 = metadata !{i32 786443, metadata !28, i32 118, i32 0, metadata !6, i32 4} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!69 = metadata !{i32 786478, i32 0, metadata !6, metadata !"reduce", metadata !"reduce", metadata !"", metadata !6, i32 124, metadata !70, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @reduce, null, null, metadata !72, i32 128} ; [ DW_TAG_subprogram ] [line 124] [def] [scope 128] [reduce]
!70 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !71, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!71 = metadata !{null, metadata !9, metadata !9, metadata !9, metadata !10, metadata !10, metadata !10}
!72 = metadata !{metadata !73}
!73 = metadata !{metadata !74, metadata !75, metadata !76, metadata !77, metadata !78, metadata !79, metadata !80, metadata !82, metadata !83, metadata !84, metadata !85, metadata !87}
!74 = metadata !{i32 786689, metadata !69, metadata !"n", metadata !6, i32 16777340, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [n] [line 124]
!75 = metadata !{i32 786689, metadata !69, metadata !"step", metadata !6, i32 33554556, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [step] [line 124]
!76 = metadata !{i32 786689, metadata !69, metadata !"ncells", metadata !6, i32 50331772, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [ncells] [line 124]
!77 = metadata !{i32 786689, metadata !69, metadata !"av_velocities", metadata !6, i32 67108989, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [av_velocities] [line 125]
!78 = metadata !{i32 786689, metadata !69, metadata !"velocities", metadata !6, i32 83886206, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [velocities] [line 126]
!79 = metadata !{i32 786689, metadata !69, metadata !"l_velocities", metadata !6, i32 100663423, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [l_velocities] [line 127]
!80 = metadata !{i32 786688, metadata !81, metadata !"i", metadata !6, i32 129, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 129]
!81 = metadata !{i32 786443, metadata !69, i32 128, i32 0, metadata !6, i32 5} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!82 = metadata !{i32 786688, metadata !81, metadata !"l", metadata !6, i32 130, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [l] [line 130]
!83 = metadata !{i32 786688, metadata !81, metadata !"l_size", metadata !6, i32 131, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [l_size] [line 131]
!84 = metadata !{i32 786688, metadata !81, metadata !"velocity", metadata !6, i32 134, metadata !11, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [velocity] [line 134]
!85 = metadata !{i32 786688, metadata !86, metadata !"j", metadata !6, i32 135, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [j] [line 135]
!86 = metadata !{i32 786443, metadata !81, i32 135, i32 0, metadata !6, i32 6} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!87 = metadata !{i32 786688, metadata !88, metadata !"offset", metadata !6, i32 142, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [offset] [line 142]
!88 = metadata !{i32 786443, metadata !81, i32 142, i32 0, metadata !6, i32 8} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!89 = metadata !{void (i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @timestep, metadata !90, metadata !91, metadata !92, metadata !93, metadata !94}
!90 = metadata !{metadata !"kernel_arg_addr_space", i32 0, i32 0, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 3}
!91 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none"}
!92 = metadata !{metadata !"kernel_arg_type", metadata !"int", metadata !"int", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*", metadata !"float*"}
!93 = metadata !{metadata !"kernel_arg_type_qual", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !""}
!94 = metadata !{metadata !"kernel_arg_name", metadata !"nx", metadata !"ny", metadata !"cells_0", metadata !"cells_in1", metadata !"cells_in2", metadata !"cells_in3", metadata !"cells_in4", metadata !"cells_out1", metadata !"cells_out2", metadata !"cells_out3", metadata !"cells_out4", metadata !"av_velocities", metadata !"l_velocities"}
!95 = metadata !{void (i32, i32, i32, float addrspace(1)*, float addrspace(1)*, float addrspace(3)*)* @reduce, metadata !96, metadata !97, metadata !98, metadata !99, metadata !100}
!96 = metadata !{metadata !"kernel_arg_addr_space", i32 0, i32 0, i32 0, i32 1, i32 1, i32 3}
!97 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none", metadata !"none"}
!98 = metadata !{metadata !"kernel_arg_type", metadata !"int", metadata !"int", metadata !"int", metadata !"float*", metadata !"float*", metadata !"float*"}
!99 = metadata !{metadata !"kernel_arg_type_qual", metadata !"", metadata !"", metadata !"", metadata !"", metadata !"", metadata !""}
!100 = metadata !{metadata !"kernel_arg_name", metadata !"n", metadata !"step", metadata !"ncells", metadata !"av_velocities", metadata !"velocities", metadata !"l_velocities"}
!101 = metadata !{i32 19, i32 0, metadata !5, null}
!102 = metadata !{i32 20, i32 0, metadata !5, null}
!103 = metadata !{i32 21, i32 0, metadata !5, null}
!104 = metadata !{i32 22, i32 0, metadata !5, null}
!105 = metadata !{i32 23, i32 0, metadata !5, null}
!106 = metadata !{i32 24, i32 0, metadata !5, null}
!107 = metadata !{i32 25, i32 0, metadata !5, null}
!108 = metadata !{i32 26, i32 0, metadata !5, null}
!109 = metadata !{i32 27, i32 0, metadata !5, null}
!110 = metadata !{i32 28, i32 0, metadata !5, null}
!111 = metadata !{i32 29, i32 0, metadata !5, null}
!112 = metadata !{i32 30, i32 0, metadata !5, null}
!113 = metadata !{i32 32, i32 0, metadata !28, null}
!114 = metadata !{i32 33, i32 0, metadata !28, null}
!115 = metadata !{i32 37, i32 0, metadata !28, null}
!116 = metadata !{float 0.000000e+00}
!117 = metadata !{i32 39, i32 0, metadata !28, null}
!118 = metadata !{i32 42, i32 0, metadata !28, null}
!119 = metadata !{i32 43, i32 0, metadata !28, null}
!120 = metadata !{i32 44, i32 0, metadata !28, null}
!121 = metadata !{i32 45, i32 0, metadata !28, null}
!122 = metadata !{i32 46, i32 0, metadata !28, null}
!123 = metadata !{metadata !"float", metadata !124}
!124 = metadata !{metadata !"omnipotent char", metadata !125}
!125 = metadata !{metadata !"Simple C/C++ TBAA"}
!126 = metadata !{i32 47, i32 0, metadata !28, null}
!127 = metadata !{i32 48, i32 0, metadata !28, null}
!128 = metadata !{i32 49, i32 0, metadata !28, null}
!129 = metadata !{i32 50, i32 0, metadata !28, null}
!130 = metadata !{i32 52, i32 0, metadata !28, null}
!131 = metadata !{i32 53, i32 0, metadata !28, null}
!132 = metadata !{i32 56, i32 0, metadata !28, null}
!133 = metadata !{i32 59, i32 0, metadata !28, null}
!134 = metadata !{float 2.500000e+00}
!135 = metadata !{i32 60, i32 0, metadata !28, null}
!136 = metadata !{i32 61, i32 0, metadata !28, null}
!137 = metadata !{i32 65, i32 0, metadata !28, null}
!138 = metadata !{i32 66, i32 0, metadata !28, null}
!139 = metadata !{i32 67, i32 0, metadata !28, null}
!140 = metadata !{i32 69, i32 0, metadata !28, null}
!141 = metadata !{i32 70, i32 0, metadata !28, null}
!142 = metadata !{i32 72, i32 0, metadata !28, null}
!143 = metadata !{i32 75, i32 0, metadata !28, null}
!144 = metadata !{i32 76, i32 0, metadata !28, null}
!145 = metadata !{i32 78, i32 0, metadata !28, null}
!146 = metadata !{i32 81, i32 0, metadata !28, null}
!147 = metadata !{i32 82, i32 0, metadata !28, null}
!148 = metadata !{i32 84, i32 0, metadata !28, null}
!149 = metadata !{i32 86, i32 0, metadata !28, null}
!150 = metadata !{i32 89, i32 0, metadata !28, null}
!151 = metadata !{i32 90, i32 0, metadata !28, null}
!152 = metadata !{i32 92, i32 0, metadata !28, null}
!153 = metadata !{i32 95, i32 0, metadata !28, null}
!154 = metadata !{i32 96, i32 0, metadata !28, null}
!155 = metadata !{i32 98, i32 0, metadata !28, null}
!156 = metadata !{i32 99, i32 0, metadata !28, null}
!157 = metadata !{i32 100, i32 0, metadata !28, null}
!158 = metadata !{i32 101, i32 0, metadata !28, null}
!159 = metadata !{i32 102, i32 0, metadata !28, null}
!160 = metadata !{i32 103, i32 0, metadata !28, null}
!161 = metadata !{i32 106, i32 0, metadata !28, null}
!162 = metadata !{i32 107, i32 0, metadata !28, null}
!163 = metadata !{i32 108, i32 0, metadata !28, null}
!164 = metadata !{i32 109, i32 0, metadata !66, null}
!165 = metadata !{i32 111, i32 0, metadata !166, null}
!166 = metadata !{i32 786443, metadata !66, i32 110, i32 0, metadata !6, i32 2} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!167 = metadata !{i32 112, i32 0, metadata !166, null}
!168 = metadata !{i32 114, i32 0, metadata !169, null}
!169 = metadata !{i32 786443, metadata !166, i32 113, i32 0, metadata !6, i32 3} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!170 = metadata !{i32 115, i32 0, metadata !169, null}
!171 = metadata !{i32 117, i32 0, metadata !28, null}
!172 = metadata !{i32 119, i32 0, metadata !68, null}
!173 = metadata !{i32 120, i32 0, metadata !68, null}
!174 = metadata !{i32 121, i32 0, metadata !68, null}
!175 = metadata !{i32 122, i32 0, metadata !28, null}
!176 = metadata !{i32 124, i32 0, metadata !69, null}
!177 = metadata !{i32 125, i32 0, metadata !69, null}
!178 = metadata !{i32 126, i32 0, metadata !69, null}
!179 = metadata !{i32 127, i32 0, metadata !69, null}
!180 = metadata !{i32 129, i32 0, metadata !81, null}
!181 = metadata !{i32 130, i32 0, metadata !81, null}
!182 = metadata !{i32 131, i32 0, metadata !81, null}
!183 = metadata !{i32 134, i32 0, metadata !81, null}
!184 = metadata !{i32 135, i32 0, metadata !86, null}
!185 = metadata !{i32 137, i32 0, metadata !186, null}
!186 = metadata !{i32 786443, metadata !86, i32 136, i32 0, metadata !6, i32 7} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!187 = metadata !{i32 141, i32 0, metadata !81, null}
!188 = metadata !{i32 142, i32 0, metadata !88, null}
!189 = metadata !{i32 144, i32 0, metadata !190, null}
!190 = metadata !{i32 786443, metadata !88, i32 143, i32 0, metadata !6, i32 9} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!191 = metadata !{i32 145, i32 0, metadata !190, null}
!192 = metadata !{i32 147, i32 0, metadata !193, null}
!193 = metadata !{i32 786443, metadata !190, i32 146, i32 0, metadata !6, i32 10} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!194 = metadata !{i32 148, i32 0, metadata !193, null}
!195 = metadata !{i32 152, i32 0, metadata !81, null}
!196 = metadata !{i32 154, i32 0, metadata !197, null}
!197 = metadata !{i32 786443, metadata !81, i32 153, i32 0, metadata !6, i32 11} ; [ DW_TAG_lexical_block ] [/Users/james/projects/oclgrind/tests/lb.cl]
!198 = metadata !{i32 155, i32 0, metadata !197, null}
!199 = metadata !{i32 156, i32 0, metadata !81, null}

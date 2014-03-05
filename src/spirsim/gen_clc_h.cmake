set(OUTPUT src/spirsim/clc_h.cpp)

file(WRITE ${OUTPUT} "extern const char CLC_H_DATA[] = \n\"")

file(READ ${CMAKE_CURRENT_LIST_DIR}/clc.h CLC_H)
string(REGEX REPLACE "\\\\" "\\\\\\\\" CLC_H "${CLC_H}")
string(REGEX REPLACE "\"" "\\\\\"" CLC_H "${CLC_H}")
string(REGEX REPLACE "\n" "\\\\n\"\n\"" CLC_H "${CLC_H}")
file(APPEND ${OUTPUT} "${CLC_H}")

file(APPEND ${OUTPUT} "\";")

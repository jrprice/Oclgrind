set(OUTPUT src/core/clc_h.cpp)

file(WRITE ${OUTPUT} "extern const char CLC_H_DATA[] = \n\"")

file(READ ${SOURCE_FILE} CLC_H)
string(REGEX REPLACE "\\\\" "\\\\\\\\" CLC_H "${CLC_H}")
string(REGEX REPLACE "\"" "\\\\\"" CLC_H "${CLC_H}")
string(REGEX REPLACE "\n" "\\\\n\"\n\"" CLC_H "${CLC_H}")
file(APPEND ${OUTPUT} "${CLC_H}")

file(APPEND ${OUTPUT} "\";")

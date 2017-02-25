set(OUTPUT src/core/opencl-c.h.cpp)

file(WRITE ${OUTPUT} "extern const char OPENCL_C_H_DATA[] = \n\"")

file(READ ${SOURCE_FILE} OPENCL_C_H)
string(REGEX REPLACE "\\\\" "\\\\\\\\" OPENCL_C_H "${OPENCL_C_H}")
string(REGEX REPLACE "\"" "\\\\\"" OPENCL_C_H "${OPENCL_C_H}")
string(REGEX REPLACE "\n" "\\\\n\"\n\"" OPENCL_C_H "${OPENCL_C_H}")
file(APPEND ${OUTPUT} "${OPENCL_C_H}")

file(APPEND ${OUTPUT} "\";")

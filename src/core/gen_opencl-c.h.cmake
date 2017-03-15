set(OUTPUT src/core/opencl-c.h.cpp)

# Load opencl-c.h
file(READ ${SOURCE_FILE} OPENCL_C_H)

# Replace each character with a C character literal, escaping as necessary
string(REGEX REPLACE "(.)" "'\\1', " CONTENT "${OPENCL_C_H}")
string(REGEX REPLACE "\n'" "\\\\n'\n" CONTENT "${CONTENT}")
string(REGEX REPLACE "\\\\'" "\\\\\\\\'" CONTENT "${CONTENT}")

# Write character array
file(WRITE ${OUTPUT} "extern const char OPENCL_C_H_DATA[] = {\n")
file(APPEND ${OUTPUT} "${CONTENT}")
file(APPEND ${OUTPUT} "'\\0'};\n")

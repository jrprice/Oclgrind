set(OUTPUT src/core/opencl-c.h.cpp)
get_filename_component(SOURCE_FILE_DIR ${SOURCE_FILE} DIRECTORY)

# Load opencl-c.h
file(READ ${SOURCE_FILE} OPENCL_C_H)
file(READ ${SOURCE_FILE_DIR}/opencl-c-base.h OPENCL_C_BASE_H)
string(REPLACE "#include \"opencl-c-base.h\"" "${OPENCL_C_BASE_H}" CONTENT "${OPENCL_C_H}")

# Replace each character with a C character literal, escaping as necessary
string(REGEX REPLACE "(.)" "'\\1', " CONTENT "${CONTENT}")
string(REGEX REPLACE "\n'" "\\\\n'\n" CONTENT "${CONTENT}")
string(REGEX REPLACE "\\\\'" "\\\\\\\\'" CONTENT "${CONTENT}")

# Write character array
file(WRITE ${OUTPUT} "extern const char OPENCL_C_H_DATA[] = {\n")
file(APPEND ${OUTPUT} "${CONTENT}")
file(APPEND ${OUTPUT} "'\\0'};\n")

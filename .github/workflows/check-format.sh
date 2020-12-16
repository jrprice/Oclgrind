#!/usr/bin/env bash

# Run git-clang-format to check for violations
OUTPUT=$(git-clang-format --diff origin/master --extensions c,cpp,h,hpp)

# Check for no-ops
grep '^no modified files to format$' <<<"$OUTPUT" && exit 0
grep '^clang-format did not modify any files$' <<<"$OUTPUT" && exit 0

# Dump formatting diff and signal failure
echo -e "\n==== FORMATTING VIOLATIONS DETECTED ====\n"
echo "$OUTPUT"
exit 1

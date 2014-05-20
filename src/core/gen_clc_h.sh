#!/bin/bash

if [ $# -ne 2 ]
then
  echo "Usage: gen_clc_h.sh INPUT OUTPUT"
  exit 1
fi

IN=$1
OUT=$2

echo "extern const char CLC_H_DATA[] =" >$OUT
sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^/"/;s/$/\\n"/' $IN >>$OUT
if [ $? -ne 0 ]
then
  exit 1
fi
echo ";" >>$OUT

#!/bin/bash

suite="$1"
shift
ctsargs="$*"

ctsdir="${suite}"
ctsbin="test_${suite}"
if [ "${suite}" == "multiple_device_context" ]; then
  ctsbin="test_multiples"
elif [ "${suite}" == "math_brute_force" ]; then
  ctsbin="test_bruteforce"
elif [ "${suite}" == "images_clCopyImage" ]; then
  ctsdir="images/${suite#images_}"
  ctsbin="test_cl_copy_images"
elif [ "${suite}" == "images_clFillImage" ]; then
  ctsdir="images/${suite#images_}"
  ctsbin="test_cl_fill_images"
elif [ "${suite}" == "images_clGetInfo" ]; then
  ctsdir="images/${suite#images_}"
  ctsbin="test_cl_get_info"
elif [ "${suite}" == "images_clReadWriteImage" ]; then
  ctsdir="images/${suite#images_}"
  ctsbin="test_cl_read_write_images"
elif [ "${suite}" == "images_kernel_image_methods" ]; then
  ctsdir="images/${suite#images_}"
  ctsbin="test_kernel_image_methods"
elif [ "${suite}" == "images_kernel_read_write" ]; then
  ctsdir="images/${suite#images_}"
  ctsbin="test_image_streams"
elif [ "${suite}" == "images_samplerlessReads" ]; then
  ctsdir="images/${suite#images_}"
  ctsbin="test_samplerless_reads"
fi

retcode=0

export PATH=$PWD/install/bin:$PATH
export CL_CONFORMANCE_RESULTS_FILENAME=$PWD/result.json
echo oclgrind opencl-cts/build/test_conformance/${ctsdir}/${ctsbin} ${ctsargs}
oclgrind opencl-cts/build/test_conformance/${ctsdir}/${ctsbin} ${ctsargs}

echo

if [ ! -r "${CL_CONFORMANCE_RESULTS_FILENAME}" ]; then
  echo "Conformance results file not found."
  exit 1
fi

grep ': "fail"' "${CL_CONFORMANCE_RESULTS_FILENAME}" | \
    awk -F '"' '{print $2}' >FAILED

if [ -r ".github/workflows/cts-xfail/${suite}" ]; then
  new_fails=$(fgrep -xvf .github/workflows/cts-xfail/${suite} FAILED)
  new_passes=$(fgrep -xvf FAILED .github/workflows/cts-xfail/${suite})
else
  new_fails=$(cat FAILED)
  new_passes=
fi

if [ -n "${new_fails}" ]; then
  echo "-------------------"
  echo "Unexpected failures"
  echo "-------------------"
  echo "${new_fails}"
  echo
  retcode=1
fi
if [ -n "${new_passes}" ]; then
  echo "-----------------"
  echo "Unexpected passes"
  echo "-----------------"
  echo "${new_passes}"
  echo
  retcode=1
fi
if [ ${retcode} -eq 0 ]; then
  if [ -s ".github/workflows/cts-xfail/${suite}" ]; then
    echo "-----------------"
    echo "Expected failures"
    echo "-----------------"
    cat ".github/workflows/cts-xfail/${suite}"
    echo
  fi
fi

exit ${retcode}

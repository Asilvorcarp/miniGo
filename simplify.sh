#!/bin/bash

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <filename>"
  exit 1
fi

vim -E -s "$1" <<-EOF
  g/cfi/d
  g/.p2align/d
  g/.type/d
  g/.file/d
  g/addl.\$0,/d
  wq
EOF!/bin/

vim -E -s file.txt <<-EOF
  g/cfi/d
  wq
EOF
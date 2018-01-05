#!/bin/bash

# NOTE: change the bandwidth limit according to the size of your device
# to get a reasonable run time.

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Use: $0 DEVICE"
    exit 1
fi

DEV=$1

make &&
    sudo \
        time -p \
        ./progress $DEV
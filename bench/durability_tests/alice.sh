#!/usr/bin/env bash

set -x

python bench/durability_tests/txn_reorder.py /tmp/pao214/mntdir > /dev/null 2>&1
cat /tmp/pao214/.cache/client/txn_reorder.txt
umount /tmp/pao214/mntdir

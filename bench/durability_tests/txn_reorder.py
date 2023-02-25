#!/usr/bin/env python

import os
import sys

def main():
	os.chdir(sys.argv[1])
	fd = os.open("txn_reorder.txt", os.O_CREAT | os.O_RDWR)
	os.write(fd, bytes("txn_beg 0\n", "utf-8"))
	os.write(fd, bytes("txn_end 0\n", "utf-8"))
	os.fsync(fd)
	os.lseek(fd, 0, os.SEEK_SET)
	os.write(fd, bytes("txn_beg 1\n", "utf-8"))
	os.write(fd, bytes("txn_end 1\n", "utf-8"))
	os.close(fd)

main()

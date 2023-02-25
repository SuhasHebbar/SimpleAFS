#!/usr/bin/env python

import os
import sys

def main():
	os.chdir(sys.argv[1])
	fd = os.open("write_reorder.txt", os.O_CREAT | os.O_RDWR)
	for i in range(0, 100):
		for _ in range(0, 100):
			os.pwrite(fd, bytes("a\n", "utf-8"), 2*i)
			os.pwrite(fd, bytes("b\n", "utf-8"), 2*i)
	os.close(fd)

main()

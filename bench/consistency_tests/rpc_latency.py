#!/usr/bin/env python

import os
import time
import sys
import numpy as np
import matplotlib.pyplot as plt


def get_rpc_latency(mntdir, fname, nbytes):
    os.chdir(mntdir)
    target_str = "A" * (nbytes - 1) + "\n"
    fd = os.open(fname, os.O_CREAT | os.O_TRUNC | os.O_RDWR)
    os.write(fd, bytes(target_str, "utf-8"))
    os.fsync(fd)
    start_time = time.monotonic_ns()
    os.close(fd)
    return time.monotonic_ns() - start_time


def main():
    mntdir = sys.argv[1]
    fname = sys.argv[2]
    curdir = os.getcwd()

    exp = range(8)
    msgs = [8**x for x in exp]

    latencies = []
    for _ in msgs:
        latencies.append([])

    for _ in range(100):
        for j, sz in enumerate(msgs):
            latencies[j].append(get_rpc_latency(mntdir, fname, sz))

    p25 = []
    p50 = []
    p75 = []
    for j, _ in enumerate(msgs):
        np_lats = np.array([lat/1000 for lat in latencies[j]])
        p25.append(np.percentile(np_lats, 25))
        p50.append(np.percentile(np_lats, 50))
        p75.append(np.percentile(np_lats, 75))
    
    fig, ax = plt.subplots()

    # plot 25, 50, 75 %ile latencies
    ax.plot(exp, p25, label="25 %ile")
    ax.plot(exp, p50, label="50 %ile")
    ax.plot(exp, p75, label="75 %ile")

    # legend, labels and title
    ax.legend()
    ax.set_xlabel("msg size in bytes (log 8 scale)")
    ax.set_ylabel("Round trip latency in us")
    ax.set_title("Round trip latency stats (100 runs)")

    # create in local directory
    os.chdir(curdir)
    fig.savefig("rtlat.png")


if __name__ == "__main__":
    main()

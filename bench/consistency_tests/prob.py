#!/usr/bin/env python

import os
import sys
import matplotlib.pyplot as plt
import shutil

from multiprocessing import Pool


def get_mtime(mntdir, fname, cname):
    os.chdir(mntdir)
    target_str = cname * (1024 - 1) + "\n"
    fd = os.open(fname, os.O_CREAT | os.O_TRUNC | os.O_RDWR)
    os.write(fd, bytes(target_str, "utf-8"))
    os.close(fd)
    # No race lieu of different directories
    return os.path.getmtime(fname)


def main():
    mdir1 = sys.argv[1]
    mdir2 = sys.argv[2]
    fname = sys.argv[3]
    curdir = os.getcwd()

    stimes = [100000000 * x for x in range(10)]

    probs = []
    for stime in stimes:
        os.chdir(mdir2)
        with open("tmp.conf", "w") as fw:
            with open("unreliablefs.conf", "r") as fr:
                for line in fr:
                    if line.strip().startswith("duration"):
                        fw.write(f"duration = {stime}")
                    else:
                        fw.write(line)
        shutil.move("tmp.conf", "unreliablefs.conf")

        b_wins = 0
        a_wins = 0
        with Pool(3) as p:
            for _ in range(1000):
                mtimes = p.starmap(get_mtime, [(mdir2, fname, "B"), (mdir1, fname, "A")])
                if mtimes[0] < mtimes[1]:
                    b_wins += 1
                elif mtimes[0] > mtimes[1]:
                    a_wins += 1
        probs.append(b_wins/(float(a_wins+b_wins)))

    # plot
    fig, ax = plt.subplots()
    ax.plot(exp, probs)
    ax.set_xlabel("slowdown in ns (log 5 scale)")
    ax.set_ylabel("Prob.")
    ax.set_title("Empirical prob. of slower server winning")

    # create in local directory
    os.chdir(curdir)
    fig.savefig("prob.png")


if __name__ == "__main__":
    main()

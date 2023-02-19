ops = set()


op_map = {
    "getattr",
    "readlink",
    "mknod",
    "mkdir",
    "unlink",
    "rmdir",
    "symlink",
    "rename",
    "link",
    "chmod",
    "chown",
    "truncate",
    "open",
    "read",
    "write",
    "statfs",
    "flush",
    "release",
    "fsync",
    "setxattr",
    "getxattr",
    "listxattr",
    "removexattr",
    "opendir",
    "readdir",
    "releasedir",
    "fsyncdir",
    "access",
    "creat",
    "ftruncate",
    "fgetattr",
    "lock",
    "ioctl",
    "flock",
    "fallocate",
    "utimens",
    "lstat"
}

open_flags = 0

with open('stdout.log', 'r') as f:
    for i, line in enumerate(f):
        try:
            l = line.split(' ')
            if l[0] == 'Operation:' and l[1][0:-1] in op_map:
                op = l[1][0:-1]
                ops.add(op)
                if op == 'symlink':
                    print(line)
            elif l[0] == 'Detail_open':
                open_flags = int(l[1]) | open_flags
        except:
            print(line)
        # if i % 80000 == 0:
        #     print(f'progressed to {i}')

print(ops)

print(', '.join(list(ops)))
print('{0:b}'.format(open_flags))



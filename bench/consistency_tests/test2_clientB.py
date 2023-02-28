#! /usr/bin/env python3
# Test case for interleaving writes

import os
import fs_util
import sys
import time
'''
This is ClientA.
'''

cs739_env_vars = [
    'CS739_CLIENT_A', 'CS739_CLIENT_B', 'CS739_SERVER', 'CS739_MOUNT_POINT'
]
ENV_VARS = {var_name: os.environ.get(var_name) for var_name in cs739_env_vars}
for env_var in ENV_VARS.items():
    print(env_var)
    assert env_var is not None
TEST_DATA_DIR = ENV_VARS['CS739_MOUNT_POINT'] + '/test_consistency'
FNAME = f'{TEST_DATA_DIR}/case2'
print(TEST_DATA_DIR)
TEST_CASE_NO = 2


def run_test():
    host_b = ENV_VARS['CS739_CLIENT_B']
    assert fs_util.test_ssh_access(host_b)
    signal_name_gen = fs_util.get_fs_signal_name()

    if not fs_util.path_exists(TEST_DATA_DIR):
        fs_util.mkdir(TEST_DATA_DIR)

    # init
    if not fs_util.path_exists(FNAME):
        fs_util.create_file(FNAME)

    print("Client A: Start Client B and Write to file case2 together")
    print("Client A: Write a's to file case2")
    print("Client B: Write b's to file case2")

    # time for client_b to work, host_b should read the all-zero file
    cur_signal_name = next(signal_name_gen)
    fs_util.start_another_client(host_b, 2, 'B', cur_signal_name)    


    init_str = fs_util.gen_str_by_repeat('a', 100)
    fd = fs_util.open_file(FNAME)
    fs_util.write_file(fd, init_str)
    fs_util.close_file(fd)

    # wait until client_b finish to check for interleaving
    while True:
        removed = fs_util.poll_signal_remove(host_b, cur_signal_name)
        if removed:
            break
        time.sleep(1)
    print('Clientb finished now check for sanity')

    # read the things and check that file has no interleaving
    print("Check if file is has interleave writes")
    fd = fs_util.open_file(FNAME)
    cur_str = fs_util.read_file(fd, 100, start_off=0)
    assert len(cur_str) == 100
    counta = 0
    countb = 0
    for c in cur_str:
        if c == 'a':
            assert countb == 0
            counta+=1
        else:
            assert counta == 0
            countb+=1

    print("Count of a: ", counta)
    print("Count of b: ", countb)
    print("Check passed: File has no interleave writes")        

    # done
    fs_util.record_test_result(TEST_CASE_NO, 'A', 'OK')


if __name__ == '__main__':
    run_test()
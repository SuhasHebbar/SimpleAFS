#! /usr/bin/env python3
# Test case for interleaving writes

import os
import fs_util
import sys
import time
import subprocess
import threading
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
FNAME = f'{TEST_DATA_DIR}/case4'
print(TEST_DATA_DIR)
TEST_CASE_NO = 4
SERVER_FILE_PATH = "/tmp/consistency/serverfs/test_consistency/case4"


def start_client():
    with open(os.devnull, "w") as outfile:
        subprocess.Popen(["/users/hebbar2/ali/CS739-P1/debug/unreliablefs/ali_client", ENV_VARS['CS739_MOUNT_POINT'], "-cachedir=/tmp/consistency/cachedir", "-serveraddr=0.0.0.0:5002", "-seed=1618680646", "-d"], stdout=outfile, stderr = outfile)

def kill_client():
    subprocess.run(["pkill", "-KILL", "ali_client"])

def start_server():
    subprocess.Popen(["/users/hebbar2/ali/CS739-P1/debug/unreliablefs/ali_server", "/tmp/consistency/serverfs", "0.0.0.0:5002"])

def kill_server():
    subprocess.run(["pkill", "-KILL", "ali_server"])

def umount():
    subprocess.run(["umount", ENV_VARS['CS739_MOUNT_POINT']])    

def crash_server():
    time.sleep(2)
    # print("Thread awake")
    kill_server()
    print("Server crashed")


def check_sanity_on_client():
    return os.path.getsize(FNAME) == 2**30

def check_sanity_on_server():
    return os.path.getsize(SERVER_FILE_PATH) == 0




def clean_up():
    kill_client()
    time.sleep(1)
    kill_server()
    umount()


def run_test():

    print("Start server")

    start_server()

    time.sleep(1)

    print("Start Client")

    start_client()

    time.sleep(1)

    print("Client creates file: case4")

    if not fs_util.path_exists(TEST_DATA_DIR):
        fs_util.mkdir(TEST_DATA_DIR)

    # init
    if not fs_util.path_exists(FNAME):
        fs_util.create_file(FNAME)

    # writing to the file and crashing the client
    print("Client opens and starts writing 1GB data...")
    fd = fs_util.open_file(FNAME)
    str = fs_util.gen_str_by_repeat('0', 2**30)
    
    x = threading.Thread(target=crash_server, args=())

    fs_util.write_file(fd, str)
    
    x.start()
        
    try:    
        fs_util.close_file(fd)
    
    except:
        print("Error")
        pass

    time.sleep(5)
    
    print("Restart server")

    start_server()
    time.sleep(10)

    print("Check if client contains the written file")

    if check_sanity_on_client() :    
        print("Check passed: File size is 1GB")

    print("Check if server contains empty file")

    if check_sanity_on_server():
        print("Check passed: File size is 0Bytes")        

    # done
    fs_util.record_test_result(TEST_CASE_NO, 'A', 'OK')

    clean_up()


if __name__ == '__main__':
    run_test()
#! /usr/bin/env python3

import os
import fs_util
import sys
import test5_clientA as test5  # Simply do not want to do test1_common
import time
'''
This is ClientB.
'''


def run_test():
    signal_name_gen = fs_util.get_fs_signal_name()

    cur_signal_name = next(signal_name_gen)
    fs_util.record_test_result(test5.TEST_CASE_NO, 'B',
                               f'Check fdir:{test5.TEST_DATA_DIR}')
    fs_util.wait_for_signal(cur_signal_name)

    if not fs_util.path_exists(test5.TEST_DATA_DIR):
        fs_util.record_test_result(test5.TEST_CASE_NO, 'B', "Directory does not exist")

    else:
        fs_util.record_test_result(test5.TEST_CASE_NO, 'B', "Directory exists")    

    os.rmdir(test5.TEST_DATA_DIR)

    time.sleep(1)

    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    fs_util.wait_for_signal(cur_signal_name, last_signal_name=last_signal_name)

    
    # done
    fs_util.record_test_result(test5.TEST_CASE_NO, 'B', 'OK')


if __name__ == '__main__':
    run_test()

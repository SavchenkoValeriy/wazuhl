import os
import sys

def set_wd(directory):
    define_getter("get_wd", directory)


def set_output(directory):
    directory = os.path.abspath(directory)
    define_getter("get_output", directory)


def get_clang():
    return sys.argv[1]


def get_clangpp():
    return sys.argv[1] + "++"


def get_mongodb():
    return os.path.join(get_wd(), "wazuhl", "mongo")


def define_getter(name, value):
    globals()[name] = lambda : value


def get_alpha():
    return 0.5


def get_llvm_test_suite_path():
    return sys.argv[2]


def get_llvm_test_suite_build_path():
    return sys.argv[3]


def get_caffe_bin():
    return sys.argv[4]


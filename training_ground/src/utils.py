import logging
import os
import sys
from distutils.spawn import find_executable


def check_file(file_to_check):
    if not os.path.exists(file_to_check):
        error("There is no such file: \"{0}\"".format(file_to_check))


def check_executable(executable):
    if not find_executable(executable):
        error("Couldn't find '{0}' executable, please, check your environment".format(executable))


def error(message):
    logging.error("Error: {}".format(message))
    sys.exit(1)


def pathify(line):
    return '_'.join(line.split())

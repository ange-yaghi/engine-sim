#!python3

"""
This is a short script which copies all required runtime DLLs to the 
target runtime directory.
"""

import sys
import os
import os.path
import shutil
from distutils.dir_util import copy_tree
from standard_logging import *

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = SCRIPT_PATH + "/../"
RUNTIME_DLL_DIRECTORY = ROOT_DIR + "/dependencies/runtime/"
TARGET_DIRECTORY = ROOT_DIR + "/project/"


def log(level, data):
    print("{0}: {1}".format(level, data))


def generate_target_directory(architecture, configuration):
    return TARGET_DIRECTORY + "/{}/{}/".format(architecture, configuration)


def generate_source_directory(architecture, configuration):
    return RUNTIME_DLL_DIRECTORY + "/{}/".format(architecture)


def copy_dlls(source, target):
    target_directory = target
    source_directory = source

    log("INFO", "Copying required runtime DLLs")
    for root, sub_dirs, files in os.walk(source_directory):
        for file_entry in files:
            if not file_entry.endswith('.dll'):
                continue

            filename = os.path.join(root, file_entry.strip())
            shutil.copy(filename, target_directory)


if __name__ == "__main__":
    architecture = sys.argv[1]
    configuration = sys.argv[2]

    print_full_header('Copy Runtime DLLs')
    source_directory = generate_source_directory(architecture, configuration)
    target_directory = generate_target_directory(architecture, configuration)
    copy_dlls(source_directory, target_directory)
    print_footer()

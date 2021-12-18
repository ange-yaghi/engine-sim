#!python3

"""
This script packages the application into a redistributable.
"""

import sys
import os
import os.path
import shutil
from distutils.dir_util import copy_tree
from standard_logging import *
import copy_dlls

class Resource(object):
    def __init__(self, name, src, target, resource_type='folder', optional=False):
        self.name = name
        self.src = src
        self.target = target
        self.resource_type = resource_type
        self.optional = optional

    def format(self, d):
        self.src = self.src.format(**d)
        self.target = self.target.format(**d)


def log(level, data):
    print("{0}: {1}".format(level, data))


def clean(directories):
    log("INFO", "Cleaning last build...")

    for directory in directories:
        try:
            shutil.rmtree(directory)
        except FileNotFoundError:
            log("INFO", "No previous build found, skipping clean")
        except OSError:
            log("ERROR", "Could not clean build, files are likely in use")
            sys.exit(1) # Return with an error


def create_dirs(directories):
    for directory in directories:
        try:
            os.makedirs(directory, mode=0o777, exist_ok=False)
        except FileExistsError:
            log("WARNING", "Build folder was not cleaned")
            pass


def copy_resource(resource):
    log("INFO", "Copying resource: {} [{} ==> {}]".format(resource.name, resource.src, resource.target))

    if resource.resource_type == "folder":
        if resource.optional:
            if not os.path.exists(resource.src):
                log("INFO", "Optional resource not found {}".format(resource.src))
                return

        try:
            if resource.optional:
                clean([resource.target])

            shutil.copytree(resource.src, resource.target)
        except FileNotFoundError:
            log("INFO", "Could not find resource {}".format(resource.src))
        except OSError:
            log("ERROR", "Could not copy resource: {} ==> {}, files likely in use".format(resource.src, resource.target))
            sys.exit(1) # Return with an error
    elif resource.resource_type == "file":
        shutil.copy(resource.src, resource.target)
    elif resource.resource_type == "dlls":
        copy_dlls.copy_dlls(resource.src, resource.target)
    else:
        log("ERROR", "Unrecognized resource type: {}".format(resource.resource_type))


def build(input_arguments, clean_directories, new_directories, resources, variables):
    variable_map = input_arguments
    for k, v in variables:
        formatted = v.format(**variable_map)
        variable_map[k] = formatted

    print_full_header('Packaging {project_name}'.format(**variable_map))

    clean_directories = [d.format(**variable_map) for d in clean_directories]
    new_directories = [d.format(**variable_map) for d in new_directories]

    clean(clean_directories)
    create_dirs(new_directories)

    for resource in resources:
        resource.format(variable_map)
        copy_resource(resource)


def run(args, argument_template, clean_directories, new_directories, resources, variables):
    input_arguments = {}
    for i in range(1, len(args)):
        input_arguments[argument_template[i - 1]] = args[i]

    build(input_arguments, clean_directories, new_directories, resources, variables)


if __name__ == "__main__":
    run(sys.argv, [], [], [], [])

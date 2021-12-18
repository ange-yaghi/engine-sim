#!python3

import sys
import package_build
import os

ARGUMENTS = ['project_name']

VARIABLES = [
    ('script_path', os.path.dirname(os.path.realpath(__file__))),
    ('root', "{script_path}/.."),
    ('dependencies', "{root}/dependencies"),
    ('delta', "{dependencies}/delta"),
    ('submodules',"{dependencies}/submodules"),
    ('delta_build',"{submodules}/delta-studio/workspace/build/delta")
]

CLEAN_DIRECTORIES = []

NEW_DIRECTORIES = []

RESOURCES = [
    package_build.Resource('delta_build', '{delta_build}', '{dependencies}/delta', optional=True)
]

if __name__ == "__main__":
    package_build.run(sys.argv, ARGUMENTS, CLEAN_DIRECTORIES, NEW_DIRECTORIES, RESOURCES, VARIABLES)

#!python3

import sys
import package_build
import os

ARGUMENTS = [
    'architecture',
    'configuration',
    'vs_crt_path',
    'project_name'
]

VARIABLES = [
    ('script_path', os.path.dirname(os.path.realpath(__file__))),
    ('root', "{script_path}/.."),
    ('dependencies', "{root}/dependencies"),
    ('dll_dir', "{dependencies}/runtime/{architecture}"),
    ('workspace', "{root}/workspace"),
    ('output', "{workspace}/build"),
    ('vs_build_output', "{root}/project/{architecture}/{configuration}"),
    ('delta', "{root}/dependencies/delta")
]

CLEAN_DIRECTORIES = [
    '{output}'
]

NEW_DIRECTORIES = [
    '{output}'
]

RESOURCES = [
    package_build.Resource('assets', '{root}/assets', '{output}/assets'),
    package_build.Resource('fonts', '{delta}/engines/basic/fonts', '{output}/delta/fonts'),
    package_build.Resource('shaders', '{delta}/engines/basic/shaders', '{output}/delta/shaders'),
    package_build.Resource('runtime dependencies', '{dll_dir}/', '{output}/', resource_type='dlls'),
    package_build.Resource('binaries', '{vs_build_output}/{project_name}.exe', '{output}/', resource_type='file'),
    package_build.Resource('configuration', '{root}/configuration/delta.conf', '{output}/', resource_type='file'),
    package_build.Resource('visual studio redist', '{vs_crt_path}', '{output}/', resource_type='dlls')
]

if __name__ == "__main__":
    package_build.run(sys.argv, ARGUMENTS, CLEAN_DIRECTORIES, NEW_DIRECTORIES, RESOURCES, VARIABLES)

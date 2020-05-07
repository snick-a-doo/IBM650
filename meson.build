project('c++ IBM650lib', 'cpp',
        version : '0.1.0',
        license : 'GPL3')
add_global_arguments('-Dwarning_level=3', language : 'cpp')

install_headers('computer.hpp', 'register.hpp')

boost_dep = dependency('boost', modules : 'log')
threads_dep = dependency('threads')

IBM650_sources = ['computer.cpp', 'register.cpp']
IBM650lib = shared_library('IBM650',
                           IBM650_sources,
                           dependencies : [boost_dep, threads_dep],
                           install : true)

test_sources = ['test.cpp', 'test_computer.cpp', 'test_opcodes.cpp', 'test_register.cpp']
test_app = executable('test_app',
                     test_sources,
                     link_with : IBM650lib)

test('computer test', test_app)

subdir('UI')

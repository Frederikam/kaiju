project('kaiju', 'c', 'cpp', version: '0.0.1', default_options: ['c_std=c11', 'cpp_std=c++14'])

add_project_arguments(['-DWLR_USE_UNSTABLE', '-std=c11'], language: 'cpp')

deps = [
    dependency('gtk+-3.0'),
    dependency('wlroots'),
]

executable('kaiju', 'main.cpp', dependencies: deps)

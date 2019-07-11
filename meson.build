project('kaiju', 
    'cpp', 
    version: '0.0.1', 
    default_options: ['cpp_std=c++14']
)

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: 'cpp')

deps = [
    dependency('wlroots'),
    dependency('wayland-server'),
]

# Meson does not allow wildcard searched for sources.
# We use a shell script for that.
c = run_command('find_sources.sh')
sources = c.stdout().strip().split('\n')

executable('kaiju', sources, dependencies: deps)

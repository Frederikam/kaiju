project('kaiju',
    'c',
    version: '0.0.1'
)

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: 'c')

wayland_protocols = dependency('wayland-protocols')
wayland_client = dependency('wayland-client')

subdir('protocols')

deps = [
    dependency('wlroots'),
    dependency('wayland-server'),
    dependency('pixman-1'),
    wayland_protocols,
    wayland_client,
    wlr_protocols,
]

# Meson does not allow wildcard searched for sources.
# We use a shell script for that.
c = run_command('find_sources.sh')
sources = c.stdout().strip().split('\n')

# The -ldl flag is required for dlfcn.h
executable('kaiju', sources, dependencies: deps, link_args: '-ldl')

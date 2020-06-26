project('obs-nvfbc', 'c')

shared_library('nvfbc', 'nvfbc.c',
    name_prefix : '',
    dependencies : [
        dependency('threads'),
        meson.get_compiler('c').find_library('obs'),
        meson.get_compiler('c').find_library('dl'),
    ],
    install : true,
    install_dir : join_paths(get_option('libdir'), 'obs-plugins'),
)

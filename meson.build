project('obs-nvfbc', 'c', default_options: ['c_std=c99'])

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

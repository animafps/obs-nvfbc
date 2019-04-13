project('obs-nvfbc', 'c')

shared_library('nvfbc', 'nvfbc.c',
    name_prefix : '',
    dependencies : [
        dependency('threads'),
        meson.get_compiler('c').find_library('obs'),
        meson.get_compiler('c').find_library('dl', required : false),
    ],
    install : true,
    install_dir : 'lib/obs-plugins',
)

project('obs-nvfbc', 'c', default_options: ['c_std=c99'])

threads = dependency('threads')
obs = meson.get_compiler('c').find_library('obs')
dl = meson.get_compiler('c').find_library('dl')

shared_library('nvfbc', 'nvfbc.c',
    name_prefix : '',
    dependencies : [threads, obs, dl],
    install : true,
    install_dir : join_paths(get_option('libdir'), 'obs-plugins'),
)

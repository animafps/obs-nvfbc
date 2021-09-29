project('obs-nvfbc', 'c', default_options: ['c_std=c99'])

threads = dependency('threads')
obs = meson.get_compiler('c').find_library('obs')
gl = dependency('gl')

shared_library('nvfbc', 'nvfbc.c',
    name_prefix : '',
    dependencies : [threads, obs, gl],
    install : true,
    c_args : [
      '-D_GNU_SOURCE',
    ],
    install_dir : join_paths(get_option('libdir'), 'obs-plugins'),
)

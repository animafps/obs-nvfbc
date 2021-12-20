project('obs-nvfbc', 'c', default_options: ['c_std=c99'])

threads = dependency('threads')
obs = meson.get_compiler('c').find_library('obs')
gl = dependency('gl')
if target_machine.system() != 'windows'
    x11 = dependency('x11')
else
    x11 = dependency('', required : false)
endif

shared_library('nvfbc', 'nvfbc.c',
    name_prefix : '',
    dependencies : [threads, obs, gl, x11],
    install : true,
    c_args : [
      '-D_GNU_SOURCE',
    ],
    install_dir : join_paths(get_option('libdir'), 'obs-plugins'),
)

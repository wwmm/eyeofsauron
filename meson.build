project(
	'eye_of_sauron',
	'c',
	'cpp',
	default_options : [
		'cpp_std=c++20',
		'buildtype=debugoptimized',
		'warning_level=2'
	],
	license: 'GPL-3-or-later',
	version: '0.0.1',
	meson_version: '>= 0.59.1'
)

cxx = meson.get_compiler('cpp')
assert(cxx.get_id() == 'gcc', 'This project only supports the gcc compiler due to usage of c++20 features.')

suppressed_warnings = [
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter'
  ]

add_project_arguments (
  suppressed_warnings,
  language: [ 'c', 'cpp' ],
  )

add_global_arguments('-DG_LOG_DOMAIN="eye_of_sauron"', language : ['c', 'cpp'])
add_global_arguments('-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
	language:'c')

gnome_mod = import('gnome')
i18n_mod = import('i18n')

prefix = get_option('prefix')
bindir = join_paths(prefix, get_option('bindir'))
datadir = join_paths(prefix, get_option('datadir'))
localedir = join_paths(prefix, get_option('localedir'))
config_h_dir = include_directories('.')
plugins_install_dir = '@0@/gstreamer-1.0'.format(get_option('libdir'))

status = []

if get_option('devel')
  status += [ 
    'Using development build mode with .Devel appended to the application ID.',
    'Also enabling -Werror.' 
  ]
  app_id_suffix = '.Devel'
  name_suffix = ' (Devel)'
  add_project_arguments ('-Werror', language: [ 'c', 'cpp' ])
else
  status += [ 'Using stable build mode with the standard application ID' ]
  app_id_suffix = ''
  name_suffix = ''
endif

# Configuration
conf = configuration_data()

conf.set10('IS_DEVEL_BUILD', get_option('devel'))
conf.set_quoted('APP_NAME', 'Eye Of Sauron' + name_suffix)
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('LOCALE_DIR', localedir)
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('PACKAGE', 'Eye Of Sauron')
conf.set_quoted('PLUGINS_INSTALL_DIR', join_paths(prefix, plugins_install_dir))

conf.set('COMMIT_DESC', '"@VCS_TAG@"')

config_h = declare_dependency(
  sources: vcs_tag(
    command: ['git', 'rev-parse', '--short', 'HEAD'],
    fallback: 'Could not find commit of build',
    input: configure_file(
      output: 'config.h.in',
      configuration: conf
    ),
    output: 'config.h'
  )
)

plugins_cxx_args = [
	'-Wl,-Bsymbolic-functions',
	'-fvisibility=hidden',
	'-fno-strict-aliasing'
]

subdir('data')
subdir('po')
subdir('src')

meson.add_install_script('meson_post_install.py')

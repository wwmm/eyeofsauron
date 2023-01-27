eye_of_sauron_sources = [
	'application.cpp',
	'application_ui.cpp',
	'chart.cpp',
	'eye_of_sauron.cpp',
	# 'microphone.cpp',
	# 'plot.cpp',
	# 'sound_wave.cpp',
	'preferences_general.cpp',
	'preferences_window.cpp',
	'tracker.cpp',
	'ui_helpers.cpp',
	'util.cpp',
	'webcam.cpp',
	'webcam_ui.cpp',
	gresources
]

eye_of_sauron_deps = [
	dependency('glib-2.0', version: '>=2.56', include_type: 'system'),
	dependency('gtk4', version: '>=4.2.1', include_type: 'system'),
	dependency('libadwaita-1', version: '>=1.2.0', include_type: 'system'),
	dependency('sigc++-3.0', version: '>=3.0.6', include_type: 'system'),
	dependency('gstreamer-1.0', version: '>=1.12.5', include_type: 'system'),
	dependency('boost', version: '>=1.72', modules:['system']),
	dependency('opencv4', include_type: 'system'),
	dependency('fmt', version: '>=8.0.0', include_type: 'system'),
	dependency('threads'),
	config_h
]

executable(
	meson.project_name(),
	eye_of_sauron_sources,
	include_directories : [config_h_dir],
	dependencies : eye_of_sauron_deps,
	install: true
)

# subdir('convolver')

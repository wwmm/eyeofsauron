schemadir = join_paths(datadir, 'glib-2.0', 'schemas')

install_data([
  'schemas/com.github.wwmm.eyeofsauron.gschema.xml',
], install_dir: schemadir)

i18n_mod.merge_file(
  input: 'com.github.wwmm.eyeofsauron.desktop.in',
  output: 'com.github.wwmm.eyeofsauron.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(datadir, 'applications')
)

icondir = join_paths(datadir, 'icons', 'hicolor')

install_data('eye_of_sauron.svg',
  install_dir: join_paths(icondir, 'scalable', 'apps')
)

conf_glade = configuration_data()
conf_glade.set('PROJECT_VERSION', meson.project_version())

gresources = gnome_mod.compile_resources(
	'eyeofsauron-resources',
	'eyeofsauron.gresource.xml',
	c_name: meson.project_name()
)
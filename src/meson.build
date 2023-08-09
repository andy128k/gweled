src_inc = include_directories('.')

app_sources = [
	'main.c', 'main.h',
	'gweled-gui.c', 'gweled-gui.h',
	'board_engine.c', 'board_engine.h',
	'graphic_engine.c', 'graphic_engine.h',
	'sge_core.c', 'sge_core.h',
	'sge_utils.c', 'sge_utils.h',
	'sound.c', 'sound.h',
	'gweled-scores.c', 'gweled-scores.h',
]

app_deps = [
  libm_dep,
  glib,
  gtk,
  clutter,
  clutter_gtk,
  libcanberra_gtk3,
  libgnome_games_support,
]


app_sources += gnome.compile_resources(
  meson.project_name() + '-resources',
  meson.project_name() + '.gresource.xml',
  source_dir: data_dir,
  c_name: '_gweled',
  export: true,
)


gweled = executable(
  meson.project_name(),
  'main.c',
  sources: app_sources,
  dependencies: app_deps,
  include_directories:  [
      config_h_inc,
      include_directories('.'),
    ],
  install: true,
  install_dir: bin_dir,
)



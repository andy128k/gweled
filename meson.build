project('gweled', 'c',
  version: '0.9.1-next',
  meson_version: '>= 0.59.0',
  license: 'GPL-2.0-or-later',
)

gnome = import('gnome')
i18n = import('i18n')


#################
# Default paths #
#################

prefix = get_option('prefix')
bin_dir = join_paths(prefix, get_option('bindir'))
locale_dir = join_paths(prefix, get_option('localedir'))
data_dir = join_paths(prefix, get_option('datadir'))
po_dir = join_paths(meson.project_source_root(), 'po')


###########
# Config #
###########

application_id = 'org.gweled.gweled'

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_URL', 'https://launchpad.net/gweled')
conf.set_quoted('PACKAGE_LOCALE_DIR', locale_dir)
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('DATA_DIRECTORY', data_dir)

conf.set_quoted('APPLICATION_ID', application_id)

config_h_inc = include_directories('.')
config_file = configure_file(output: 'config.h', configuration: conf)


# Compiler flags
cc = meson.get_compiler('c')
libm_dep = cc.find_library('m', required : false)


################
# Dependencies #
################

glib = dependency('glib-2.0', version: '>= 2.36')
gtk = dependency('gtk+-3.0', version: '>= 3.20')
clutter = dependency('clutter-1.0', version: '>= 1.20')
clutter_gtk = dependency('clutter-gtk-1.0', version: '>= 1.8')
libcanberra_gtk3 = dependency('libcanberra-gtk3', version: '>= 0.3')
libgnome_games_support = dependency('libgnome-games-support-1', version: '>= 1.8')


# Subdirs
subdir('src')
subdir('data')
subdir('po')

install_subdir('sounds',
  install_dir: join_paths (data_dir, 'sounds', meson.project_name()),
  strip_directory: true,
)

install_subdir('pixmaps',
  install_dir: join_paths (data_dir, 'pixmaps', meson.project_name()),
  strip_directory: true,
)


gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)

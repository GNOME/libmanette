project('libmanette','c',
  version: '0.2.5',
  meson_version: '>= 0.47.0',
)

gnome = import('gnome')

prefix = get_option('prefix')
libexecdir = prefix / get_option('libexecdir')

confinc = include_directories('.')
srcinc = include_directories('src')

libmanette_api_version = '0.2'
libmanette_module = 'manette-' + libmanette_api_version

glib_version = '>= 2.50'
gudev_version = '>= 1.0'
libevdev_version = '>= 1.4.5'

gio = dependency ('gio-2.0', version: glib_version)
glib = dependency ('glib-2.0', version: glib_version)
gobject = dependency ('gobject-2.0', version: glib_version)
gudev = dependency ('gudev-1.0', version: gudev_version, required : get_option('gudev'))
libevdev = dependency ('libevdev', version: libevdev_version)

config_h = configuration_data()

configure_file(
  output: 'libmanette-config.h',
  configuration: config_h,
)

subdir('src')
if get_option('build-tests')
  subdir('tests')
endif
if get_option('demos')
  subdir('demos')
endif

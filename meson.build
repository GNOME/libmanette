project('libmanette','c',
  version: '1.0.alpha',
  meson_version: '>= 0.53.0',
)

gnome = import('gnome')

prefix = get_option('prefix')
libexecdir = prefix / get_option('libexecdir')
datadir = prefix / get_option('datadir')

root_inc = include_directories('.')
src_inc = include_directories('src')

libmanette_version = meson.project_version()
libmanette_api_version = '1'
libmanette_module = 'manette-' + libmanette_api_version

glib_version = '>= 2.50'
gudev_version = '>= 1.0'
libevdev_version = '>= 1.4.5'

gio = dependency ('gio-2.0', version: glib_version)
glib = dependency ('glib-2.0', version: glib_version)
gobject = dependency ('gobject-2.0', version: glib_version)
gudev = dependency ('gudev-1.0', version: gudev_version, required : get_option('gudev'))
libevdev = dependency ('libevdev', version: libevdev_version)
hidapi = dependency ('hidapi-hidraw')

config_h = configuration_data()

config_h.set('_MANETTE_EXTERN', '__attribute__((visibility("default"))) extern')

configure_file(
  output: 'config.h',
  configuration: config_h,
)

subdir('src')
if get_option('build-tests')
  subdir('tests')
endif
if get_option('doc')
  subdir('doc')
endif
if get_option('demos')
  subdir('demo')
endif

summary(
  {
    'Demos': get_option('demos'),
    'Build tests': get_option('build-tests'),
    'Install tests': get_option('install-tests'),
  }, section: 'Demos and tests')
summary(
  {
    'Documentation': get_option('doc'),
    'Introspection': get_option('introspection'),
    'Vala bindings': get_option('vapi'),
  }, section: 'Documentation and introspection')
summary(
  {
    'gudev': get_option('gudev').enabled(),
  }, section: 'Optional dependencies')
summary(
  {
    'prefix': prefix,
    'libexecdir': libexecdir,
  }, section: 'Directories')

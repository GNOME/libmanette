project('libmanette','c',
  version: '0.2.5',
  meson_version: '>= 0.47.0',
)

gnome = import('gnome')

prefix = get_option('prefix')
libexecdir = prefix / get_option('libexecdir')

confinc = include_directories('.')
srcinc = include_directories('src')

libmanette_version = meson.project_version()
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
if get_option('doc')
  subdir('doc')
endif
if get_option('demos')
  subdir('demos')
endif

summary = [
  '',
  '------',
  'manette @0@ (@1@)'.format(libmanette_version, libmanette_api_version),
  '',
  '                   Demos: @0@'.format(get_option('demos')),
  '             Build tests: @0@'.format(get_option('build-tests')),
  '           Install tests: @0@'.format(get_option('install-tests')),
  'Documentation and introspection:',
  '           Documentation: @0@'.format(get_option('doc')),
  '           Introspection: @0@'.format(get_option('introspection')),
  '           Vala bindings: @0@'.format(get_option('vapi')),
  'Dependencies:',
  '                   gudev: @0@'.format(get_option('gudev').enabled()),
  'Directories:',
  '                  prefix: @0@'.format(prefix),
  '              libexecdir: @0@'.format(libexecdir),
  '------',
  ''
]

message('\n'.join(summary))

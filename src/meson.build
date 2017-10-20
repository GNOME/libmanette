api_version = '0.2'

libmanette_module = 'libmanette-' + api_version

libmanette_sources = [
]

libmanette_main_header = 'libmanette.h'

libmanette_headers = [
  libmanette_main_header,
]

version_split = meson.project_version().split('.')
MAJOR_VERSION = version_split[0]
MINOR_VERSION = version_split[1]
MICRO_VERSION = version_split[2]

version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
version_conf.set('MAJOR_VERSION', MAJOR_VERSION)
version_conf.set('MINOR_VERSION', MINOR_VERSION)
version_conf.set('MICRO_VERSION', MICRO_VERSION)

configure_file(
  input: 'manette-version.h.in',
  output: 'manette-version.h',
  configuration: version_conf,
  install: true,
  install_dir: join_paths(get_option('includedir'), 'libmanette')
)

libmanette_c_args = [
  '-DMANETTE_COMPILATION',
  '-DG_LOG_DOMAIN="Manette"',
]

libmanette_deps = [
  gio,
  glib,
  gobject,
  libevdev,
]

if gudev.found()
  libmanette_c_args += [ '-DGUDEV_ENABLED' ]
  libmanette_deps += [ gudev ]
endif

libmanette_lib = shared_library(
  libmanette_module,
  libmanette_sources,
  c_args: libmanette_c_args,
  dependencies: libmanette_deps,
  install: true,
)

libmanette_gir_args = [
   '--quiet',
]

install_headers(libmanette_headers, subdir: 'libmanette')

pkg = import('pkgconfig')

pkg.generate(
  description: 'A simple GObject game controller library',
    libraries: libmanette_lib,
         name: 'manette',
     filebase: 'manette-' + api_version,
      version: meson.project_version(),
      subdirs: 'libmanette',
     requires: 'glib-2.0',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

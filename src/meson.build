libmanette_resources = gnome.compile_resources(
  'manette_resources',
  'libmanette.gresource.xml',
  c_name: 'manette',
  source_dir: '.',
)

libmanette_sources = [
  libmanette_resources,
  'manette-device.c',
  'manette-event.c',
  'manette-event-mapping.c',
  'manette-mapping.c',
  'manette-mapping-manager.c',
  'manette-mapping-error.c',
  'manette-monitor.c',
  'manette-monitor-iter.c',
]

libmanette_main_header = 'libmanette.h'

libmanette_headers = [
  libmanette_main_header,
  'manette-device.h',
  'manette-event.h',
  'manette-monitor.h',
  'manette-monitor-iter.h',
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
  install_dir: get_option('includedir') / 'libmanette'
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
  include_directories: confinc,
  install: true,
  soversion: 0,
)

libmanette_gir_args = [
   '--quiet',
]

libmanette_gir_includes = [
  'GObject-2.0',
  'Gio-2.0',
]

if gudev.found()
  libmanette_gir_includes += [ 'GUdev-1.0' ]
endif

built_girs = []
if get_option('introspection')
  libmanette_gir = gnome.generate_gir(
    libmanette_lib,
    extra_args: libmanette_gir_args,
    export_packages: libmanette_module,
    sources: libmanette_headers + libmanette_sources,
    nsversion: libmanette_api_version,
    namespace: 'Manette',
    identifier_prefix: 'Manette',
    includes: libmanette_gir_includes,
    header: libmanette_main_header,
    install: true,
    symbol_prefix: 'manette',
  )
  built_girs += [ libmanette_gir ]

  if get_option('vapi')
    libmanette_vapi_packages = [
      'gobject-2.0',
      'gio-2.0',
    ]

    if gudev.found()
      libmanette_vapi_packages += [ 'gudev-1.0' ]
    endif

    gnome.generate_vapi(
      libmanette_module,
      install: true,
      packages: libmanette_vapi_packages,
      sources: libmanette_gir[0],
    )
  endif
endif

libmanette_dep = declare_dependency(
  link_with: libmanette_lib,
  sources: built_girs,
  include_directories: confinc,
  dependencies: libmanette_deps,
)

install_headers(libmanette_headers, subdir: 'libmanette')

pkg = import('pkgconfig')

pkg.generate(
  description: 'A simple GObject game controller library',
    libraries: libmanette_lib,
         name: 'libmanette',
     filebase: libmanette_module,
      version: meson.project_version(),
      subdirs: 'libmanette',
     requires: 'glib-2.0',
  install_dir: get_option('libdir') / 'pkgconfig'
)

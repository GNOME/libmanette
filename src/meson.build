libmanette_header_subdir = libmanette_module
libmanette_header_dir = get_option('includedir') / libmanette_header_subdir

libmanette_resources = gnome.compile_resources(
  'manette_resources',
  'libmanette.gresource.xml',
  c_name: 'manette',
  source_dir: '.',
)

libmanette_public_enum_headers = [
  'manette-device-type.h',
  'manette-inputs.h',
]

libmanette_public_enums = gnome.mkenums_simple('manette-enums',
  sources: libmanette_public_enum_headers,
  body_prefix: '#include "config.h"',
  header_prefix: '#if !defined(__MANETTE_INSIDE__) && !defined(MANETTE_COMPILATION)\n' +
                 '# error "Only <libmanette.h> can be included directly."\n' +
                 '#endif\n' +
                 '\n' +
                 '#include "manette-version.h"\n',
  decorator: 'MANETTE_AVAILABLE_IN_ALL',
  install_header: true,
  install_dir: libmanette_header_dir,
)

version_split = meson.project_version().split('.')
MAJOR_VERSION = version_split[0]
MINOR_VERSION = version_split[1]

if version_split[2].startswith('alpha') or version_split[2].startswith('beta') or version_split[2].startswith('rc')
  MICRO_VERSION = '0'
else
  MICRO_VERSION = version_split[2]
endif

version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
version_conf.set('MAJOR_VERSION', MAJOR_VERSION)
version_conf.set('MINOR_VERSION', MINOR_VERSION)
version_conf.set('MICRO_VERSION', MICRO_VERSION)

manette_version_h = configure_file(
  input: 'manette-version.h.in',
  output: 'manette-version.h',
  configuration: version_conf,
  install_dir: libmanette_header_dir,
)

libmanette_public_sources = [
  libmanette_public_enums,
  'manette-device.c',
  'manette-device-type.c',
  'manette-inputs.c',
  'manette-monitor.c',
  'manette-version.c',
]

libmanette_private_sources = [
  libmanette_resources,
  'drivers/manette-steam-deck-driver.c',
  'manette-backend.c',
  'manette-evdev-backend.c',
  'manette-event-mapping.c',
  'manette-hid-backend.c',
  'manette-hid-driver.c',
  'manette-mapping.c',
  'manette-mapping-manager.c',
  'manette-mapping-error.c',
]

libmanette_sources = [
  libmanette_public_sources,
  libmanette_private_sources,
]

libmanette_main_header = 'libmanette.h'

libmanette_headers = [
  libmanette_main_header,
  manette_version_h,
  'manette-device.h',
  'manette-device-type.h',
  'manette-inputs.h',
  'manette-monitor.h',
]

libmanette_generated_headers = [
  libmanette_public_enums[1],
]

libmanette_c_args = [
  '-DMANETTE_COMPILATION',
  '-DG_LOG_DOMAIN="Manette"',
]

libmanette_deps = [
  gio,
  glib,
  gobject,
  hidapi,
  libevdev,
]

if gudev.found()
  libmanette_c_args += [ '-DGUDEV_ENABLED' ]
  libmanette_deps += [ gudev ]
endif

libmanette_lib = library(
  libmanette_module,
  libmanette_sources,
  c_args: libmanette_c_args,
  dependencies: libmanette_deps,
  include_directories: [ root_inc, src_inc ],
  install: true,
  soversion: 0,
  gnu_symbol_visibility: 'hidden',
)

if get_option('build-tests')
  libmanette_internal = static_library(
    libmanette_module + '-internal',
    libmanette_sources,
    c_args: libmanette_c_args,
    dependencies: libmanette_deps,
    include_directories: [ root_inc, src_inc ],
  )

  libmanette_internal_dep = declare_dependency(
    sources: libmanette_generated_headers,
    dependencies: libmanette_deps,
    link_whole: libmanette_internal,
    include_directories: include_directories('.'),
  )
endif

libmanette_gir_args = [
   '--quiet',
   '--doc-format=gi-docgen',
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
  gir_dep = dependency('gobject-introspection-1.0')
  if gir_dep.version().version_compare('>=1.83.2')
    libmanette_gir_args += [
      '--doc-format=gi-docgen',
    ]
  endif

  libmanette_gir = gnome.generate_gir(
    libmanette_lib,
    extra_args: libmanette_gir_args,
    export_packages: libmanette_module,
    sources: libmanette_headers + libmanette_public_sources,
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
  sources: libmanette_generated_headers,
  include_directories: include_directories('.'),
  dependencies: libmanette_deps,
)

install_headers(libmanette_headers, subdir: libmanette_module)

pkg = import('pkgconfig')

pkg.generate(
  description: 'A simple GObject game controller library',
    libraries: libmanette_lib,
         name: libmanette_module,
     filebase: libmanette_module,
      version: meson.project_version(),
      subdirs: libmanette_module,
     requires: 'glib-2.0',
  install_dir: get_option('libdir') / 'pkgconfig'
)

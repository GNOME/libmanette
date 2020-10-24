subdir('xml')

glib_docpath = glib.get_pkgconfig_variable('prefix') / 'share' / 'gtk-doc' / 'html'
docpath = get_option('datadir') / 'gtk-doc' / 'html'

private_headers = [
  'manette-device-private.h',
  'manette-event-mapping-private.h',
  'manette-event-private.h',
  'manette-mapping-error-private.h',
  'manette-mapping-manager-private.h',
  'manette-mapping-private.h',
  'manette-monitor-iter-private.h',
]

content_files = [
]

gnome.gtkdoc(
  'manette',
  main_xml: 'manette-doc.xml',
  src_dir: [
    meson.source_root() / 'src',
    meson.build_root()  / 'src',
  ],
  ignore_headers: private_headers,
  dependencies: libmanette_dep,
  gobject_typesfile: 'manette.types',
  scan_args: [
    '--rebuild-types',
  ],
  mkdb_args: [
    '--default-includes=src/libmanette.h',
  ],
  fixxref_args: [
    '--html-dir=@0@'.format(docpath),
    '--extra-dir=@0@'.format(glib_docpath / 'glib'),
    '--extra-dir=@0@'.format(glib_docpath / 'gobject'),
    '--extra-dir=@0@'.format(glib_docpath / 'gio'),
    '--extra-dir=@0@'.format(glib_docpath / 'gi'),
  ],
  install_dir: 'manette',
  content_files: content_files,
  install: true,
)

expand_content_md_files = [
]

toml_data = configuration_data()
toml_data.set('VERSION', libmanette_version)

libmanette_toml = configure_file(
  input: 'libmanette.toml.in',
  output: 'libmanette.toml',
  configuration: toml_data
)

dependency(
  'gi-docgen', version: '>= 2021.1',
  fallback: ['gi-docgen', 'dummy_dep'],
  native: true,
  required: get_option('doc')
)

gidocgen = find_program('gi-docgen')

docs_dir = datadir / 'doc'

custom_target('libmanette-doc',
  input: [ libmanette_toml, libmanette_gir[0] ],
  output: libmanette_module,
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  depend_files: [ expand_content_md_files ],
  build_by_default: true,
  install: true,
  install_dir: docs_dir,
)

test('Validate docs',
  gidocgen,
  args: [
    'check',
    '--config', libmanette_toml,
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../src'),
    libmanette_gir[0],
  ],
  depends: libmanette_gir[0],
  suite: ['doc'],
)

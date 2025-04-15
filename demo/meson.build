manette_demo_resources = gnome.compile_resources(
  'manette-demo-resources',
  'manette-demo.gresources.xml',
  source_dir: meson.current_source_dir(),
  c_name: 'manette_demo',
)

manette_demo_sources = [
  'manette-demo.c',
  'manette-demo-axis-bar.c',
  'manette-demo-window.c',
  manette_demo_resources
]

manette_demo_dependencies = [
  libmanette_dep,
  dependency('libadwaita-1'),
]

executable(
  'manette-@0@-demo'.format(libmanette_api_version),
  manette_demo_sources,
  dependencies: manette_demo_dependencies,
  include_directories: [ root_inc, src_inc ],
  install: true
)

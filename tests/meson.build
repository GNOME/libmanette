installed_test_bindir = libexecdir / 'installed-tests' / libmanette_module

tests = [
  ['ManetteEventMapping', 'test-event-mapping'],
  ['ManetteMapping', 'test-mapping'],
  ['ManetteMappingManager', 'test-mapping-manager'],
]

foreach t : tests
  test_display_name = t.get(0)
  test_name = t.get(1)
  test_srcs = ['@0@.c'.format(test_name)]

  test_exe = executable(test_display_name, test_srcs,
    c_args: libmanette_c_args,
    dependencies: libmanette_internal_dep,
    install: get_option('install-tests'),
    install_dir: installed_test_bindir,
  )

  test('@0@ test'.format(test_display_name), test_exe)
endforeach

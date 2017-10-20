tests = [
  ['ManetteMapping', 'test-mapping'],
]

foreach t : tests
  test_display_name = t.get(0)
  test_name = t.get(1)
  test_srcs = ['@0@.c'.format(test_name)]

  test_exe = executable(test_display_name, test_srcs,
    c_args: libmanette_c_args,
    dependencies: libmanette_deps,
    link_with: libmanette_lib)

  test('@0@ test'.format(test_display_name), test_exe)
endforeach

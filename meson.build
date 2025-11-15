project('mys3-client', 'c',
  version : '0.1.0',
  default_options : ['c_std=c17', 'warning_level=2'])

gnome = import('gnome')
cc = meson.get_compiler('c')

# UI files
compiled_resources = gnome.compile_resources('resources',
  'res/mys3-client.gresource.xml',
  gresource_bundle: true,
)

# Manually find libs3
libs3_dep = cc.find_library('s3', dirs: ['/usr/lib'])

# Manually find our custom-built Scintilla
scintilla_dep = cc.find_library('scintilla', dirs: ['/tmp/scintilla_build/bin'])
scintilla_inc = include_directories('/tmp/scintilla_build/gtk4', '/tmp/scintilla_build/include')


executable('mys3-client',
  'src/main.c',
  'src/settings.c',
  'src/s3_client.c',
  'src/credential_storage.c',
  compiled_resources,
  dependencies : [
    dependency('gtk4'),
    dependency('glib-2.0'),
    libs3_dep,
    scintilla_dep
  ],
  include_directories: [include_directories('/usr/include'), scintilla_inc],
  install : true)

# Internationalization
i18n = import('i18n')
i18n.gettext('mys3-client',
  preset: 'glib'
)

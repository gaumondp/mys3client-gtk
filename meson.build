project('mys3-client', 'c', 'cpp',
  version : '0.1.0',
  default_options : ['c_std=c17', 'warning_level=2'])

add_project_arguments('-DG_LOG_USE_STRUCTURED', language : 'c')

gnome = import('gnome')
cc = meson.get_compiler('c')

# UI files
compiled_resources = gnome.compile_resources('resources',
  'res/mys3-client.gresource.xml',
  gresource_bundle: true,
)

# AWS SDK for C++
cmake = import('cmake')
aws_sdk_opts = cmake.subproject_options()
aws_sdk_opts.add_cmake_defines({
  'BUILD_SHARED_LIBS': 'OFF',
  'MINIMIZE_SIZE': 'ON',
  'ENABLE_TESTING': 'OFF',
  'AUTORUN_UNIT_TESTS': 'OFF',
  'BUILD_ONLY': 's3;core'
})
aws_sdk_proj = cmake.subproject('aws-sdk-cpp', options: aws_sdk_opts)
aws_sdk_dep = aws_sdk_proj.dependency('aws-cpp-sdk-s3')

s3_wrapper_lib = static_library('s3_wrapper',
  'src/s3_client_cpp.cpp',
  dependencies: [aws_sdk_dep, dependency('glib-2.0')]
)
s3_wrapper_dep = declare_dependency(link_with: s3_wrapper_lib)

# Dependencies
deps = [
  dependency('gtk4'),
  dependency('gtksourceview-5'),
  dependency('glib-2.0'),
  s3_wrapper_dep
]

if host_machine.system() == 'darwin'
  deps += cc.find_library('Security', required : true)
elif host_machine.system() == 'windows'
  deps += cc.find_library('credui', required : true)
endif

executable('mys3-client',
  'src/main.c',
  'src/settings.c',
  'src/s3_client.c',
  'src/credential_storage.c',
  'src/logging.c',
  compiled_resources,
  dependencies : deps,
  install : true)

# Internationalization
i18n = import('i18n')
i18n.gettext('mys3-client',
  preset: 'glib'
)

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

# AWS SDK for C++ (Pre-built)
cpp = meson.get_compiler('cpp')
aws_sdk_lib_dir_abs = meson.current_source_dir() / 'scripts/dist/lib'
aws_sdk_include_dir_rel = 'scripts/dist/include'

if run_command('test', '-d', aws_sdk_lib_dir_abs).returncode() != 0
  error('Pre-built AWS SDK not found in "scripts/dist". Please run "scripts/build-aws-sdk.sh" first.')
endif

# Define dependencies for the AWS SDK static libraries
# The SDK's own dependencies (like curl, zlib) are expected to be found on the system.
# On macOS, the build script configures the SDK to link against system frameworks automatically.
aws_s3_lib = cpp.find_library('aws-cpp-sdk-s3', dirs: aws_sdk_lib_dir_abs)
aws_core_lib = cpp.find_library('aws-cpp-sdk-core', dirs: aws_sdk_lib_dir_abs)
aws_crt_lib = cpp.find_library('aws-crt-cpp', dirs: aws_sdk_lib_dir_abs)
aws_checksums_lib = cpp.find_library('aws-checksums', dirs: aws_sdk_lib_dir_abs)
aws_c_common_lib = cpp.find_library('aws-c-common', dirs: aws_sdk_lib_dir_abs)
aws_c_cal_lib = cpp.find_library('aws-c-cal', dirs: aws_sdk_lib_dir_abs)
aws_c_io_lib = cpp.find_library('aws-c-io', dirs: aws_sdk_lib_dir_abs)
aws_c_http_lib = cpp.find_library('aws-c-http', dirs: aws_sdk_lib_dir_abs)
aws_c_auth_lib = cpp.find_library('aws-c-auth', dirs: aws_sdk_lib_dir_abs)
aws_c_sdkutils_lib = cpp.find_library('aws-c-sdkutils', dirs: aws_sdk_lib_dir_abs)
aws_c_event_stream_lib = cpp.find_library('aws-c-event-stream', dirs: aws_sdk_lib_dir_abs)
aws_c_mqtt_lib = cpp.find_library('aws-c-mqtt', dirs: aws_sdk_lib_dir_abs)
aws_c_s3_lib = cpp.find_library('aws-c-s3', dirs: aws_sdk_lib_dir_abs)
aws_c_compression_lib = cpp.find_library('aws-c-compression', dirs: aws_sdk_lib_dir_abs)
s2n_lib = cpp.find_library('s2n', dirs: aws_sdk_lib_dir_abs)
curl_dep = dependency('libcurl', required: true)
crypto_dep = dependency('libcrypto', required: true)
zlib_dep = dependency('zlib', required: true)


aws_sdk_dep = declare_dependency(
  include_directories: include_directories(aws_sdk_include_dir_rel),
  dependencies: [
    aws_s3_lib, aws_core_lib, aws_crt_lib, aws_checksums_lib,
    aws_c_common_lib, aws_c_cal_lib, aws_c_io_lib, aws_c_http_lib,
    aws_c_auth_lib, aws_c_sdkutils_lib, aws_c_event_stream_lib,
    aws_c_mqtt_lib, aws_c_s3_lib, aws_c_compression_lib, s2n_lib,
    curl_dep, crypto_dep, zlib_dep
  ]
)

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

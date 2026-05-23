project(
  'voxkey',
  'cpp',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=2'
  ]
)

cpp = meson.get_compiler('cpp')

x11_dep = dependency('x11', required: true)
xtst_dep = dependency('xtst', required: true)
pulse_simple_dep = dependency('libpulse-simple', required: true)
pulse_dep = dependency('libpulse', required: true)

conf = configuration_data()
conf.set10('WHISPER_DICTATE_WITH_WHISPER', get_option('with_whisper'))
conf.set10('WHISPER_DICTATE_CUDA', get_option('cuda'))
conf.set_quoted('WHISPER_DICTATE_VERSION', meson.project_version())
conf.set_quoted('WHISPER_DICTATE_DEFAULT_MODEL', get_option('default_model'))
configure_file(
  output: 'config_build.h',
  configuration: conf
)

sources = [
  'src/main.cpp',
  'src/config.cpp',
  'src/audio_pulse.cpp',
  'src/hotkey_x11.cpp',
  'src/output_x11.cpp',
]

deps = [
  x11_dep,
  xtst_dep,
  pulse_simple_dep,
  pulse_dep,
]

inc_dirs = [
  include_directories('src'),
  include_directories(meson.current_build_dir()),
]

link_libs = []

if get_option('with_whisper')
  whisper_include_dir = get_option('whisper_include_dir')
  whisper_lib_dir = get_option('whisper_lib_dir')

  if whisper_include_dir == '' or whisper_lib_dir == ''
    error('with_whisper=true requires -Dwhisper_include_dir and -Dwhisper_lib_dir')
  endif

  inc_dirs += include_directories(whisper_include_dir)

  whisper_lib = cpp.find_library('whisper', dirs: [whisper_lib_dir], required: true)
  link_libs += whisper_lib

  ggml_lib = cpp.find_library('ggml', dirs: [whisper_lib_dir], required: false)
  ggml_base_lib = cpp.find_library('ggml-base', dirs: [whisper_lib_dir], required: false)
  ggml_cpu_lib = cpp.find_library('ggml-cpu', dirs: [whisper_lib_dir], required: false)
  ggml_cuda_lib = cpp.find_library('ggml-cuda', dirs: [whisper_lib_dir], required: get_option('cuda'))

  if ggml_lib.found()
    link_libs += ggml_lib
  endif
  if ggml_base_lib.found()
    link_libs += ggml_base_lib
  endif
  if ggml_cpu_lib.found()
    link_libs += ggml_cpu_lib
  endif
  if get_option('cuda') and ggml_cuda_lib.found()
    link_libs += ggml_cuda_lib
  endif

  sources += ['src/transcriber_whisper.cpp']
else
  sources += ['src/transcriber_stub.cpp']
endif

executable(
  'voxkey',
  sources,
  include_directories: inc_dirs,
  dependencies: deps,
  link_with: link_libs,
  install: true
)

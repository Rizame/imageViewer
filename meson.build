project('imageViewerGtk', 'cpp',
  version: '0.1',
  default_options: ['cpp_std=c++20'])

gtkmm_dep = dependency('gtkmm-4.0')
include_dir = include_directories('include')

# Auto-find all .cpp files
sources = files(
  'main.cpp',
  'src/PPMviewer.cpp',
  'src/viewer.cpp'# All .cpp files recursively in src
)

executable('imageViewerGtk', sources,
  dependencies: gtkmm_dep,
  include_directories: include_dir)

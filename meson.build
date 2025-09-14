project('tecla', 'c',
    version: '49.0',
    meson_version: '>=0.54',
    default_options: [
        'c_std=c99',
        'warning_level=2',
    ]
)

cc = meson.get_compiler('c')

cc_warning_flags = [
    '-Wno-unused-parameter',
]

add_project_arguments(
    cc.get_supported_arguments(cc_warning_flags),
    language: 'c'
)

config_h = configuration_data()
config_defines = [
  # package
  ['PACKAGE', meson.project_name()],
  ['PACKAGE_VERSION', meson.project_version()],
  ['VERSION', meson.project_version()],
  ['GETTEXT_PACKAGE', meson.project_name()],
  ['LOCALEDIR', get_option('prefix') / get_option('localedir')],
]

foreach define: config_defines
  config_h.set_quoted(define[0], define[1])
endforeach

configure_file(
  output: 'config.h',
  configuration: config_h
)

config_inc = include_directories('.')

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

gtk_dep = dependency('gtk4')
gtk_wayland_dep = dependency('gtk4-wayland', required: false)
wayland_dep = dependency('wayland-client', required: false)
adw_dep = dependency('libadwaita-1', version: '>=1.4')
xkbcommon_dep = dependency('xkbcommon')
libm_dep = cc.find_library('m')

subdir('data')
subdir('src')
subdir('po')

pkg.generate(
    name: 'tecla',
    description: 'Keyboard layout previewer',
    dataonly: true,
    variables: [
        'prefix=' + get_option('prefix'),
        'app=${prefix}' / get_option('bindir') / 'tecla',
    ]
)

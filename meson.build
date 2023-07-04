project('tecla', 'c',
    version: '45.alpha',
    meson_version: '>=0.53',
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

gnome = import('gnome')
i18n = import('i18n')

gtk_dep = dependency('gtk4')
gtk_wayland_dep = dependency('gtk4-wayland', required: false)
wayland_dep = dependency('wayland-client', required: false)
adw_dep = dependency('libadwaita-1', version: '>=1.4')
xkbcommon_dep = dependency('xkbcommon')
libm_dep = cc.find_library('m')

subdir('data')
subdir('src')
subdir('po')

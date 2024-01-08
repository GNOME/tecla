resource_data = files (
    'tecla-view.ui',
)

tecla_gresources = gnome.compile_resources('tecla-gresources',
    'tecla.gresource.xml',
    dependencies: resource_data,
)

source = [
    'tecla-application.c',
    'tecla-key.c',
    'tecla-keymap-observer.c',
    'tecla-model.c',
    'tecla-util.c',
    'tecla-view.c',
    'main.c',
    tecla_gresources,
]

config_h = configuration_data()
config_h.set_quoted('PACKAGE', meson.project_name())
config_h.set_quoted('VERSION', meson.project_version())

configure_file(
  output: 'config.h',
  configuration: config_h
)

tecla = executable('tecla',
    sources: source,
    dependencies: [gtk_dep, gtk_wayland_dep, wayland_dep, adw_dep, xkbcommon_dep, libm_dep],
    install: true,
    include_directories: [config_inc],
)

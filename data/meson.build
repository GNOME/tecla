desktop_filename = 'org.gnome.Tecla.desktop'
desktop_file = i18n.merge_file(
  input: desktop_filename + '.in',
  output: desktop_filename,
  po_dir: '../po',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / 'applications',
  type: 'desktop'
)

subdir('icons')

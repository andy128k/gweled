install_data (join_paths ('icons', 'scalable', meson.project_name() + '.svg'),
              install_dir: join_paths (data_dir, 'icons', 'hicolor', 'scalable', 'apps'))


install_data (application_id + '.gschema.xml',
              install_dir: join_paths (data_dir, 'glib-2.0', 'schemas'))


# Desktop files
desktop_file = i18n.merge_file(
  type: 'desktop',
  input: meson.project_name() + '.desktop.in',
  output: meson.project_name() + '.desktop',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(data_dir, 'applications')
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif


# Appdata files
appstream_file = i18n.merge_file(
  input: application_id + '.appdata.xml.in',
  output: application_id + '.appdata.xml',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(data_dir, 'metainfo')
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util,
    args: ['validate-relax', appstream_file]
  )
endif

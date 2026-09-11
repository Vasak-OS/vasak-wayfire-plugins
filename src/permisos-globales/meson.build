# La lógica va en su propia biblioteca estática para que las pruebas la puedan
# enlazar sin arrastrar Wayfire: correr las pruebas no necesita un compositor.
vigilancia = static_library('vigilancia',
    'vigilancia.cpp',
    install: false)

vigilancia_dep = declare_dependency(
    link_with: vigilancia,
    include_directories: include_directories('.'))

shared_module('permisos-globales',
    'plugin.cpp',
    dependencies: [wayfire, wlserver, vigilancia_dep],
    install: true,
    install_dir: plugindir)

install_data('permisos-globales.xml', install_dir: metadatadir)

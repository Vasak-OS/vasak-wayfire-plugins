project('vasak-wayfire-plugins', 'cpp',
    version: '0.2.0',
    license: 'GPL-3.0-or-later',
    meson_version: '>=0.60.0',
    default_options: ['cpp_std=c++20', 'warning_level=2'])

wayfire = dependency('wayfire')
wlserver = dependency('wayland-server')

# Adónde van los plugins y sus metadatos lo dice el propio Wayfire. Fijarlos a
# mano rompe el día que Wayfire cambie de prefijo, y calla: el plugin queda
# instalado donde nadie lo busca y el compositor arranca sin él.
plugindir = wayfire.get_variable(pkgconfig: 'plugindir')
metadatadir = wayfire.get_variable(pkgconfig: 'metadatadir')

subdir('src/permisos-globales')
subdir('pruebas')

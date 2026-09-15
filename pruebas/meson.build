pruebas = executable('pruebas-vigilancia',
    'vigilancia.cpp',
    dependencies: vigilancia_dep,
    install: false)

test('vigilancia', pruebas)

# El nivel y la palabra del registro del filtro. No lo ve el compilador: `LOGI`
# y `LOGD` compilan igual, y la diferencia son cientos de líneas por arranque.
test('el-registro-no-grita',
    find_program('el-registro-no-grita.sh'))

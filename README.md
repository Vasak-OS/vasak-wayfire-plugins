# vasak-wayfire-plugins

Plugins de Wayfire de VasakOS.

## `permisos-globales`

Anota qué programa le pide al compositor los protocolos de Wayland que **ven o gobiernan la sesión**. Por ahora sólo anota: no niega nada.

### Por qué

La captura de pantalla por el portal ya pregunta. La directa no: cualquier cliente de Wayland puede pedirle los protocolos privilegiados al compositor, y el compositor se los da. Medido en una sesión real, un proceso sin ningún privilegio sacó una captura de 313 899 bytes con `grim` y leyó 84 822 bytes del portapapeles. Mientras eso siga así, la pregunta del portal es una formalidad que se saltea quien quiere.

Lo que está expuesto es más que la captura:

| protocolo | para qué sirve |
|---|---|
| `zwlr_screencopy_manager_v1` | capturar la pantalla |
| `ext_image_copy_capture_manager_v1` | ídem, el reemplazo estándar |
| `ext_output_image_capture_source_manager_v1` | elegir qué pantalla capturar |
| `zwlr_export_dmabuf_manager_v1` | exportar el framebuffer |
| `zwlr_data_control_manager_v1` | leer el portapapeles, continuo y sin foco |
| `ext_data_control_manager_v1` | ídem, el reemplazo estándar |
| `zwlr_foreign_toplevel_manager_v1` | enumerar ventanas y títulos |
| `ext_foreign_toplevel_list_v1` | ídem, el reemplazo estándar |
| `zwp_virtual_keyboard_manager_v1` | escribir teclas en cualquier ventana |
| `zwlr_virtual_pointer_manager_v1` | mover el puntero y hacer clic |
| `zwp_keyboard_shortcuts_inhibit_manager_v1` | quedarse con los atajos del escritorio |
| `zwp_primary_selection_device_manager_v1` | leer la selección del medio |

Y aparte, los que no leen nada pero deciden qué se ve: `ext_session_lock_manager_v1`, `zwlr_layer_shell_v1`, `zwlr_output_manager_v1`, `zwlr_output_power_manager_v1`, `zwlr_gamma_control_manager_v1`, `zwf_shell_manager_v2`.

La lista se comprobó enumerando los globals de una sesión de verdad con `wayland-info`, y cubre entera la de `privileged_protocols` que `vasak-desktop-settings` configura para `security-context-v1`. Son dos listas de lo mismo en dos repositorios, o sea dos que se pueden separar: si acá faltara uno que allá se oculta, la semana de medición no vería quién lo pide y la lista de permitidos se decidiría sin ese dato. Hay una prueba que avisa si esta lista se achica.

### Por qué todavía no niega

Porque no se sabe quién los usa. Negar primero y ver qué se rompe deja el escritorio sin poder dibujarse —el panel, la pantalla de bloqueo, los carteles y `vasak-shot` están del otro lado de esta misma puerta— y el problema aparece cuando ya nadie puede leer el error.

Con una semana de sesiones normales anotadas se sabe qué programas piden qué, y recién entonces se decide la lista de permitidos.

### Por qué no `security-context-v1`

Wayfire lo trae y VasakOS ya lo tiene activado. No alcanza, y el motivo es la regla del escritorio: **todo lo que se bloquea se tiene que poder desbloquear**.

`security-context-v1` es binario y global. Un cliente entra por un socket «en caja» o por el normal, y eso se decide **al arrancarlo**. No hay forma de devolverle la captura a una aplicación después de negársela sin relanzarla con otro entorno — o sea, bloquearía sin poder desbloquear.

El filtro de globals, en cambio, decide **por cliente y en el momento**: se saca el pid del `wl_client`, y de ahí el binario, que es exactamente lo que el servicio de permisos ya hace para identificar a quien llama. Una decisión revocada vuelve a aplicar sin relanzar nada.

### Por qué no `wl_display_set_global_filter`

Es la función que la documentación de Wayland nombra, y es la que **no** hay que usar acá. Wayfire ya la llama para implementar `security-context-v1`, y no encadena: el segundo que la llama reemplaza al primero. Usarla desde un plugin desactivaría en silencio el filtrado que `vasak-desktop-settings` configura hoy en `wayfire.ini`.

Wayfire 0.11 expone la forma correcta: `wf::compositor_core_t::create_global_filter()`. La documentación del encabezado lo dice explícitamente — con varios filtros, un global se ofrece a un cliente **si todos devuelven `true`**.

### Qué escribe

Una línea por cada combinación nueva de programa y protocolo, en el registro de Wayfire:

```
II ... - [permisos-globales] /usr/bin/grim pide zwlr_screencopy_manager_v1 (capturar la pantalla) [espia]
```

Una sola vez por par. El filtro se llama por cada global y por cada cliente que enumera el registro —decenas de veces cada vez que arranca un programa—, así que anotar todo taparía el resto del registro del escritorio.

Para leerlo:

```bash
journalctl --user -b | grep permisos-globales
```

### Activarlo

En `~/.config/wayfire.ini`, agregar `permisos-globales` a la lista de `plugins`.

## Compilar

```bash
meson setup build
ninja -C build
meson test -C build
```

Las pruebas no necesitan un compositor: la lógica que se puede decidir sin Wayland vive aparte, en `vigilancia.cpp`, y el enganche con Wayfire no decide nada.

## Licencia

GPL-3.0-or-later.

# vasak-wayfire-plugins

Plugins de Wayfire de VasakOS.

## `permisos-globales`

Los protocolos de Wayland que **ven o gobiernan la sesión** van sólo a los programas del escritorio que los necesitan. Al resto no se le ofrecen, así que no los puede pedir.

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

### De dónde sale la lista

De **lo que el escritorio necesita**, leído en el código de cada componente. No de medir quién lo pide.

El plan era medir una semana y decidir con esos datos. Queda escrito por qué no sirve, para que nadie lo reintente: el filtro corre cuando el compositor **ofrece** el protocolo, no cuando el cliente lo usa. La cabecera de Wayfire lo dice textual —«select which globals are *advertised* to clients»— y todo cliente de Wayland enumera el registro entero al arrancar. Medido en una sesión real, `grim` figuraba pidiendo 17 de los 18 protocolos vigilados, incluidos la pantalla de bloqueo, el teclado virtual y el control de gamma, que no usa. Una semana de eso da «todos los programas, todos los protocolos», que no decide nada. No hay enganche en el *bind* en esa API.

| programa | qué le damos | por qué |
|---|---|---|
| `vasak-desktop` | `layer_shell` | el panel y el escritorio se dibujan encima |
| `vasak-flare-daemon` | `layer_shell` | los carteles |
| `vasak-shot` | `layer_shell`, captura | la superficie de selección tapa todo, y los píxeles los toma él |
| `slurp` | `layer_shell` | la superficie de selección tapa todo |
| `vasak-session-manager`, `gtklock` | `session_lock` | la pantalla de bloqueo |
| `vasak-press-and-hold` | `layer_shell`, `virtual_keyboard` | el selector se dibuja encima y escribe el carácter |
| `xdg-desktop-portal-wlr` | captura (los cuatro) | el camino con pregunta de por medio |
| `wl-copy`, `wl-paste` | `data_control`, selección primaria | copiar y pegar |
| `wlsunset` | `gamma_control` | luz nocturna |
| `wlopm` | `output_power` | apagar la pantalla |
| `wlr-randr` | `output_manager` | configurar monitores |

**Acota, no habilita.** Estar en la lista no le da nada a nadie: le deja pedir lo que ya sabíamos que usa. Por eso cada fila nombra sus protocolos y no «todos».

**Y la lista es por ejecutable, que es su límite.** `grim` estuvo acá mientras `vasak-shot` lo llamaba para tomar los píxeles, y eso lo convertía en el intermediario de cualquiera: un guion de dos líneas —`grim "$1"`— capturaba la pantalla entera sin estar permitido. Medido en una sesión de verdad. Ahora `vasak-shot` habla el protocolo por su cuenta y `grim` salió de la lista, así que capturar a mano desde una terminal dejó de andar: es el precio de que el permiso signifique algo. Para el equipo de alguien que lo necesite está `permitidos_extra`.

El mismo razonamiento vale para las demás filas: cada programa de la lista que alguien más pueda ejecutar es un intermediario posible para lo que esa fila le concede.

**Lo que no le damos a nadie**: enumerar ventanas y títulos (`foreign_toplevel`, ambos), puntero virtual, inhibir atajos y `zwf_shell`. Ningún componente del escritorio los usa — el panel lista ventanas por el socket IPC de Wayfire, y `vasak-monitor` las cuenta con `ss -x`.

### Si esto rompe algo

`solo_anotar = true` en `wayfire.ini` y el escritorio vuelve a como estaba, sin reiniciar la sesión: Wayfire relee su configuración sola. El registro dice qué se negó y a quién, que es con lo que se arregla la lista.

Para permitir algo puntual sin tocar el código:

```ini
[permisos-globales]
permitidos_extra = /usr/bin/obs
```

Un programa de terceros que quiera la pantalla tiene además el portal, que pregunta. Sin alguna de esas dos salidas esto sería `security-context-v1` otra vez: bloquear sin poder desbloquear.

### Por qué no `security-context-v1`

Wayfire lo trae y VasakOS ya lo tiene activado. No alcanza, y el motivo es la regla del escritorio: **todo lo que se bloquea se tiene que poder desbloquear**.

`security-context-v1` es binario y global. Un cliente entra por un socket «en caja» o por el normal, y eso se decide **al arrancarlo**. No hay forma de devolverle la captura a una aplicación después de negársela sin relanzarla con otro entorno — o sea, bloquearía sin poder desbloquear.

El filtro de globals, en cambio, decide **por cliente y en el momento**: se saca el pid del `wl_client`, y de ahí el binario, que es exactamente lo que el servicio de permisos ya hace para identificar a quien llama. Una decisión revocada vuelve a aplicar sin relanzar nada.

### Por qué no `wl_display_set_global_filter`

Es la función que la documentación de Wayland nombra, y es la que **no** hay que usar acá. Wayfire ya la llama para implementar `security-context-v1`, y no encadena: el segundo que la llama reemplaza al primero. Usarla desde un plugin desactivaría en silencio el filtrado que `vasak-desktop-settings` configura hoy en `wayfire.ini`.

Wayfire 0.11 expone la forma correcta: `wf::compositor_core_t::create_global_filter()`. La documentación del encabezado lo dice explícitamente — con varios filtros, un global se ofrece a un cliente **si todos devuelven `true`**.

### Qué escribe

Una línea por cada combinación nueva de programa y protocolo **negada**, en el registro de Wayfire:

```
II ... - [permisos-globales] negado a /usr/bin/cualquiera: zwlr_screencopy_manager_v1 (capturar la pantalla)
```

Sólo lo negado, y una sola vez por par. Lo permitido es lo normal, y anotarlo taparía esto —que es lo que alguien busca cuando algo dejó de funcionar—. El filtro se llama por cada global y por cada cliente que enumera el registro, decenas de veces por programa que arranca.

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

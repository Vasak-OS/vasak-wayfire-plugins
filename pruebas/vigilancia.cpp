// Pruebas de lo que se puede decidir sin un compositor.
//
// No hay marco de pruebas a propósito: son comprobaciones sobre funciones puras
// y un binario de cincuenta líneas las corre sin agregarle una dependencia al
// paquete de un plugin del compositor.
#include "vigilancia.hpp"

#include <cstdio>
#include <unistd.h>

namespace
{
int fallos = 0;

void comprobar(bool condicion, const char *que)
{
    if (condicion)
    {
        std::printf("  ok   %s\n", que);
    } else
    {
        std::printf("  MAL  %s\n", que);
        ++fallos;
    }
}
} // namespace

int main()
{
    using vasak::Gravedad;

    std::printf("Los protocolos que miran quedan como los que miran\n");
    comprobar(vasak::gravedad_de("zwlr_screencopy_manager_v1") == Gravedad::ESPIA,
        "capturar la pantalla");
    comprobar(vasak::gravedad_de("ext_data_control_manager_v1") == Gravedad::ESPIA,
        "leer el portapapeles");
    comprobar(vasak::gravedad_de("zwp_virtual_keyboard_manager_v1") == Gravedad::ESPIA,
        "escribir teclas");

    std::printf("\nLos que mandan sobre la sesión van aparte\n");
    comprobar(vasak::gravedad_de("ext_session_lock_manager_v1") == Gravedad::GOBIERNA,
        "ser la pantalla de bloqueo");
    comprobar(vasak::gravedad_de("zwlr_layer_shell_v1") == Gravedad::GOBIERNA,
        "dibujar por encima de todo");

    std::printf("\nLo corriente no se anota\n");
    // Si `wl_compositor` entrara en la lista, el registro tendría una línea por
    // cada programa que abre una ventana, o sea todos.
    comprobar(vasak::gravedad_de("wl_compositor") == Gravedad::NINGUNA, "wl_compositor");
    comprobar(vasak::gravedad_de("xdg_wm_base") == Gravedad::NINGUNA, "xdg_wm_base");
    comprobar(vasak::gravedad_de("") == Gravedad::NINGUNA, "una cadena vacía");

    std::printf("\nCada par zwlr_/ext_ está completo\n");
    // Es el error que ya se encontró en la lista por omisión de Wayfire:
    // nombra los `zwlr_` y no sus reemplazos, así que oculta el protocolo viejo
    // y deja pasar el nuevo, que hace exactamente lo mismo.
    const char *pares[][2] = {
        {"zwlr_data_control_manager_v1", "ext_data_control_manager_v1"},
        {"zwlr_foreign_toplevel_manager_v1", "ext_foreign_toplevel_list_v1"},
        {"zwlr_screencopy_manager_v1", "ext_image_copy_capture_manager_v1"},
    };
    for (const auto& par : pares)
    {
        comprobar(vasak::gravedad_de(par[0]) != Gravedad::NINGUNA
            && vasak::gravedad_de(par[1]) != Gravedad::NINGUNA, par[1]);
    }

    std::printf("\nSe cubre todo lo que security-context-v1 oculta\n");
    // Copia de `privileged_protocols` de
    // `vasak-desktop-settings/etc/skel/.config/wayfire.ini`. Son dos listas de
    // lo mismo en dos repositorios: si allá se oculta un protocolo y acá no se
    // anota, la semana de medición no ve quién lo pide y la lista de permitidos
    // se decide sin ese dato. Esta prueba no puede leer el otro repositorio,
    // así que al menos avisa cuando esta lista se achica.
    for (const char *protocolo :
        {"zwlr_screencopy_manager_v1", "ext_image_copy_capture_manager_v1",
         "ext_output_image_capture_source_manager_v1", "zwlr_export_dmabuf_manager_v1",
         "zwlr_data_control_manager_v1", "ext_data_control_manager_v1",
         "zwp_primary_selection_device_manager_v1", "zwlr_virtual_pointer_manager_v1",
         "zwp_virtual_keyboard_manager_v1", "zwp_keyboard_shortcuts_inhibit_manager_v1",
         "ext_session_lock_manager_v1", "zwlr_layer_shell_v1",
         "zwlr_foreign_toplevel_manager_v1", "zwlr_output_manager_v1",
         "zwlr_output_power_manager_v1", "zwlr_gamma_control_manager_v1",
         "zwf_shell_manager_v2"})
    {
        comprobar(vasak::gravedad_de(protocolo) != Gravedad::NINGUNA, protocolo);
    }

    std::printf("\nTodo lo que se anota dice para qué sirve\n");
    // Un registro que dice el nombre del protocolo y nada más obliga a buscarlo
    // afuera, que es justo lo que no va a pasar dentro de seis meses.
    for (const char *protocolo :
        {"zwlr_screencopy_manager_v1", "ext_data_control_manager_v1",
         "zwlr_layer_shell_v1", "zwlr_gamma_control_manager_v1"})
    {
        comprobar(vasak::para_que_sirve(protocolo)[0] != '\0', protocolo);
    }
    comprobar(vasak::para_que_sirve("wl_compositor")[0] == '\0',
        "y lo que no está en la lista no inventa una explicación");

    std::printf("\nLo mismo no se anota dos veces\n");
    // El filtro se llama decenas de veces por programa que arranca. Sin esto,
    // el registro del escritorio queda tapado por la misma línea repetida.
    vasak::Memoria memoria;
    comprobar(memoria.es_nuevo("/usr/bin/grim", "zwlr_screencopy_manager_v1"),
        "la primera vez sí");
    comprobar(!memoria.es_nuevo("/usr/bin/grim", "zwlr_screencopy_manager_v1"),
        "la segunda no");
    comprobar(memoria.es_nuevo("/usr/bin/grim", "ext_data_control_manager_v1"),
        "otro protocolo del mismo programa sí");
    comprobar(memoria.es_nuevo("/usr/bin/obs", "zwlr_screencopy_manager_v1"),
        "el mismo protocolo de otro programa sí");
    comprobar(memoria.cuantos() == 3, "y se contaron tres");

    std::printf("\nEl binario de un pid\n");
    comprobar(vasak::binario_de(::getpid()).find('/') == 0,
        "el de este proceso es una ruta absoluta");
    // Un pid que no existe tiene que dejar algo con qué buscar, no una cadena
    // vacía: «pid 0 (no se pudo leer)» se puede rastrear y «» no.
    comprobar(vasak::binario_de(0).find("sin pid") != std::string::npos,
        "el pid cero se dice, no se calla");
    comprobar(vasak::binario_de(-1).find("sin pid") != std::string::npos,
        "un pid negativo también");

    std::printf("\nQuién puede pedir qué\n");
    // El escritorio tiene que poder dibujarse. Cada una de estas cuatro es una
    // pieza que, negada, deja la sesión rota de una forma distinta: sin panel,
    // sin carteles, sin pantalla de bloqueo, sin poder sacar una captura.
    comprobar(vasak::decidir("/usr/bin/vasak-desktop", "zwlr_layer_shell_v1") ==
        vasak::Decision::PERMITIR, "el panel puede dibujar encima de todo");
    comprobar(vasak::decidir("/usr/bin/vasak-flare-daemon", "zwlr_layer_shell_v1") ==
        vasak::Decision::PERMITIR, "los carteles también");
    // El binario, no el paquete: `vasak-session-manager` publica tres, y el
    // que bloquea es éste. Escrito como prueba porque la lista decía el
    // nombre del gestor de sesión y el bloqueo no se tomaba —y sin bloqueo la
    // sesión sigue a la vista al suspender, que es peor que no tener plugin.
    comprobar(vasak::decidir("/usr/bin/vasak-lock-screen", "ext_session_lock_manager_v1") ==
        vasak::Decision::PERMITIR, "la pantalla de bloqueo puede bloquear");
    comprobar(vasak::decidir("/usr/bin/vasak-session-manager", "ext_session_lock_manager_v1") ==
        vasak::Decision::NEGAR, "el greeter no bloquea: corre contra otro compositor");
    comprobar(vasak::decidir("/usr/bin/grim", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::PERMITIR, "grim puede capturar");

    // El portal es el camino por el que un programa de terceros pide la
    // pantalla con una pregunta de por medio. Negárselo rompe todo compartir
    // pantalla, que es lo contrario de lo que este plugin busca.
    comprobar(vasak::decidir("/usr/lib/xdg-desktop-portal-wlr", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::PERMITIR, "el portal puede capturar");

    comprobar(vasak::decidir("/usr/bin/wl-copy", "zwlr_data_control_manager_v1") ==
        vasak::Decision::PERMITIR, "el portapapeles se puede copiar");
    comprobar(vasak::decidir("/usr/bin/wlsunset", "zwlr_gamma_control_manager_v1") ==
        vasak::Decision::PERMITIR, "la luz nocturna puede cambiar el gamma");

    // Xwayland llega con el pid de Wayfire, porque el compositor le arma el
    // socket. Negarle deja a todas las aplicaciones X11 sin portapapeles.
    comprobar(vasak::es_plomeria_del_compositor(4321, 4321),
        "lo que armó el compositor pasa");
    comprobar(!vasak::es_plomeria_del_compositor(4322, 4321),
        "y un pid ajeno no, aunque corra el mismo binario");

    std::printf("\nY quién no\n");
    // Lo que este plugin viene a tapar: cualquier programa sacaba una captura y
    // leía el portapapeles sin pedirle permiso a nadie.
    comprobar(vasak::decidir("/usr/bin/cualquiera", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::NEGAR, "un programa cualquiera no captura la pantalla");
    comprobar(vasak::decidir("/usr/bin/cualquiera", "ext_data_control_manager_v1") ==
        vasak::Decision::NEGAR, "ni lee el portapapeles");
    comprobar(vasak::decidir("/usr/bin/cualquiera", "zwp_virtual_keyboard_manager_v1") ==
        vasak::Decision::NEGAR, "ni escribe teclas");

    // Estar en la lista no es ser de confianza para todo: cada fila dice lo que
    // ese programa usa. Sin esto, `grim` —que cualquiera puede ejecutar— sería
    // un pase libre a leer el portapapeles.
    comprobar(vasak::decidir("/usr/bin/grim", "ext_data_control_manager_v1") ==
        vasak::Decision::NEGAR, "grim captura pero no lee el portapapeles");
    comprobar(vasak::decidir("/usr/bin/vasak-desktop", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::NEGAR, "el panel dibuja pero no captura");

    // Nadie lo pide en todo el escritorio, así que nadie lo tiene.
    comprobar(vasak::decidir("/usr/bin/vasak-desktop", "zwlr_foreign_toplevel_manager_v1") ==
        vasak::Decision::NEGAR, "enumerar ventanas no se lo damos a nadie");

    // Lo que no está vigilado tiene que pasar: son casi todos los globals de
    // Wayland, y negarlos deja al escritorio sin poder abrir una ventana.
    comprobar(vasak::decidir("/usr/bin/cualquiera", "wl_compositor") ==
        vasak::Decision::PERMITIR, "los protocolos normales pasan");
    comprobar(vasak::decidir("/usr/bin/cualquiera", "xdg_wm_base") ==
        vasak::Decision::PERMITIR, "abrir una ventana también");

    // Una ruta parecida no alcanza: el límite es la ruta absoluta, y escribir
    // en /usr/bin pide root.
    comprobar(vasak::decidir("/home/quien/grim", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::NEGAR, "una copia de grim en el home no hereda nada");
    comprobar(vasak::decidir("/usr/bin/grim-falso", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::NEGAR, "ni un nombre parecido");

    std::printf("\nLa vía para desbloquear\n");
    // Sin esto, un error en la lista se arregla recompilando, y eso no puede
    // pasar en algo que puede dejar la sesión sin pantalla de bloqueo.
    vasak::fijar_permitidos_extra({"/usr/bin/obs"});
    comprobar(vasak::decidir("/usr/bin/obs", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::PERMITIR, "lo agregado por configuración pasa");
    comprobar(vasak::decidir("/usr/bin/otro", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::NEGAR, "y sólo eso");
    vasak::fijar_permitidos_extra({});
    comprobar(vasak::decidir("/usr/bin/obs", "zwlr_screencopy_manager_v1") ==
        vasak::Decision::NEGAR, "al sacarlo vuelve a negarse");

    std::printf("\n%s\n", fallos == 0 ? "Todo bien." : "Hay fallos.");
    return fallos == 0 ? 0 : 1;
}

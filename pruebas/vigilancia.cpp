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

    std::printf("\n%s\n", fallos == 0 ? "Todo bien." : "Hay fallos.");
    return fallos == 0 ? 0 : 1;
}

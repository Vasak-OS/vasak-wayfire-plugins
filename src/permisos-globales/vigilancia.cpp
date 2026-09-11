#include "vigilancia.hpp"

#include <array>
#include <cstdio>
#include <unistd.h>

namespace vasak
{
namespace
{
struct Entrada
{
    const char *protocolo;
    Gravedad gravedad;
    const char *para_que;
};

// La lista salió de enumerar los globals de una sesión real con `wayland-info`
// y de mirar qué hace cada uno, no de copiar la de otro proyecto.
//
// Los dos pares `zwlr_`/`ext_` están completos a propósito. La lista por
// omisión de Wayfire para `security-context-v1` nombra sólo los `zwlr_`, así
// que oculta el protocolo viejo y deja pasar el nuevo, que hace lo mismo.
//
// `ext_output_image_capture_source_manager_v1` estaba expuesto y no figuraba en
// ninguna lista: es la mitad que dice *qué* capturar del protocolo de captura
// nuevo. Sin él, `ext_image_copy_capture_manager_v1` no tiene de dónde sacar la
// imagen de una pantalla.
constexpr std::array<Entrada, 16> CONOCIDOS{{
    // Ver lo que la persona hace.
    {"zwlr_screencopy_manager_v1", Gravedad::ESPIA, "capturar la pantalla"},
    {"ext_image_copy_capture_manager_v1", Gravedad::ESPIA, "capturar la pantalla (reemplazo estándar)"},
    {"ext_output_image_capture_source_manager_v1", Gravedad::ESPIA, "elegir qué pantalla capturar"},
    {"zwlr_export_dmabuf_manager_v1", Gravedad::ESPIA, "exportar el framebuffer"},
    {"zwlr_data_control_manager_v1", Gravedad::ESPIA, "leer el portapapeles sin foco"},
    {"ext_data_control_manager_v1", Gravedad::ESPIA, "leer el portapapeles sin foco (reemplazo estándar)"},
    {"zwlr_foreign_toplevel_manager_v1", Gravedad::ESPIA, "enumerar ventanas y títulos"},
    {"ext_foreign_toplevel_list_v1", Gravedad::ESPIA, "enumerar ventanas y títulos (reemplazo estándar)"},

    // Escribir en lugar de la persona.
    {"zwp_virtual_keyboard_manager_v1", Gravedad::ESPIA, "escribir teclas en cualquier ventana"},
    {"zwlr_virtual_pointer_manager_v1", Gravedad::ESPIA, "mover el puntero y hacer clic"},
    {"zwp_keyboard_shortcuts_inhibit_manager_v1", Gravedad::ESPIA, "quedarse con los atajos del escritorio"},

    // Mandar sobre la sesión. No leen nada, pero deciden qué se ve.
    {"ext_session_lock_manager_v1", Gravedad::GOBIERNA, "ser la pantalla de bloqueo"},
    {"zwlr_layer_shell_v1", Gravedad::GOBIERNA, "dibujar por encima de todo"},
    {"zwlr_output_manager_v1", Gravedad::GOBIERNA, "reconfigurar las pantallas"},
    {"zwlr_output_power_manager_v1", Gravedad::GOBIERNA, "apagar y encender las pantallas"},
    {"zwlr_gamma_control_manager_v1", Gravedad::GOBIERNA, "cambiar el color de las pantallas"},
}};

const Entrada *buscar(const std::string& protocolo)
{
    for (const auto& entrada : CONOCIDOS)
    {
        if (protocolo == entrada.protocolo)
        {
            return &entrada;
        }
    }

    return nullptr;
}
} // namespace

Gravedad gravedad_de(const std::string& protocolo)
{
    const Entrada *entrada = buscar(protocolo);
    return entrada ? entrada->gravedad : Gravedad::NINGUNA;
}

const char *para_que_sirve(const std::string& protocolo)
{
    const Entrada *entrada = buscar(protocolo);
    return entrada ? entrada->para_que : "";
}

bool Memoria::es_nuevo(const std::string& binario, const std::string& protocolo)
{
    return vistos.emplace(binario, protocolo).second;
}

std::string binario_de(pid_t pid)
{
    if (pid <= 0)
    {
        return "(sin pid)";
    }

    // `readlink` y no `realpath`: el destino puede no existir —un programa
    // borrado o actualizado mientras corría— y `realpath` devolvería nada,
    // justo en el caso en que más interesa saber qué era.
    const std::string enlace = "/proc/" + std::to_string(pid) + "/exe";
    std::string destino(4096, '\0');

    const ssize_t largo = ::readlink(enlace.c_str(), destino.data(), destino.size() - 1);
    if (largo <= 0)
    {
        return "pid " + std::to_string(pid) + " (no se pudo leer)";
    }

    destino.resize(static_cast<std::size_t>(largo));
    return destino;
}
} // namespace vasak

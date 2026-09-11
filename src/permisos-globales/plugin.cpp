// Quién le pide al compositor los protocolos que ven o gobiernan la sesión.
//
// # Por qué existe
//
// La captura por el portal ya pregunta. La directa no: cualquier cliente de
// Wayland puede pedirle los protocolos privilegiados al compositor y el
// compositor se los da. Medido en una sesión real, un proceso sin ningún
// privilegio sacó una captura de 313 899 bytes con `grim` y leyó 84 822 bytes
// del portapapeles. Mientras eso siga así, la pregunta del portal es una
// formalidad que se saltea quien quiere.
//
// # Por qué este plugin todavía no niega nada
//
// Porque no se sabe quién los usa. Negar primero y ver qué se rompe deja el
// escritorio sin dibujarse —el panel, la pantalla de bloqueo, los carteles y
// `vasak-shot` están del otro lado de esta misma puerta— y el problema
// aparece cuando ya nadie puede leer el error. Así que esta primera versión
// **anota y deja pasar**: con una semana de sesiones normales se sabe qué
// programas piden qué, y recién entonces se decide la lista de permitidos.
//
// # Por qué no `wl_display_set_global_filter`
//
// Es lo que dice el issue y es lo que **no** hay que hacer. Wayfire ya instala
// uno —`/usr/bin/wayfire` importa ese símbolo—, que es con lo que implementa
// `security-context-v1`. Esa función no encadena: el segundo que la llama
// reemplaza al primero. Usarla acá desactivaría en silencio el filtrado que
// `vasak-desktop-settings` configura hoy en `wayfire.ini`, y nada avisaría.
//
// Wayfire 0.11 expone la forma correcta, y está pensada justo para esto:
// `wf::compositor_core_t::create_global_filter()` devuelve un filtro propio, y
// la documentación del encabezado dice que con varios filtros un global se
// ofrece **si todos devuelven `true`**. O sea que se componen, y que el día
// que este plugin empiece a negar, negar va a alcanzar.
#include "vigilancia.hpp"

#include <wayfire/core.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/util/log.hpp>

#include <wayland-server-core.h>

class permisos_globales_t : public wf::plugin_interface_t
{
  public:
    void init() override
    {
        filtro = wf::get_core().create_global_filter();
        filtro->set_filter([this] (const wl_client *cliente, const wl_global *global)
        {
            anotar(cliente, global);

            // Siempre. Esta versión mira y no decide.
            return true;
        });

        LOGI("[permisos-globales] anotando quién pide los protocolos privilegiados; "
             "esta versión no niega nada");
    }

    void fini() override
    {
        // El filtro se da de baja solo al destruirse, pero soltarlo acá y no en
        // el destructor deja claro cuándo deja de correr: Wayfire llama a
        // `fini()` antes de liberar el plugin, y entre esas dos cosas el filtro
        // no tiene que seguir llamando a un `this` que está por irse.
        filtro.reset();
        LOGI("[permisos-globales] fin; se anotaron ", memoria.cuantos(),
            " combinaciones de programa y protocolo");
    }

    bool is_unloadable() override
    {
        return true;
    }

  private:
    void anotar(const wl_client *cliente, const wl_global *global)
    {
        // Lo barato primero. Esto corre por cada global y por cada cliente que
        // enumera el registro —decenas de veces cada vez que arranca un
        // programa—, así que todo lo que no sea una comparación de cadenas
        // tiene que quedar detrás de esta guarda.
        const wl_interface *interfaz = wl_global_get_interface(global);
        if (!interfaz || !interfaz->name)
        {
            return;
        }

        const std::string protocolo = interfaz->name;
        const vasak::Gravedad gravedad = vasak::gravedad_de(protocolo);
        if (gravedad == vasak::Gravedad::NINGUNA)
        {
            return;
        }

        pid_t pid = 0;
        uid_t uid = 0;
        gid_t gid = 0;
        wl_client_get_credentials(cliente, &pid, &uid, &gid);

        const std::string binario = vasak::binario_de(pid);
        if (!memoria.es_nuevo(binario, protocolo))
        {
            return;
        }

        LOGI("[permisos-globales] ", binario, " pide ", protocolo,
            " (", vasak::para_que_sirve(protocolo), ")",
            gravedad == vasak::Gravedad::ESPIA ? " [espia]" : " [gobierna]");
    }

    std::unique_ptr<wf::wayland_global_filter_t> filtro;
    vasak::Memoria memoria;
};

DECLARE_WAYFIRE_PLUGIN(permisos_globales_t)

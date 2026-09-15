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
// # De dónde sale la lista de permitidos
//
// De lo que el escritorio necesita, leído en el código de cada componente.
//
// El plan anterior era medir una semana y decidir con esos datos. No sirve, y
// conviene que quede escrito para que nadie lo reintente: este filtro corre
// cuando el compositor **ofrece** el protocolo, no cuando el cliente lo usa
// —la cabecera de Wayfire dice «select which globals are advertised to
// clients»— y todo cliente de Wayland enumera el registro entero al arrancar.
// Medido en una sesión real, `grim` figuraba pidiendo 17 de los 18 protocolos
// vigilados, incluidos la pantalla de bloqueo y el teclado virtual, que no usa.
// Una semana de eso habría dado «todos los programas, todos los protocolos».
//
// # Si esto rompe algo
//
// `solo_anotar = true` en `wayfire.ini` y el escritorio vuelve a como estaba,
// sin reiniciar la sesión: Wayfire relee su configuración sola. El registro dice
// qué se negó y a quién, que es con lo que se arregla la lista.
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
#include <wayfire/option-wrapper.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/util/log.hpp>

#include <set>
#include <string>

#include <unistd.h>

#include <wayland-server-core.h>

class permisos_globales_t : public wf::plugin_interface_t
{
  public:
    void init() override
    {
        vasak::fijar_permitidos_extra(leer_permitidos_extra());

        filtro = wf::get_core().create_global_filter();
        filtro->set_filter([this] (const wl_client *cliente, const wl_global *global)
        {
            return decidir(cliente, global);
        });

        if (solo_anotar)
        {
            LOGI("[permisos-globales] SOLO ANOTANDO: se registra lo que se dejaría de "
                 "ofrecer, pero se ofrece todo igual. El detalle va en nivel debug. "
                 "Quitá `solo_anotar` de wayfire.ini para que rija.");
        } else
        {
            LOGI("[permisos-globales] los protocolos privilegiados van sólo a los "
                 "programas del escritorio. El detalle de qué no se le ofrece a quién "
                 "va en nivel debug (`WAYFIRE_DEBUG=1` o `core/debug`); si algo dejó de "
                 "andar, ahí está, y `solo_anotar = true` lo devuelve todo");
        }
    }

    void fini() override
    {
        // El filtro se da de baja solo al destruirse, pero soltarlo acá y no en
        // el destructor deja claro cuándo deja de correr: Wayfire llama a
        // `fini()` antes de liberar el plugin, y entre esas dos cosas el filtro
        // no tiene que seguir llamando a un `this` que está por irse.
        filtro.reset();
        // El único número de todo esto que va en info, y por eso se queda: dice
        // de un vistazo si el filtro estuvo haciendo algo, sin las cientos de
        // líneas del detalle.
        LOGI("[permisos-globales] fin; no se ofrecieron ", memoria.cuantos(),
            " combinaciones de programa y protocolo");
    }

    bool is_unloadable() override
    {
        return true;
    }

  private:
    /**
     * Lee de la configuración los binarios que se agregan a la lista.
     *
     * Rutas absolutas separadas por comas. Es la vía para desbloquear algo que
     * la lista compilada no previó sin tener que recompilar el plugin.
     */
    std::set<std::string> leer_permitidos_extra() const
    {
        std::set<std::string> binarios;
        const std::string crudo = permitidos_extra;

        std::size_t desde = 0;
        while (desde <= crudo.size())
        {
            const std::size_t coma = crudo.find(',', desde);
            const std::size_t hasta = (coma == std::string::npos) ? crudo.size() : coma;

            std::string ruta = crudo.substr(desde, hasta - desde);
            const std::size_t principio = ruta.find_first_not_of(" \t");
            const std::size_t final = ruta.find_last_not_of(" \t");
            if (principio != std::string::npos)
            {
                binarios.insert(ruta.substr(principio, final - principio + 1));
            }

            if (coma == std::string::npos)
            {
                break;
            }
            desde = coma + 1;
        }

        return binarios;
    }

    bool decidir(const wl_client *cliente, const wl_global *global)
    {
        const wl_interface *interfaz = wl_global_get_interface(global);
        if (!interfaz || !interfaz->name)
        {
            return true;
        }

        const std::string protocolo = interfaz->name;
        if (vasak::gravedad_de(protocolo) == vasak::Gravedad::NINGUNA)
        {
            // Lo barato primero: la inmensa mayoría de los globals son los
            // normales de Wayland y no hay que leer `/proc` por cada uno.
            return true;
        }

        pid_t pid = 0;
        uid_t uid = 0;
        gid_t gid = 0;
        wl_client_get_credentials(const_cast<wl_client*>(cliente), &pid, &uid, &gid);

        // Lo que armó el propio compositor —Xwayland, sobre todo— llega con el
        // pid de Wayfire. Ver `es_plomeria_del_compositor`: negarle acá sería
        // negarle a Xwayland, y con él a todas las aplicaciones X11.
        if (vasak::es_plomeria_del_compositor(pid, ::getpid()))
        {
            return true;
        }

        const std::string binario = vasak::binario_de(pid);
        const bool permitido = vasak::decidir(binario, protocolo) == vasak::Decision::PERMITIR;

        if (!permitido && memoria.es_nuevo(binario, protocolo))
        {
            // # Por qué «no se le ofrece» y no «negado»
            //
            // Esto es un filtro de **globals**: corre cuando un cliente enumera
            // el registro, no cuando pide algo. Nadie pidió nada — un cliente de
            // Wayland cualquiera enumera el registro entero al conectarse. Decir
            // «negado» describe un rechazo que no hubo, y manda a buscar un
            // fallo que no existe: pasó, `zwf_shell_manager_v2` «negado» a
            // `/usr/bin/vasak-desktop` parecía un permiso faltante cuando el
            // binario del escritorio ni siquiera nombra ese protocolo.
            //
            // # Por qué debug y no info
            //
            // Son dieciséis líneas por cliente que se conecta. Medido: **276 en
            // un solo arranque**, de diecisiete binarios. La idea era que quien
            // busca por qué algo dejó de funcionar lo encuentre acá, y con ese
            // volumen pasa lo contrario — tapa todo lo demás del arranque.
            //
            // En debug sigue estando entero, y la línea de `init()` dice cómo
            // verlo. Una línea por par, y sólo de lo que no se ofrece: lo
            // ofrecido es lo normal.
            LOGD("[permisos-globales] ",
                solo_anotar ? "no se le ofrecería a " : "no se le ofrece a ",
                binario, ": ", protocolo, " (", vasak::para_que_sirve(protocolo), ")");
        }

        return permitido || solo_anotar;
    }

    /**
     * Devuelve todo a como estaba, sin reiniciar la sesión.
     *
     * Es la salida cuando esta lista se equivoca y algo del escritorio deja de
     * funcionar: Wayfire relee su configuración sola, así que alcanza con
     * editar `wayfire.ini`. Sin esto, un error en la lista se arregla
     * recompilando, que es lo que no puede pasar en algo que puede dejar la
     * sesión sin pantalla de bloqueo.
     */
    wf::option_wrapper_t<bool> solo_anotar{"permisos-globales/solo_anotar"};

    /**
     * Binarios que se suman a la lista, con todo permitido.
     *
     * Rutas absolutas separadas por comas. Para lo que no pasa por el portal y
     * la lista compilada no previó.
     */
    wf::option_wrapper_t<std::string> permitidos_extra{"permisos-globales/permitidos_extra"};

    std::unique_ptr<wf::wayland_global_filter_t> filtro;
    vasak::Memoria memoria;
};

DECLARE_WAYFIRE_PLUGIN(permisos_globales_t)

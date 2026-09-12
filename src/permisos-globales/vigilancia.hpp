// Lo que se puede decidir sin un compositor.
//
// Está separado del enganche con Wayfire a propósito: acá adentro no hay ni un
// tipo de Wayland, así que se puede probar con un binario chico y sin sesión
// gráfica. La otra mitad —`plugin.cpp`— es pegamento y no decide nada.
#pragma once

#include <set>
#include <string>
#include <sys/types.h>
#include <utility>

namespace vasak
{
/**
 * Qué tan grave es que un cliente cualquiera tenga este protocolo.
 *
 * No están todos los protocolos privilegiados en la misma bolsa porque no son
 * el mismo problema, y meterlos juntos haría que la lista del día que se
 * empiece a negar se decida de una sola vez para cosas muy distintas.
 */
enum class Gravedad
{
    /** Ve o escribe lo que la persona hace: pantalla, portapapeles, teclado. */
    ESPIA,
    /** Cambia cómo se ve o se comporta la sesión, sin leer nada. */
    GOBIERNA,
    /** No es de esta lista. */
    NINGUNA,
};

/**
 * Si el protocolo merece anotarse, y por qué.
 *
 * La lista salió de enumerar los globals de una sesión de verdad con
 * `wayland-info`, no de una lista de otro proyecto: la de Wayfire para
 * `security-context-v1` nombra los `zwlr_` y no sus reemplazos `ext_`, así que
 * «protegía» dejando pasar lo nuevo.
 */
Gravedad gravedad_de(const std::string& protocolo);

/** Qué hace ese protocolo, en una línea, para que el registro se entienda. */
const char *para_que_sirve(const std::string& protocolo);

/**
 * Qué hacer con un pedido.
 */
enum class Decision
{
    /** Se ofrece el protocolo. */
    PERMITIR,
    /** No se ofrece: el cliente no lo ve en el registro y no lo puede pedir. */
    NEGAR,
};

/**
 * Si este binario puede usar este protocolo.
 *
 * # De dónde sale la lista
 *
 * De **lo que el escritorio necesita**, leído en el código de cada componente,
 * y no de medir quién lo pide. Medir no servía: el filtro del compositor corre
 * cuando el protocolo se **ofrece**, no cuando se usa —la propia cabecera de
 * Wayfire dice «select which globals are advertised to clients»— y todo cliente
 * de Wayland enumera el registro entero al arrancar. Medido en una sesión real,
 * `grim` figuraba pidiendo 17 de los 18 protocolos, incluidos la pantalla de
 * bloqueo y el teclado virtual, que no usa. Una semana de eso habría dado
 * «todos los programas, todos los protocolos».
 *
 * # Acota, no habilita
 *
 * Estar en la lista no le da nada a nadie: le deja pedir lo que ya sabíamos que
 * usa. Lo que no está en la lista se niega, y para eso existe la lista.
 *
 * # Lo que se bloquea se tiene que poder desbloquear
 *
 * Un programa de terceros que quiera capturar la pantalla tiene el portal, que
 * pregunta —que es el camino que corresponde—. Y para lo que no pasa por el
 * portal está `permitidos_extra` en la configuración del plugin, que no exige
 * recompilar nada. Sin alguna de esas dos cosas esto sería `security-context-v1`
 * otra vez: bloquear sin poder desbloquear.
 */
Decision decidir(const std::string& binario, const std::string& protocolo);

/**
 * Los binarios que la configuración agrega a la lista, con todo permitido.
 *
 * Se fija una vez al arrancar el plugin. Vacío es lo normal.
 */
void fijar_permitidos_extra(const std::set<std::string>& binarios);

/**
 * Lo que ya se anotó, para no repetirlo.
 *
 * El filtro se llama por cada global y por cada cliente que enumera el
 * registro, o sea decenas de veces por programa que arranca. Anotar todo sería
 * un registro que nadie lee, y encima uno que tapa el resto del log del
 * escritorio. Se anota la primera vez que un binario pide un protocolo, y
 * nunca más.
 */
class Memoria
{
  public:
    /** `true` si es la primera vez que este binario pide este protocolo. */
    bool es_nuevo(const std::string& binario, const std::string& protocolo);

    std::size_t cuantos() const
    {
        return vistos.size();
    }

  private:
    std::set<std::pair<std::string, std::string>> vistos;
};

/**
 * El binario detrás de un pid, o algo que se entienda si no se pudo saber.
 *
 * Lee `/proc/<pid>/exe`. Puede fallar —el cliente se murió entre que pidió y
 * que miramos, o es de otro usuario— y en ese caso devuelve un texto con el
 * pid en vez de una cadena vacía: un registro que dice «pid 4321 (no se pudo
 * leer)» sirve para buscar, y uno vacío no.
 */
std::string binario_de(pid_t pid);
} // namespace vasak

#!/usr/bin/env bash
# Que el detalle del filtro no vuelva a salir en cada arranque.
#
# El filtro corre cuando un cliente **enumera el registro**, no cuando pide
# algo, así que cualquier programa que se conecte genera una ráfaga de líneas
# por el solo hecho de arrancar. Medido antes de bajarlo a debug: **276 líneas
# en un solo arranque**, de diecisiete binarios.
#
# La idea era que quien busca por qué algo dejó de funcionar lo encontrara ahí.
# Con ese volumen pasaba lo contrario: tapaba todo el resto del arranque, y
# encima mandaba a perseguir fantasmas —`zwf_shell_manager_v2` «negado» al
# escritorio parecía un permiso faltante cuando el binario ni nombra ese
# protocolo—.
#
# Esto no lo ve el compilador: `LOGI` y `LOGD` compilan igual de bien.
set -uo pipefail

AQUI=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
FUENTE="$AQUI/../src/permisos-globales/plugin.cpp"

fallos=0
mal()  { echo "  FALLA: $*" >&2; fallos=$((fallos + 1)); }
bien() { echo "  ok: $*"; }

# El cuerpo de `decidir`, que es lo que corre por cada par de cliente y global.
cuerpo=$(sed -n '/bool decidir(const wl_client/,/^    }$/p' "$FUENTE")

if [ -z "$cuerpo" ]; then
    mal 'no se encontró el cuerpo de decidir(): esta prueba quedó mirando al vacío'
else
    if printf '%s' "$cuerpo" | grep -qE '^[[:space:]]*LOGI\('; then
        mal 'decidir() volvió a anotar en info: son cientos de líneas por arranque'
    else
        bien 'el detalle por cliente y protocolo no sale en info'
    fi

    printf '%s' "$cuerpo" | grep -qE '^[[:space:]]*LOGD\(' \
        && bien 'el detalle sigue estando en debug' \
        || mal 'decidir() dejó de anotar: el caso que esto explica se queda sin registro'
fi

# «negado» describe un rechazo que no hubo: nadie pidió nada, el cliente
# enumeró el registro y se le ocultó un global. La palabra es la que mandó a
# buscar un fallo inexistente, así que no puede volver.
if printf '%s' "$cuerpo" | grep -q 'negado a '; then
    mal 'volvió el «negado a»: es un ocultamiento, no un rechazo, y esa palabra manda a buscar un fallo que no existe'
else
    bien 'el texto dice que no se ofrece, no que se negó'
fi

# Y la línea de arranque tiene que seguir diciendo dónde está el detalle, o
# bajarlo a debug lo vuelve invisible.
grep -q 'debug' "$FUENTE" \
    && bien 'la línea de arranque dice cómo ver el detalle' \
    || mal 'nada le dice a nadie que el detalle está en debug'

echo
if [ "$fallos" -eq 0 ]; then
    echo "Todo bien."
else
    echo "$fallos comprobación(es) fallaron."
fi
exit "$((fallos > 0 ? 1 : 0))"

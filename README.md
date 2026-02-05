# KaServerUDP

Sistema de transferencia de archivos cliente-servidor utilizando el protocolo UDP con mecanismos de fiabilidad implementados a nivel de aplicacion.

## Descripcion

KaServerUDP implementa un protocolo personalizado sobre UDP para la transferencia confiable de archivos. El sistema maneja multiples sesiones concurrentes, deteccion de paquetes duplicados, reenvio automatico en caso de perdida y confirmacion de recepcion (ACK) para cada fragmento transmitido.

## Arquitectura

El proyecto consta de dos componentes principales:

### Sender (Cliente)
Programa que envia archivos al servidor dividiendolos en fragmentos de 1024 bytes. Implementa:
- Fragmentacion de archivos en chunks de tamano fijo
- Sistema de numeracion secuencial de paquetes
- Mecanismo de reintento con timeout de 500ms
- Verificacion de ACKs del servidor
- Soporte para multiples hilos de envio concurrentes

### Receiver (Servidor)
Servidor que escucha en el puerto 8500 y recibe archivos de multiples clientes simultaneamente. Caracteristicas:
- Manejo de hasta 8 sesiones concurrentes
- Deteccion y descarte de paquetes duplicados
- Gestion de sesiones por IP y puerto del cliente
- Sincronizacion con mutex para acceso seguro a recursos compartidos
- Almacenamiento de archivos recibidos con nomenclatura basada en la direccion del cliente

## Estructura del Proyecto

```
KaServerUDP/
├── sender.c           # Implementacion del cliente
├── receiver.c         # Implementacion del servidor
├── include/
│   └── structs.h      # Definicion de estructuras compartidas
├── files_read/        # Directorio para archivos a enviar
├── files_received/    # Directorio donde se almacenan archivos recibidos
└── Makefile           # Herramientas para simulacion de red con netem
```

## Protocolo de Comunicacion

Cada paquete tiene la siguiente estructura:

```
[4 bytes: sequence_index][4 bytes: size][size bytes: data]
```

- `sequence_index`: Numero de secuencia del fragmento (network byte order)
- `size`: Cantidad de bytes de datos en el paquete (network byte order)
- `data`: Contenido del fragmento

El servidor responde con un ACK que contiene el numero de secuencia recibido.

## Compilacion

Para compilar el sender:
```bash
gcc -o sender sender.c -pthread
```

Para compilar el receiver:
```bash
gcc -o receiver receiver.c -pthread
```

## Uso

### Iniciar el servidor

```bash
./receiver
```

El servidor comenzara a escuchar en el puerto 8500.

### Enviar un archivo

Modifica las direcciones IP y archivos en `sender.c` (lineas 23-24) y ejecuta:

```bash
./sender
```

Los archivos recibidos se guardaran en `./files_received/` con el formato:
```
<IP_cliente>_<puerto>.txt
```

## Simulacion de Condiciones de Red

El Makefile incluye comandos para simular condiciones adversas de red usando Linux Traffic Control (tc) con netem:

```bash
# Activar simulacion (delay, perdida, duplicacion)
make netem-on

# Desactivar simulacion
make netem-off

# Ver estado actual
make netem-show
```

Parametros configurados:
- Delay: 100ms ± 20ms
- Perdida de paquetes: 5%
- Duplicacion de paquetes: 20%

## Caracteristicas Tecnicas

- Protocolo: UDP (SOCK_DGRAM)
- Tamano maximo de chunk: 1024 bytes
- Offset de cabecera: 8 bytes
- Sesiones maximas: 8 concurrentes
- Timeout de reenvio: 500ms
- Puerto del servidor: 8500

## Manejo de Errores

El sistema implementa:
- Reenvio automatico de paquetes perdidos
- Deteccion de paquetes duplicados
- Validacion de tamano de paquetes
- Sincronizacion con mutex para evitar condiciones de carrera
- Gestion dinamica de memoria para arrays de chunks

## Requisitos

- Compilador GCC con soporte para C99 o superior
- POSIX threads (pthread)
- Linux (para las funciones de networking y tc)

## Notas

- El cliente esta configurado para enviar a 192.168.64.5:8500 por defecto
- Los archivos se leen en modo binario para soportar cualquier tipo de archivo
- El servidor continua ejecutandose indefinidamente hasta ser terminado manualmente

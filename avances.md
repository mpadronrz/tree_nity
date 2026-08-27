# Avances del cliente de treenity

## 1. Resumen y teoría del proyecto

`treenity` es una cola de mensajes. Una cola de mensajes permite que distintos programas se comuniquen sin tener que trabajar exactamente al mismo tiempo.

Ejemplo sencillo: una aplicación crea un usuario y publica `user.create`. Otro programa puede recibir ese mensaje más tarde para enviar un correo o guardar estadísticas. El primer programa no debe esperar a que el segundo termine.

El proyecto tiene dos programas:

- El servidor guarda los topics, los mensajes y la información de los consumidores.
- El cliente es el programa que una persona usa para hablar con el servidor.

### Conceptos importantes

- **Topic:** un canal con nombre donde se guardan mensajes, por ejemplo `user_events` u `orders`.
- **Productor:** cliente que manda mensajes a un topic.
- **Consumidor:** cliente que se suscribe a un topic para recibir mensajes.
- **Mensaje:** tiene una key, un body y un offset. Un ejemplo es `user.create:{"name":"Ada"}`.
- **Offset:** número que indica la posición de un mensaje dentro del topic. El servidor lo incrementa.
- **ACK:** significa *acknowledgement*, es decir, “confirmación”. Es como decirle al servidor: “he recibido y procesado este mensaje; apunta que ya no necesito que me lo mandes otra vez”.

#### ACK explicado paso a paso

Imagina que un topic tiene estos mensajes:

```text
offset 0 -> user.create
offset 1 -> user.update
offset 2 -> user.delete
```

1. El consumidor recibe el mensaje del offset `0`.
2. Lo muestra o lo procesa.
3. Envía un ACK con el número `1`.
4. Ese `1` no significa “he recibido el mensaje 1”. Significa “la próxima vez quiero empezar por el mensaje 1”.

Por eso, al procesar el mensaje `N`, siempre se confirma `N + 1`. Si el programa se cierra y vuelve a conectarse, el servidor mira el offset guardado y sabe desde qué mensaje debe continuar. Así se evita repetir mensajes ya confirmados.

### FIFO y protocolo

FIFO también se llama *named pipe* o “tubería con nombre”. Es un archivo especial del sistema que permite que dos procesos intercambien bytes.

Explicado de forma simple: imagina un tubo con una etiqueta. Un programa mete información por un extremo y otro programa la saca por el otro. La etiqueta es el nombre o ruta del FIFO, por ejemplo `/tmp/treenity.server.1234`. Como tiene un nombre, procesos que no se conocen pueden abrir el mismo tubo y comunicarse.

FIFO viene de **First In, First Out**, que significa “lo primero que entra es lo primero que sale”. Si un productor escribe primero el mensaje A y después el mensaje B, quien lee el FIFO recibirá A antes que B.

- El servidor tendrá un FIFO principal para las peticiones. Es el tubo al que el cliente manda “crea este topic” o “quiero suscribirme”.
- Cada petición necesita un FIFO de respuesta para que el cliente pueda recibir el resultado. Es otro tubo que permite al servidor contestar “topic creado” o “ha ocurrido un error”.
- Cada consumidor tendrá un FIFO dedicado para recibir mensajes continuamente. Es su tubo privado de entrada mientras está suscrito.

Un FIFO transmite bytes, no objetos de C++. Por eso cliente y servidor deben acordar cómo se convierten las peticiones, respuestas, mensajes y ACKs en bytes. Ese acuerdo se llama **protocolo**.

### Formatos de mensajes

El productor admite dos formatos:

- **Texto:** `key:body`, una línea por mensaje. Solo el primer `:` separa key y body.
- **Raw:** `[keysize:int32][key][valuesize:int32][value]`.

En raw, `int32` ocupa cuatro bytes y usa little-endian: el byte menos importante se escribe primero. Los registros se colocan seguidos, sin saltos de línea ni separadores.

El tamaño conjunto de key y body no puede superar 1024 bytes.

### EOF, filtros y reconexión

**EOF** significa “fin de archivo”: ya no quedan más datos en stdin.

- En texto, EOF normalmente aparece al pulsar `Ctrl+D`.
- En raw, EOF solo es válido justo después de un registro completo.
- EOF en mitad de un registro raw es un error de productor y debe dar código `1`.

Un consumidor puede usar un prefijo para recibir solo algunas keys. Por ejemplo, `user` recibe `user` y `user.create`, pero no `admin.login`. El prefijo vacío recibe todos los mensajes.

El offset guardado es siempre el próximo mensaje que el consumidor quiere recibir. Por eso el ACK de un mensaje `N` es `N + 1`. Si el suscriptor vuelve sin `--offset`, continúa desde su offset guardado.

### Miembro 2

`./client`:

1. Leer y validar los argumentos de línea de comandos.
2. Leer mensajes de productor en texto o raw.
3. Enviar peticiones por FIFO y recibir respuestas.
4. Mantener el bucle de escucha del consumidor suscrito.
5. Mostrar mensajes, enviar ACKs y aplicar códigos de salida.
6. Cerrar ordenadamente cuando llegue `SIGINT` o `SIGTERM`.

## 2. Estructura creada

```text
client/
├── inc/          # Archivos .hpp: declaraciones de funciones y estructuras
└── src/          # Archivos .cpp: implementación

tests/
└── client/       # Tests propios del cliente
```

## 3. Trabajo realizado

### 3.1. Lectura de comandos

El cliente ya entiende estas órdenes:

```text
./client <ipc_identifier> create <topic_name>
./client <ipc_identifier> list
./client <ipc_identifier> produce <topic_name> [--raw]
./client <ipc_identifier> subscribe <topic_name> <subscriber_name> [--prefix <prefix>] [--offset <offset>] [--raw]
./client <ipc_identifier> info <subscriber_name>
```

Antes de usar FIFO, el programa revisa que los argumentos tengan sentido. Por ejemplo, detecta nombres con espacios, opciones repetidas, un `--offset` que no es un número o un subcomando desconocido.

### 3.2. Validación de nombres y opciones

Los nombres de toopic y suscriptor deben tener entre 1 y 32 caracteres. Solo pueden contener letras, números, punto, guion bajo y guion normal.

Las opciones de `subscribe` se pueden escribir en distinto orden. Por ejemplo, tanto `--raw --offset 5` como `--offset 5 --raw` son válidos.

### 3.3. Lectura de mensajes de productor en texto

En el modo normal, cada línea de entrada debe tener esta forma:

```text
key:body
```

Solo el primer `:` separa la key del body. Esto permite que el body contenga más dos puntos, por ejemplo un JSON.

También se rechazan mensajes cuyo tamaño conjunto de key y body sea mayor de 1024 bytes, como manda el subject.

### 3.4. Lectura de mensajes binarios con `--raw`

El modo raw no usa texto ni saltos de línea. Lee los datos exactamente así:

```text
[keysize:int32][key][valuesize:int32][value]
```

Los números se interpretan en little-endian: el byte menos importante se guarda primero. Los registros pueden ir uno detrás de otro, sin espacios ni separadores.

**EOF** significa “fin de archivo”: ya no quedan más datos en stdin.

- Si aparece después de un mensaje completo, está bien.
- Si aparece en mitad de un mensaje raw, es un error: faltan bytes y el cliente debe terminar con código `1`.

### 3.5. Formato de salida del consumidor

También está preparado el formato que usará un consumidor al recibir un mensaje:

- Modo texto: `key:body`, terminado en salto de línea.
- Modo raw: `[offset:int32][keysize:int32][key][valuesize:int32][value]`.

El offset se añade en la salida raw del consumidor para saber qué posición tenía ese mensaje dentro del topic.

### 3.6. Códigos de salida

Se ha centralizado la regla del subject:

- `0`: todo fue bien.
- `1`: error general o argumentos inválidos.
- `2`: error de topic o cliente.
- `3`: error de comunicación IPC.

Si ocurren varios errores, gana esta prioridad: `1 > 3 > 2`.

### 3.7. Base para FIFO

Ya existe una capa pequeña para leer y escribir correctamente en descriptores de FIFO:

- Escritura completa: aunque el sistema acepte solo una parte de los bytes, se sigue escribiendo hasta terminar.
- Lectura exacta: se espera hasta recibir todos los bytes necesarios para una estructura.
- Detección de desconexión: diferencia entre un FIFO que termina limpiamente y un mensaje que se corta a mitad.
- Reintento con `EINTR`: si una señal interrumpe una lectura o escritura, se vuelve a intentar de forma segura.

### 3.8. Señales de cierre

El cliente ya instala handlers para `SIGINT` (Ctrl+C) y `SIGTERM`.

El handler no cierra nada directamente: solo marca que se pidió el cierre. Esto es importante porque dentro de un handler de señal no es seguro hacer trabajo complejo.

Cuando exista el bucle de suscripción, ese bucle verá la marca, enviará el mensaje de desconexión al servidor y cerrará los FIFO ordenadamente.

## 4. Tests realizados

Estado actual: `make test` ejecuta 22 tests y los 22 pasan.

Se prueban:

- Comandos correctos e incorrectos.
- Mensajes de texto.
- Mensajes raw.
- EOF parcial.
- Límite de 1024 bytes.
- Formato de salida.
- Prioridad de errores.
- Lectura y escritura de descriptores.
- Señales.

## 5. Lo que queda

La siguiente parte depende del protocolo compartido. Antes de unir cliente y servidor, los miembros 1 y 3 deben acordar las estructuras binarias de petición, respuesta y ACK.

1. Definir e integrar el protocolo común: tipos de operación, tamaños, respuestas, errores y serialización.
2. Abrir el FIFO principal del servidor y crear un FIFO temporal de respuesta para cada petición del cliente.
3. Implementar de verdad `create`, `list` e `info`.
4. Implementar `produce`: comprobar que existe el topic y enviar todos los mensajes leídos.
5. Implementar `subscribe`: registrar al consumidor, abrir su FIFO dedicado y recibir mensajes continuamente.
6. Enviar un ACK con `offset + 1` después de procesar cada mensaje.
7. Al recibir Ctrl+C o SIGTERM, desconectar limpiamente, esperar lo necesario y terminar con el código correcto.
8. Hacer pruebas de integración con el servidor y los binarios de ayuda del subject.

## 6. Resumen

La parte hecha no habla todavía con el servidor, pero ya prepara, valida, lee y formatea toda la información que el cliente necesita. La siguiente fase consiste en conectar estas piezas mediante el protocolo FIFO compartido.

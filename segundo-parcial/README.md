# Monitor de señal analógica — FRDM-MCXA156

Nombre: Henyel Mota. Matrícula: 2020-10735 (verificar antes de entregar).
Placa: NXP FRDM-MCXA156. MCU: MCXA156, Cortex-M33.

Este paquete contiene el programa de aplicación y los archivos de pines; se integra sobre un proyecto SDK oficial. NO contiene un proyecto IDE autónomo ni un binario compilado. Generar ese proyecto con el SDK instalado, como se indica abajo. No hay resultados físicos inventados.

## 1. Materiales y montaje desde cero

- FRDM-MCXA156 y cable USB-C con datos.
- Potenciómetro lineal de 10 kΩ, protoboard y tres cables.
- Multímetro; para tiempos, osciloscopio o analizador lógico compatible con 3.3 V.
- Se usan el botón SW2 y los canales verde y rojo del LED RGB D12 integrados: sus resistencias ya están en la placa. Al encenderse ambos puede verse amarillo; no son dos encapsulados separados.

Desconectar USB antes de conectar cables. Identificar el pin 1 por el marcado del PCB y el manual, sin contar posiciones según una foto girada.

| Elemento | Conexión | Función |
|---|---|---|
| Extremo del potenciómetro | 3.3 V, J3-8 (LDO_3V3) | Alimentación |
| Otro extremo | GND, J3-12 o J3-14 | Tierra |
| Cursor central | A0, J4-2, P1_10 | ADC1_A8 |
| SW2 integrado | P1_7 | Pulsado = 0; pull-up configurado |
| Verde D12 integrado | P3_13 | Actividad, activo en 0 |
| Rojo D12 integrado | P3_12 | Alarma, activo en 0 |
| Azul D12 integrado | P3_0 | Permanece apagado |
| USB MCU-Link | J21 | Programación, alimentación y puerto COM |
| UART RX del MCU | P0_2, LPUART0_RXD | Conexión interna con MCU-Link |
| UART TX del MCU | P0_3, LPUART0_TXD | Conexión interna con MCU-Link |
| Diagnóstico | D2, J1-6, P3_1 | Alterna en cada disparo ADC |

No conectar el potenciómetro a 5 V. VREF configurada = VDDA (kLPADC_ReferenceVoltageAlt3), nominalmente 3.3 V; medir la alimentación analógica real al validar P03. Entrada admitida para esta práctica: 0 a VDDA. La fórmula de comprobación usa la referencia medida, no supone que es exactamente 3.300 V.

Conectar el USB a J21, marcado MCU-Link, no solamente al USB del MCU objetivo J23. Mantener los jumpers de fábrica; JP6 (VCOM_DIS) debe estar abierto para habilitar el puerto COM. No hace falta adaptador USB-UART externo.

## 2. Preparar MCUXpresso IDE

1. Instalar MCUXpresso IDE y el MCUXpresso SDK compatible con FRDM-MCXA156. El SDK debe corresponder a esta placa, no a otra FRDM. Registrar versiones reales abajo.
2. En el IDE, abrir Installed SDKs y agregar el SDK descargado si todavía no aparece. Los nombres de los menús pueden variar por versión.
3. Usar Import SDK example(s), seleccionar FRDM-MCXA156 y buscar lpadc_polling, dentro de driver_examples / lpadc / polling.
4. Importar con nombre Monitor_MCXA156. Elegir UART para consola si aparece esa opción; no semihosting. Compilar y ejecutar primero el ejemplo original para comprobar instalación y conexión.
5. En source, ubicar lpadc_polling.c, que contiene main(). Sustituir su contenido por main.c de este paquete. Alternativamente eliminar/excluir ese archivo y agregar main.c. Debe existir UN solo main.
6. Sustituir pin_mux.c y pin_mux.h del proyecto por los dos incluidos. Conservar board.c, board.h, clock_config.c/.h, startup, linker, CMSIS y los demás archivos originales del proyecto importado. No copiar otro startup ni borrar las dependencias.
7. Abrir Manage SDK Components y habilitar los controladores CTIMER, GPIO, PORT y LPUART si no están incluidos; LPADC ya pertenece al ejemplo. Verificar que se compilan fsl_ctimer.c, fsl_lpadc.c y fsl_lpuart.c, junto con los controladores requeridos por el SDK. No copiar drivers de otra versión del SDK.
8. Este main configura directamente LPUART0. No llamar BOARD_InitDebugConsole(), PRINTF() ni GETCHAR() en el bucle. El archivo hardware_init.c importado puede permanecer: su BOARD_InitHardware() no se llama desde este main.
9. Ejecutar Clean y Build. Un error 'fsl_ctimer.h: No such file' significa que falta agregar el componente/las rutas; 'undefined reference to CTIMER_...' significa que falta compilar su .c. 'multiple definition of main' significa que el original sigue incluido.
10. Pulsar Debug, seleccionar MCU-Link, permitir cargar el programa y pulsar Resume/Run. Al detenerse inicialmente en main el programa todavía no está ejecutando tareas. Terminar la sesión de depuración y reiniciar para la prueba final autónoma.
11. Si aparecen errores por nombres de API o estructuras, registrar versión SDK y primer error; este código se revisó contra el SDK oficial consultado, pero no fue compilado con tu instalación. No reemplazar identificadores al azar.

Herramientas usadas por el alumno: MCUXpresso IDE [completar versión]; SDK [completar versión]; compilador [completar versión]; terminal [completar nombre y versión]; instrumento [completar].

## 3. Terminal y primera ejecución

Seleccionar el puerto COM de MCU-Link. Configurar 115200 baudios, 8 bits, sin paridad, 1 stop, sin control de flujo. Envío terminado en LF o CRLF; CR solo no ejecuta comandos. Activar marcas de tiempo y guardar el registro. Usar una terminal que permita enviar una línea completa: Backspace no está implementado como edición dentro del firmware.

Con potenciómetro al mínimo, pulsar SW1 Reset y dejar SW2 libre. A partir del primer segundo aparece una línea periódica. Enviar STATUS después de 200 ms. El formato es:

t=1200;ADC=2866;PCT=70;THR=70;MODE=RUN;ALARM=1

Este ejemplo ilustra formato, no es una medición. Antes de la primera muestra ADC/PCT se muestran como cero; la alarma espera la primera muestra válida, a los 100 ms.

Comandos exactos, sensibles a mayúsculas:

| Comando | Respuesta |
|---|---|
| STATUS | Una línea de estado |
| THR 50 | OK THR=50 |
| START | OK RUN |
| STOP | OK PAUSE |
| Cualquier línea inválida | ERR |

THR acepta únicamente dígitos, sin signo, decimales ni espacios adicionales; 10..95 inclusive. Cada respuesta termina CRLF. Se admite una línea de 32 caracteres sin el terminador; el carácter 33 provoca descarte hasta LF y un ERR. CR solo es válido antes de LF. Líneas vacías se ignoran. Al fragmentar una línea, no se ejecuta nada hasta recibir LF.

## 4. Configuración y explicación del programa

### TIMER

Periférico real: CTIMER1, reloj FRO_HF con divisor funcional 1. Se conserva la configuración de reloj del ejemplo y se consulta CLOCK_GetCTimerClkFreq(1). La variable timer_hz permite ver el valor efectivo en el depurador. No confundir el reloj UART de 12 MHz con el del timer.

PR=0: f_contador = f_CTIMER / (PR+1) = f_CTIMER.
MR0=f_CTIMER/1000: al alcanzar esa cuenta, el hardware reinicia el contador y dispara interrupción. T_tick=MR0/f_contador=1 ms para las frecuencias enteras en kHz usadas aquí. Si timer_hz=96000000, MR0=96000; si es 48000000, MR0=48000. Registrar el valor real de la placa en el informe.

El driver fsl_ctimer.c habilita IRQ y atiende su bandera; llama TimerTick mediante CTIMER_RegisterCallBack. No definir otro CTIMER1_IRQHandler sobre el driver. La interrupción incrementa el tiempo, dispara ADC cada 100 ticks, cambia el verde cada 500 y publica un evento de telemetría cada 1000. No hay cadenas ni esperas dentro de la ISR. El tiempo t es el tiempo de aplicación contado desde el arranque de CTIMER al terminar la inicialización; la breve inicialización/calibración anterior no se contabiliza.

### ADC

ADC1, canal A8, entrada P1_10, referencia VDDA, modo estándar SINGLE-ENDED de 12 bits. El resultado LPADC estándar está desplazado 3 bits: convValue >> 3. Rango útil 0..4095. El TIMER dispara por software el trigger 0; el ADC convierte por hardware y el bucle consulta su FIFO sin esperar activamente el resultado.

PCT=((uint32_t)ADC*100 + 2047)/4095. Es redondeo al entero más cercano; el producto usa 32 bits. Valores orientativos: ADC=0 -> 0%; 2048 -> 50%; 4095 -> 100%.

El único SDK_DelayAtLeastUs de 1 us está en la inicialización de calibración, igual que en la secuencia del fabricante. No se usa para planificar tareas ni para antirrebote; en ejecución normal no hay retardos bloqueantes.

### GPIO y alarma

SW2 usa pull-up y nivel activo bajo. ButtonService detecta cambios del nivel candidato y exige 30 ms estables. Solo se arma una nueva pulsación tras liberar de forma estable; mantener pulsado no repite. Al reiniciar con SW2 presionado, primero debe liberarse para armarlo.

En RUN, AlarmSample enciende con PCT>=THR y apaga con PCT<THR-5; entre ambos conserva el estado. Con THR=70, 65% conserva el estado, 64% apaga. En PAUSE fuerza apagado.

SetRunning solo reinicia la decisión al pasar realmente de PAUSE a RUN. START repetido en RUN conserva la decisión. THR válido reinicia la decisión incluso si el nuevo umbral es igual al anterior. Sin muestra válida, espera a la primera conversión. Al reanudar o cambiar umbral se enciende si PCT>=THR y se apaga en otro caso; después vuelve la histéresis.

### UART y concurrencia

LPUART0, fuente FRO12M, 115200 8N1. RxService consulta la recepción sin bloquear, con presupuesto de 16 bytes por vuelta. ReceiveByte conserva el ensamblado de líneas entre vueltas; Command procesa fuera de interrupciones y valida antes de cambiar parámetros.

La salida se coloca como líneas completas en una cola de 2048 bytes. TxService deposita un byte solo cuando el UART tiene espacio; las líneas nunca se intercalan. A 115200 8N1 cada byte tarda unos 86.8 us: una línea de 80 bytes tarda unos 6.95 ms. La carga solicitada de 5 STATUS/s más telemetría cabe holgadamente; debe medirse la latencia real en P10. No es una garantía para entrada ilimitada: si se llena la cola se descarta una línea completa y aumenta tx_overflows; comprobar que permanezca cero en P10.

## 5. Pruebas y evidencias

resultados.csv es una plantilla SIN EJECUTAR, no contiene PASS inventados. Ejecutar las pruebas exactas del mandato. Al terminar usar PASS/FAIL con valores, archivo de evidencia e interpretación de todo FAIL.

- P01: mínimo, reset, STATUS después de 200 ms.
- P02: medir 10 intervalos representativos por evento. D2 alterna cada 100 ms: medir flancos consecutivos de cualquier polaridad, no solo ascendentes (estos se separan 200 ms). Verde P3_13 alterna cada 500 ms. En UART medir inicio de las líneas periódicas sin comandos, cada 1000 ms. Tolerancias: 95..105, 475..525 y 950..1050 ms.
- P03: medir Vin y VDDA en mínimo/medio/máximo; comparar PCT con 100*Vin/VDDA, tolerancia ±3 puntos. Verificar fórmula a partir del ADC mostrado.
- P04: THR 70 y niveles mostrados 60,72,67,62 producen 0,1,1,0.
- P05: 5 pulsaciones separadas; una mantenida 2 s; observar una acción por pulsación. Medir estabilidad del botón si se dispone de instrumento.
- P06: a ~80%, STOP; mover entrada; STATUS; volver a ~80%; START. ADC y actividad continúan y la alarma solo vuelve al reanudar.
- P07/P08: comprobar límites, cambios inmediatos y que inválidos no cambian umbral ni modo.
- P09: enviar b'STA' y después b'TUS\n'; repetir con CRLF; enviar 33 caracteres+LF y luego STATUS.
- P10: 30 s a 5 STATUS/s, cambiar potenciómetro y alternar SW2; medir desde fin del LF recibido hasta fin de la respuesta, incluyendo cualquier telemetría en cola. Verificar ≤100 ms, intervalos y tx_overflows=0.

Conectar siempre GND del instrumento a GND de placa. Una traza del TX, RX y D2 permite medir muestreo y latencia. Si no puedes acceder a las señales internas RX/TX, guardar marcas de tiempo de terminal como evidencia complementaria y declarar la precisión/limitación de esa medida.

## 6. Qué se ha verificado aquí

Se ejecutaron pruebas de la lógica C en un equipo anfitrión con salidas GPIO/UART simuladas: comandos válidos/erróneos, límites, fragmentación, LF/CRLF, recuperación de línea larga, histéresis y reglas de START/THR. No prueban drivers, compilación para MCXA156, precisión ADC ni tiempos reales. El programa completo no se ha compilado ni cargado en una placa en este entorno. P01..P10 siguen pendientes.

## 7. GitHub y entrega

Crear repositorio propio y trabajar con commits reales. No fabricar un historial de avances ya realizados. El primer commit puede contener esta base identificada como pendiente de validar; después guardar las correcciones de compilación y montaje, resultados físicos y documentación como ocurran.

Una vez importado y compilado, subir el proyecto generado COMPLETO: .project, .cproject, .mex si aplica, fuentes, startup, linker y configuración. Conservar una copia/exportación reproducible del SDK o indicar versión y obtención. No subir únicamente este ZIP como si fuera ya el proyecto IDE completo. Excluir carpetas de compilación temporales y metadatos de workspace. Agregar evidencias y resultados; etiquetar la versión final únicamente al terminar las pruebas. El alumno debe poder explicar y modificar cada función.

## Referencias oficiales

- Manual de placa: https://www.nxp.com/docs/en/user-manual/UM12121.pdf
- Inicio: https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa156:GS-FRDM-MCXA156
- Ejemplos NXP: https://github.com/nxp-mcuxpresso/mcuxsdk-examples
- API MCXA156: https://docs.mcuxpresso.nxp.com/mcuxsdk/latest/api/devices/MCXA156/index.html

pin_mux.c/.h conservan sus avisos del fabricante; procedencia del ejemplo oficial frdmmcxa156/driver_examples/lpadc/polling, commit 7f27ca7c319e51594e04afa50139515d780f6575. Usar los drivers y reloj del SDK instalado, no mezclar versiones de drivers.

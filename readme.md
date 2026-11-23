# Mini Paint - Rasterizador 2D con C++, OpenGL e ImGui

Aplicacion de dibujo y rasterizacion 2D interactiva desarrollada en **C++** que utiliza **OpenGL 3.3**, **GLFW**, **GLAD** y **Dear ImGui**. Implementa algoritmos graficos desde cero directamente sobre un bufer de pixeles en memoria, permitiendo trabajar tanto a nivel de pixel puro como en un modo de grilla discreta.

## Caracteristicas Principales

* **Modo Dual de Renderizado:** Alterna en tiempo real entre dibujo libre a nivel de pixel y modo de grilla estructurada (`GRID_SIZE`), ajustando automaticamente el centrado y el grosor de los trazos.
* **Herramientas de Dibujo y Geometria:**
  * **Mano Alzada y Borrador:** Trazos continuos optimizados mediante muestreo de distancia minima (`minPaso`).
  * **Linea Recta:** Implementacion pura del algoritmo de Bresenham.
  * **Circulo:** Renderizado simetrico por octantes mediante el algoritmo de circulo de Bresenham.
  * **Triangulo (3 clics):** Construccion interactiva mediante seleccion de vertices con vista previa dinamica.
  * **Cuadrado, Poligono Regular y Estrella:** Generacion parametrica configurable (numero de lados y puntas) mediante trigonometria analitica.
* **Herramientas de Edicion y Transformacion:**
  * **Relleno (Flood Fill):** Algoritmo de inundacion por pila para celdas y algoritmo optimizado *Scanline Fill* para pixeles.
  * **Rotacion Interactiva:** Calculo de centroide y transformacion matricial en 2D mediante clic izquierdo (horario) y clic derecho (antihorario).
  * **Historial:** Soporte para deshacer acciones (`Deshacer`) y limpiar el lienzo por completo (`Limpiar`).
* **Optimizacion de Rendimiento:** Uso de doble bufer en CPU (`cache` y `pantalla`) con bandera de actualizacion condicional (`sucio`) para evitar el renderizado innecesario de figuras estaticas en cada fotograma inactivo.

## Arquitectura y Algoritmos Implementados

El proyecto esta estructurado de forma modular en un unico archivo fuente principal, dividido en los siguientes modulos logicos:

### 1. Gestion del Bufer y Color
* Representacion de color empaquetada en enteros de 32 bits (`uint32_t` en formato RGBA).
* Funciones en linea (`rgba`, `obtR`, `obtG`, `obtB`) para la manipulacion eficiente de canales mediante operaciones a nivel de bits.
* **Alpha Blending (`mezclarPixel`):** Mezcla semitransparente para superponer la grilla visual sin alterar los datos del dibujo subyacente.

### 2. Algoritmos de Rasterizacion
* **Linea de Bresenham (`lineaBresenham`):** Trazado de segmentos rectos utilizando exclusivamente aritmetica entera y un acumulador de error (`err`), eliminando operaciones de punto flotante.
* **Circulo de Bresenham (`circuloBresenham`):** Calculo de un octante de la circunferencia replicado por simetria en los 8 octantes simultaneamente.
* **Relleno por Inundacion (`rellenoCeldas` y `rellenoPixeles`):**
  * *Modo Celda:* Búsqueda en profundidad iterativa usando `std::vector` como pila y matriz de estados visitados.
  * *Modo Pixel:* Algoritmo *Scanline Fill* que escanea tramos horizontales completos (`xl` a `xr`) para optimizar el uso de memoria y la velocidad de procesamiento.

### 3. Geometria Analitica y Seleccion
* **Deteccion de Proximidad (`distSegmento` y `buscarFigura`):** Proyeccion vectorial para calcular la distancia minima entre el cursor y los segmentos de las figuras almacenadas dentro de un umbral de tolerancia (`TOL = 8.0f`).
* **Transformaciones Afines (`rotarFigura`):** Traslacion al origen respecto al centroide geometrico, aplicacion de matriz de rotacion en 2D ($\cos \theta, \sin \theta$) y retorno a las coordenadas originales del lienzo.

## Requisitos y Dependencias

Para compilar y ejecutar este proyecto se requiere un entorno compatible con **C++17** (GCC, Clang o MSVC) y las siguientes librerias de desarrollo:

* **OpenGL** (version 3.3 Core Profile o superior)
* **GLFW3** (gestion de ventanas y eventos de entrada)
* **GLAD** (cargador de punteros a funciones de OpenGL)
* **Dear ImGui** (interfaz grafica de usuario, incluyendo los backends `imgui_impl_glfw` e `imgui_impl_opengl3`)

## Compilacion y Ejecucion

### Linux (GCC / G++)
Asegurese de tener instalados los paquetes de desarrollo de GLFW (`libglfw3-dev`) y los archivos fuente de GLAD e ImGui en el directorio de trabajo:

```bash
g++ -std=c++17 main.cpp glad.c imgui/*.cpp -I./include -I./imgui -lglfw -lGL -ldl -lpthread -o MiniPaint
./MiniPaint
```

### Windows (MinGW / MSYS2)
```bash
g++ -std=c++17 main.cpp glad.c imgui/*.cpp -I./include -I./imgui -lglfw3 -lopengl32 -lgdi32 -o MiniPaint.exe
MiniPaint.exe
```

## Guia de Uso

1. **Panel Izquierdo (Herramientas):**
   * Seleccione la herramienta deseada mediante los botones de opcion.
   * Ajuste el **Color de trazo**, el **Grosor**, el **Numero de lados/puntas** (para poligonos y estrellas) o el **Angulo de rotacion**.
   * Active o desactive las opciones de **Mostrar grilla** y **Pintar cuadrados** segun el modo de trabajo deseado.
2. **Lienzo Principal:**
   * **Clic izquierdo y arrastrar:** Dibuja lineas, circulos, poligonos, estrellas o trazos a mano alzada.
   * **Herramienta Triangulo:** Realice 3 clics consecutivos en distintas posiciones del lienzo para definir sus vertices.
   * **Herramienta Rotar:** Haga clic izquierdo sobre el borde de una figura para rotarla en sentido horario, o **clic derecho** para rotarla en sentido antihorario.
   * **Clic derecho (durante un trazo):** Cancela la construccion de la figura o triangulo activo en curso.
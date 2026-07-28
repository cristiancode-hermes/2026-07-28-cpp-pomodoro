# Pomodoro Timer 🍅

**CLI Pomodoro Technique timer escrito en C++23** — herramienta de productividad que gestiona intervalos de trabajo y descanso usando la técnica Pomodoro.

## Características

- ⏱️ Temporizador Pomodoro completamente funcional con conteo regresivo en tiempo real
- 🎨 Salida con colores ANSI para mejor legibilidad
- 📋 Planificador de sesiones: configura duración de trabajo, descanso y descanso largo
- 🔄 Ciclos automáticos: trabajo → descanso → trabajo → descanso largo
- 📊 Barra de progreso visual y porcentaje completado
- 🔧 Totalmente configurable vía argumentos CLI
- 📄 Salida JSON para integración con scripts y herramientas
- 🚫 Sin dependencias externas — solo C++23 estándar

## Compilación

### Directa (g++)
```bash
g++ -std=c++23 -O2 -Wall -Wextra src/main.cpp -o pomodoro
```

### Con CMake
```bash
cmake --preset release
cmake --build --preset release
```

## Uso

```bash
# Mostrar plan de sesiones por defecto
./pomodoro

# Configuración personalizada
./pomodoro --work 30 --break 10 --long 20 --sessions 3

# Temporizador interactivo
./pomodoro --timer

# Salida JSON
./pomodoro --json

# Sin colores
./pomodoro --no-color

# Ayuda
./pomodoro --help
```

## Ejemplo de salida

```
📋 Pomodoro Session Plan
  2026-07-28  |  Started at 10:30

  Work:        25 min
  Break:       5 min
  Long Break:  15 min  (every 4 sessions)

  Schedule:
    Session 1:  Work 25 min  →  Break 5 min
    Session 2:  Work 25 min  →  Break 5 min
    Session 3:  Work 25 min  →  Break 5 min
    Session 4:  Work 25 min  →  Long Break 15 min

  Total time:  130 min = 2 h 10 min
```

## Técnicas utilizadas

- C++23 (`std::println`, `std::print`, `std::chrono`)
- `std::chrono::steady_clock` para temporización precisa
- ANSI escape codes para interfaz coloreada
- `\r` carriage return para actualización en tiempo real
- RAII y manejo de errores sin excepciones
- Argumentos CLI manuales sin dependencias externas

# Binary-SQL Studio — God Mode Design Reference
> Monochrome terminal with amber accents. The design feels like a precisely coded interface, where every element serves a distinct, functional purpose against a dark, featureless backdrop. It is designed to be the ultimate, premium database management studio for a custom C-based SQL engine.

**Theme:** dark

This design system evokes a sparse, high-contrast digital workspace, reminiscent of a modern command line interface or top-tier developer tools (like Linear, Vercel, or Raycast) but with a refined typographic sensibility. The stark black backgrounds (#101010, #080808) are punctuated by crisp white text (#F3F3F3, #FFFFFF), creating a sense of technical precision and directness. Minimal chromatic accents (#E7C59A, #00AC5C) are used sparingly, like status lights, ensuring they immediately draw the eye to critical SQL states (like active transactions or successful queries) without overwhelming the monochrome base. Custom Aeonik and Input fonts lend a distinctive, somewhat retro-futuristic feel, reinforcing the ultimate hacker/developer aesthetic.

## Tokens — Colors

| Name | Value | Token | Role |
|------|-------|-------|------|
| Midnight Void | `#101010` | `--color-midnight-void` | Primary page background, deepest dark surface for the code editor and main panels. |
| Deep Space | `#080808` | `--color-deep-space` | Secondary background, used for the sidebar (Schema Explorer) and Data Grid headers. |
| Polar White | `#F3F3F3` | `--color-polar-white` | Primary text color, SQL syntax base, clear contrast against dark backgrounds. |
| Absolute Zero | `#FFFFFF` | `--color-absolute-zero` | Accent text and background for interactive elements like the 'Ejecutar' button. |
| Ash Gray | `#949494` | `--color-ash-gray` | Secondary text, subtle borders, inactive database items, query timestamps. |
| Dark Carbon | `#333333` | `--color-dark-carbon` | Panel dividers, borders for the data grid, muted backgrounds for secondary elements. |
| Slate | `#C1C1C1` | `--color-slate` | Subtle outlines, UI dividers. |
| Light Gradients | `linear-gradient(rgb(181, 181, 181), rgb(228, 228, 228))` | `--color-light-gradients` | Subtle background gradient for highlighted UI elements. |
| Amber Glow | `#E7C59A` | `--color-amber-glow` | Key accent color for SQL Keywords (`CREAR`, `INSERTAR`, `INICIAR TRANSACCION`), 'Active' tags, and warnings. |
| Neon Green | `#00AC5C` | `--color-neon-green` | Success indicators (Query OK), connection status, execution times. |
| Crimson Error | `#EF4444` | `--color-crimson-error` | SQL Syntax errors, aborted transactions. |

## Tokens — Typography

### Aeonik — Primary typeface for UI elements, sidebar, and headers.
- **Substitute:** Inter, Geist
- **Weights:** 400, 700
- **Role:** Confident, geometric forms convey technical modernity and clarity for the UI shell.

### Input — Secondary typeface used STRICTLY for the SQL Editor and Data Grid.
- **Substitute:** JetBrains Mono, Fira Code, IBM Plex Mono
- **Weights:** 400, 500
- **Role:** Monospaced, technical contrast to Aeonik. Essential for writing queries and aligning tabular data perfectly. The tighter letter spacing enhances its code-like appearance.

## Tokens — Spacing & Shapes

**Base unit:** 4px
**Density:** compact (developer-focused)

### Border Radius
- default panels: `0px` or `4px` (keep it sharp and technical)
- buttons: `6px`
- statusIcons: `99px`

## Components for the Database Studio

### 1. Sidebar (Schema Explorer)
**Role:** Displays databases, tables, and columns.
Background: Deep Space (#080808). Text: Ash Gray (#949494) for collapsed items, Polar White (#F3F3F3) for expanded. Active database has a subtle Amber Glow (#E7C59A) dot indicator next to it. Font: Aeonik 14px.

### 2. SQL Editor Panel
**Role:** Where the user types Spanish SQL commands.
Background: Midnight Void (#101010). No visible borders, just a Dark Carbon (#333333) line separating it from the results. Font: Input (Monospace) 16px.
**Syntax Highlighting Rules:**
- Commands (`CREAR BASE DE DATOS`, `USAR`, `SELECCIONAR`, `INSERTAR`, `INICIAR TRANSACCION`, `CONFIRMAR`, `DESHACER`, `SALIR`): Amber Glow (#E7C59A).
- Strings & Values: Neon Green (#00AC5C).
- Table/Column names: Polar White (#F3F3F3).

### 3. Primary Action Button ('Ejecutar')
**Role:** Runs the SQL query.
Positioned floating at the bottom right of the SQL editor. Dark Carbon (#333333) background, Absolute Zero (#FFFFFF) text (Aeonik, weight 500), with a 6px radius. Text: "Ejecutar (Cmd+Enter)".

### 4. Data Grid (Resultados)
**Role:** Displays the results of a `SELECCIONAR` query.
Table with 1px solid Dark Carbon (#333333) borders. Header background is Deep Space (#080808) with Polar White text. Row hover effect changes background slightly to #151515. Font is strictly Input (Monospace) 14px for perfect data alignment.

### 5. Execution Terminal / Log
**Role:** Shows success/error messages from the C Engine.
Located below the Data Grid or as a separate tab. Text like `=> Base de datos 'escuela' creada en 12ms` should have the "=>" in Amber Glow, the string in Polar White, and the time "12ms" in Neon Green.

## Agent Prompt Guide (Para Claude Code / V0 / IDX)

Si eres un agente de IA generando código a partir de este documento, **DEBES SEGUIR ESTAS INSTRUCCIONES ESTRICTAMENTE:**

1. **Contexto:** Estás construyendo "Binary-SQL Studio", un frontend web para un motor de base de datos SQL escrito en C que usa **comandos en español**.
2. **Layout Principal:** Usa un layout de pantalla completa (100vh). No hagas scroll en la página principal. Panel izquierdo de 250px (Sidebar), y el resto dividido verticalmente (60% Editor SQL arriba, 40% Data Grid/Logs abajo).
3. **Estilo 'Modo Dios':** Aplica el tema oscuro especificado en los Tokens de Color. El fondo principal DEBE ser `#101010`. Todo debe verse como una herramienta para hackers profesionales y desarrolladores "10x". Usa TailwindCSS.
4. **Editor SQL:** Crea un área de texto grande tipo monospaced. Implementa un highlighting falso o real para palabras clave como `SELECCIONAR` y `CREAR TABLA` usando el color `#E7C59A`.
5. **Data Grid:** Crea una tabla hermosa y minimalista. Sin estilos por defecto del navegador. Usa bordes colapsados color `#333333`.
6. **Animaciones:** Agrega transiciones sutiles (150ms) en los botones y al hacer hover sobre las filas de la tabla.

### Prompt de Ejemplo para iniciar a generar:
> "Actúa como un Frontend Engineer nivel Dios. Construye la interfaz principal de 'Binary-SQL Studio' basado en el archivo `DESIGN.md`. Necesito un layout de pantalla completa (Sidebar a la izquierda, Editor SQL arriba a la derecha, Tabla de Resultados abajo a la derecha). Usa TailwindCSS, fondos `#101010` y `#080808`, textos `#F3F3F3`, y resalta los botones y palabras clave de SQL en `#E7C59A`. La fuente del editor de código debe ser monospaciada. Llena la tabla con datos falsos de estudiantes para que se vea espectacular."

## Do's and Don'ts

### Do
- Prioritize high contrast between text and background.
- Emplea Amber Glow (#E7C59A) exclusivamente para palabras clave de SQL y botones importantes.
- Usa fuentes monospaciadas (Input / Fira Code) para TODO lo relacionado con código y datos.
- Mantén el diseño ultra-limpio, con bordes de 1px oscuros (#333333).

### Don't
- NO uses colores vibrantes genéricos como azul o rojo pastel. Mantente en la paleta oscura + ambar/verde neón.
- NO redondees excesivamente los bordes (máximo 6px). Es una herramienta técnica, debe verse afilada.
- NO uses sombras suaves y difusas (drop shadows). Usa colores de fondo para diferenciar capas.

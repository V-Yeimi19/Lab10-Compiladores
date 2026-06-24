import glob
import os
import shutil
import subprocess
import sys

import difflib
from rich.console import Console
from rich.panel import Panel
from rich.text import Text
from rich.table import Table

console = Console()

# ---------------------------------------------------------------------------
# Configuración de directorios
# ---------------------------------------------------------------------------
BASE_DIR      = os.path.dirname(os.path.abspath(__file__))
INPUT_DIR     = os.path.join(BASE_DIR, "inputs")
OUTPUT_DIR    = os.path.join(BASE_DIR, "outputs")
EXPECTED_DIR  = os.path.join(BASE_DIR, "outputs_test")

CPP_SOURCES = [
    "main.cpp", "scanner.cpp", "token.cpp",
    "parser.cpp", "ast.cpp", "visitor.cpp",
]

# ---------------------------------------------------------------------------
# Fase 1: compilar el compilador C++ y ejecutarlo sobre cada input
# ---------------------------------------------------------------------------

def build_and_run():
    """Compila los fuentes C++ y ejecuta el compilador sobre cada input*.txt."""

    # --- Compilar ---
    sources = [os.path.join(BASE_DIR, s) for s in CPP_SOURCES]
    executable = os.path.join(BASE_DIR, "a.exe" if os.name == "nt" else "a.out")

    console.print(Panel(
        "[bold blue]Compilando fuentes C++...[/bold blue]",
        border_style="blue"
    ))

    result = subprocess.run(
        ["g++", "-o", executable] + sources,
        capture_output=True, text=True, cwd=BASE_DIR
    )

    if result.returncode != 0:
        console.print(f"[bold red]Error al compilar C++:[/bold red]\n{result.stderr}")
        return False

    console.print("[bold green]✓ Compilación C++ exitosa[/bold green]\n")

    # --- Ejecutar sobre cada input ---
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    inputs = sorted(glob.glob(os.path.join(INPUT_DIR, "input*.txt")))
    if not inputs:
        console.print(f"[bold yellow]Advertencia:[/bold yellow] No se encontraron inputs en {INPUT_DIR}")
        return False

    all_ok = True
    for filepath in inputs:
        filename = os.path.basename(filepath)
        stem     = os.path.splitext(filename)[0]
        number   = stem.replace("input", "")

        result = subprocess.run(
            [executable, filepath],
            capture_output=True, text=True, cwd=BASE_DIR
        )

        generated_s = os.path.join(INPUT_DIR, f"{stem}.s")
        dest_s      = os.path.join(OUTPUT_DIR, f"input_{number}.s")

        if result.returncode != 0:
            console.print(f"[bold red]✗ {filename}:[/bold red] error al compilar input\n{result.stderr}")
            all_ok = False
        elif os.path.isfile(generated_s):
            shutil.move(generated_s, dest_s)
            console.print(f"[green]✓[/green] {filename} → [cyan]input_{number}.s[/cyan]")
        else:
            console.print(f"[bold red]✗ {filename}:[/bold red] no se generó el .s")
            all_ok = False

    return all_ok


# ---------------------------------------------------------------------------
# Fase 2: comparar outputs/ contra outputs_test/
# ---------------------------------------------------------------------------

def format_diff(expected: str, actual: str, filename: str):
    diff = difflib.unified_diff(
        expected.splitlines(keepends=True),
        actual.splitlines(keepends=True),
        fromfile=f"ESPERADO  {filename}",
        tofile=f"GENERADO  {filename}",
    )
    text = Text()
    for line in diff:
        if line.startswith("---") or line.startswith("+++"):
            text.append(line, style="bold cyan")
        elif line.startswith("@@"):
            text.append(line, style="cyan")
        elif line.startswith("+"):
            text.append(line, style="bold green")
        elif line.startswith("-"):
            text.append(line, style="bold red")
        else:
            text.append(line, style="dim white")
    return Panel(text, title=f"Diferencia en [bold cyan]{filename}[/]", border_style="red")


def compare_outputs():
    """Compara cada .s en outputs_test/ contra su equivalente en outputs/."""

    console.print(Panel(
        f"[bold blue]Comparando resultados...[/bold blue]\n"
        f"  Esperado : [cyan]{EXPECTED_DIR}[/cyan]\n"
        f"  Generado : [cyan]{OUTPUT_DIR}[/cyan]",
        border_style="blue"
    ))

    expected_files = sorted(f for f in os.listdir(EXPECTED_DIR) if f.endswith(".s"))
    if not expected_files:
        console.print(f"[bold yellow]Advertencia:[/bold yellow] No hay archivos .s en {EXPECTED_DIR}")
        return False

    all_passed = True
    results    = []

    for filename in expected_files:
        expected_path = os.path.join(EXPECTED_DIR, filename)
        actual_path   = os.path.join(OUTPUT_DIR,   filename)

        if not os.path.exists(actual_path):
            console.print(f"[bold red]✗[/bold red] {filename}: archivo generado no encontrado")
            results.append((filename, False, "Archivo faltante"))
            all_passed = False
            continue

        try:
            expected_text = open(expected_path, encoding="utf-8").read()
            actual_text   = open(actual_path,   encoding="utf-8").read()
        except Exception as exc:
            results.append((filename, False, str(exc)))
            all_passed = False
            continue

        if expected_text == actual_text:
            results.append((filename, True, "OK"))
        else:
            console.print(f"\n[bold red]Falló {filename}[/bold red]")
            console.print(format_diff(expected_text, actual_text, filename))
            results.append((filename, False, "Diferencias en código ensamblador"))
            all_passed = False

    # --- Tabla resumen ---
    console.print()
    table = Table(title="Resumen de Resultados", border_style="magenta", show_lines=True)
    table.add_column("Archivo",  justify="left",   style="cyan", no_wrap=True)
    table.add_column("Estado",   justify="center")
    table.add_column("Detalle",  justify="left")

    for filename, passed, detail in results:
        status = "[bold green]✓ PASSED[/bold green]" if passed else "[bold red]✗ FAILED[/bold red]"
        style  = "dim" if passed else "red"
        table.add_row(filename, status, f"[{style}]{detail}[/{style}]")

    console.print(table)
    return all_passed


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    ok = build_and_run()

    console.print()

    if not ok:
        console.print(Panel(
            "[bold red]❌ La fase de compilación/ejecución tuvo errores.[/bold red]",
            border_style="red"
        ))
        sys.exit(1)

    passed = compare_outputs()

    console.print()
    if passed:
        console.print(Panel(
            "[bold green]✨ TODOS LOS TESTS PASARON CORRECTAMENTE ✨[/bold green]",
            border_style="green"
        ))
        sys.exit(0)
    else:
        console.print(Panel(
            "[bold red]❌ ALGUNOS TESTS FALLARON. REVISA LOS DETALLES ARRIBA ❌[/bold red]",
            border_style="red"
        ))
        sys.exit(1)

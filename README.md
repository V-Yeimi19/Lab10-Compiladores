# Laboratorio 14 — Generación de código con optimizaciones

Compilador de un lenguaje de alto nivel a ensamblador x86-64 (AT&T syntax), con tres optimizaciones aplicadas al operador de potencia (`**`) y a la evaluación de expresiones en general.

---

## Pipeline de compilación

```
Fuente (.fun)
    │
    ▼
Scanner  ──► Token stream
    │
    ▼
Parser   ──► AST
    │
    ▼
Opt1Visitor  ──► Plegado de constantes + inlining de funciones puras
    │
    ▼
Opt2Visitor  ──► Etiquetado Sethi-Ullman (reducción de carga)
    │
    ▼
GenCodeVisitor ──► Ensamblador x86-64 (.s)
```

El flujo está en `main.cpp:72-84`. Las tres fases de optimización recorren el AST con el **patrón Visitor** antes de emitir código.

---

## 1. GenCode de la Potencia

El operador `**` se mapea al `BinaryOp::POW_OP`. La generación de código para este nodo vive en `visitor.cpp:906-932` (`GenCodeVisitor::visit(BinaryExp*)`).

### Casos de emisión

| Situación | Código emitido | Dónde |
|---|---|---|
| Ambos operandos son constantes | `movq $<resultado>, %rax` | Opt1Visitor (plegado previo) |
| Exponente conocido == 2 | `imulq %rax, %rax` | Reducción de fuerza |
| Exponente conocido == 4 | `imulq %rax, %rax` × 2 | Efecto cascada |
| Exponente variable o cualquier otro | `call potencia` | Función auxiliar recursiva |

### La función `potencia` (exponenciación rápida)

Cuando el exponente no es una constante conocida en tiempo de compilación, se emite una llamada a la subrutina `potencia`, que implementa **exponenciación binaria** (fast exponentiation) en O(log n):

```asm
potencia:                   # base → %rdi,  exp → %rsi
  pushq %rbp
  movq %rsp, %rbp
  cmpq $0, %rsi
  je potencia_n_zero        # exp == 0 → retorna 1
  cmpq $1, %rsi
  je potencia_n_one         # exp == 1 → retorna base
  pushq %rdi                # guarda base original
  movq %rsi, %rdx
  andq $1, %rdx             # bit menos significativo del exponente
  pushq %rdx
  movq %rdi, %rax
  imulq %rdi, %rax          # base² → nuevo argumento
  movq %rax, %rdi
  sarq $1, %rsi             # exp >> 1
  call potencia             # potencia(base², exp/2)
  popq %rdx                 # recupera el bit de paridad
  popq %rcx                 # recupera base original
  cmpq $0, %rdx
  je potencia_end           # exp era par → resultado listo
  imulq %rcx, %rax          # exp era impar → multiplica por base
```

La función **solo se emite** si `needPotencia == true` (flag en `visitor.h:281`), es decir, solo cuando alguna potencia con exponente no constante realmente se ejecuta. Esto evita emitir código muerto.

### Ejemplo — `input2.txt` (exponente en variable)

```
a = 2;
b = 3;
print(a ** b);   →   call potencia
```

Salida (`outputs/input_2.s`):
```asm
movq -8(%rbp), %rax   # base = a
pushq %rax
movq -16(%rbp), %rax  # exp  = b
movq %rax, %rcx
popq %rax
movq %rax, %rdi
movq %rcx, %rsi
call potencia
```

---

## 2. Reducción de carga (Strength Reduction)

La **reducción de fuerza** sustituye una operación costosa por una equivalente más barata. Está implementada en dos niveles:

### 2a. Plegado de constantes — `Opt1Visitor`

`Opt1Visitor` (`visitor.cpp:1361-1603`) recorre el AST marcando cada nodo con `isConstant=true` y `constantValue=<valor>` cuando el resultado se puede determinar en tiempo de compilación.

Operaciones cubiertas: `+`, `-`, `*`, `**` (todos los operadores sobre literales).

```
print(3 ** 3)  →  constantValue = 27  →  movq $27, %rax
```

También hace **inlining de funciones puras**: si todos los argumentos de una llamada son constantes y la función retorna una constante, la llamada entera se colapsa a un literal (`visitor.cpp:1541-1585`).

```
fun int triple(int x)  return(3 * x)  endfun
print(triple(5))  →  constantValue = 15  →  movq $15, %rax
```

La función `triple` no llega a emitirse porque `liveUserFunctions` (eliminación de código muerto) la descarta.

### 2b. Reducción de fuerza específica para `**`

Cuando el exponente es la constante 2, se evita la llamada a `potencia` y se sustituye por una sola multiplicación entera:

```
n ** 2  →  movq n(%rbp), %rax
           imulq %rax, %rax     ← una sola imulq en lugar de call potencia
```

Código en `visitor.cpp:910-912`:
```cpp
if (rightIsConst && expVal == 2) {
    exp->left->accept(this);
    out << "  imulq %rax, %rax\n";
}
```

### 2c. Reducción de carga en expresiones binarias — `Opt2Visitor` (Sethi-Ullman)

`Opt2Visitor` (`visitor.cpp:1612-1793`) implementa el algoritmo de **etiquetado Sethi-Ullman**: asigna a cada nodo del AST un número de registros necesarios para evaluarlo sin spill.

```
- Hoja (NumberExp):         label = asignado al llegar (0 ó 1 según posición)
- Nodo interno (BinaryExp): si label(left) == label(right)  → label = label(left) + 1
                            si label(left) != label(right)  → label = max(left, right)
```

`GenCodeVisitor::visit(BinaryExp*)` usa estos labels para decidir el **orden de evaluación**:

```cpp
if (l >= r) {
    // evalúa izquierda primero (más costosa → al registro %rax)
    exp->left->accept(this);
    out << "  pushq %rax\n";
    exp->right->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  popq %rax\n";
} else {
    // evalúa derecha primero y luego intercambia
    exp->right->accept(this);
    out << "  pushq %rax\n";
    exp->left->accept(this);
    ...
    out << "  xchgq %rax, %rcx\n";
}
```

Esto minimiza los spills a la pila evaluando primero el subárbol que requiere más registros.

---

## 3. Efecto Cascada

El **efecto cascada** aprovecha que `x**4 = (x**2)**2`: en lugar de llamar a `potencia` o de calcular el cuadrado dos veces desde cero, se reutiliza el resultado intermedio de `x**2` para computar `x**4` con **solo dos multiplicaciones en línea**.

Código en `visitor.cpp:913-917`:
```cpp
} else if (rightIsConst && expVal == 4) {
    // Efecto Cascada: x**4 = (x**2)**2
    exp->left->accept(this);
    out << "  imulq %rax, %rax\n";   // %rax = x²
    out << "  imulq %rax, %rax\n";   // %rax = x⁴
}
```

### Ejemplo — `input4.txt`

```
n = 4;
print(n ** 4 + n ** 2);
```

Salida (`outputs/input_4.s`):
```asm
movq -8(%rbp), %rax   # n
imulq %rax, %rax      # n²  ← primer imulq (cascada)
imulq %rax, %rax      # n⁴  ← segundo imulq (cascada)
pushq %rax
movq -8(%rbp), %rax   # n
imulq %rax, %rax      # n²  ← reducción de fuerza para **2
movq %rax, %rcx
popq %rax
addq %rcx, %rax
```

No se emite ningún `call potencia`. Ambas potencias se resuelven inline con `imulq`.

### Por qué no `x**8` de forma similar

La implementación actual solo reconoce los casos `exp==2` y `exp==4` explícitamente. Para exponentes distintos se delega a `potencia`, que ya es O(log n) y no necesita expansión inline.

---

## Resumen de las tres optimizaciones

| Optimización | Clase | Condición de activación | Resultado |
|---|---|---|---|
| Plegado de constantes | `Opt1Visitor` | Ambos operandos son literales o funciones puras con args constantes | `movq $<valor>, %rax` — sin operación en tiempo de ejecución |
| Reducción de fuerza | `GenCodeVisitor` | `exp == 2` y es constante | `imulq %rax, %rax` — sin `call` |
| Efecto cascada | `GenCodeVisitor` | `exp == 4` y es constante | Dos `imulq %rax, %rax` encadenados — sin `call` |
| Sethi-Ullman | `Opt2Visitor` + `GenCodeVisitor` | Toda expresión binaria | Evalúa el subárbol más pesado primero, reduce spills |
| Eliminación de código muerto | `Opt1Visitor::computeLiveness` | Función cuya única llamada fue plegada | La función no se emite en el `.s` |

---

## Compilar y ejecutar

```bash
# Compilar el compilador
g++ -std=c++17 -o compilador main.cpp scanner.cpp parser.cpp ast.cpp visitor.cpp token.cpp

# Compilar un programa fuente
./compilador inputs/input4.txt

# Ensamblar y ejecutar
gcc -o resultado inputs/input4.s && ./resultado
```

### Ejecutar todos los casos de prueba

```bash
python3 run_all_inputs.py
```

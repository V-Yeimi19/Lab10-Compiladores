// =============================================================================
// visitor.cpp — Implementación de TypeCheckerVisitor y GenCodeVisitor
// =============================================================================

#include "visitor.h"
#include "ast.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <unordered_map>

// =============================================================================
// Despacho del patrón Visitor (accept en cada nodo del AST)
// =============================================================================

int BinaryExp::accept(Visitor *v)    { return v->visit(this); }
int NumberExp::accept(Visitor *v)    { return v->visit(this); }
int IdExp::accept(Visitor *v)        { return v->visit(this); }
int Program::accept(Visitor *v)      { return v->visit(this); }
int PrintStm::accept(Visitor *v)     { return v->visit(this); }
int AssignStm::accept(Visitor *v)    { return v->visit(this); }
int ExpListSize::accept(Visitor *v)  { return v->visit(this); }
int ExpListVals::accept(Visitor *v)  { return v->visit(this); }
int ExpMatrixSize::accept(Visitor *v){ return v->visit(this); }
int ExpMatrixVals::accept(Visitor *v){ return v->visit(this); }
int IfStm::accept(Visitor *v)        { return v->visit(this); }
int WhileStm::accept(Visitor *v)     { return v->visit(this); }
int Body::accept(Visitor *v)         { return v->visit(this); }
int VarDec::accept(Visitor *v)       { return v->visit(this); }
int FcallExp::accept(Visitor *v)     { return v->visit(this); }
int FunDec::accept(Visitor *v)       { return v->visit(this); }
int ReturnStm::accept(Visitor *v)    { return v->visit(this); }
int DoWhileStm::accept(Visitor *v)   { return v->visit(this); }
int BreakStm::accept(Visitor *v)     { return v->visit(this); }
int SwitchStm::accept(Visitor *v)    { return v->visit(this); }
int UnaryExp::accept(Visitor *v)     { return v->visit(this); }
int IndexExp::accept(Visitor *v)     { return v->visit(this); }
int MatrixExp::accept(Visitor *v)    { return v->visit(this); }
int FieldExp::accept(Visitor *v)     { return v->visit(this); }
int StructDec::accept(Visitor *v)    { return v->visit(this); }

// =============================================================================
// TypeCheckerVisitor — Análisis semántico
// =============================================================================

int TypeCheckerVisitor::TypeChecker(Program *program) {
  for (auto sd : program->sdlist)
    sd->accept(this);

  for (auto fd : program->fdlist)
    funAridad[fd->nombre] = int(fd->Pnombres.size());

  for (auto fd : program->fdlist)
    fd->accept(this);

  return 0;
}

int TypeCheckerVisitor::visit(FunDec *fd) {
  funcionActual = fd->nombre;
  locales = 0;
  int parametros = int(fd->Pnombres.size());

  entorno.add_level();
  tiposVar.add_level();

  for (size_t i = 0; i < fd->Pnombres.size(); ++i) {
    entorno.add_var(fd->Pnombres[i], 0);
    tiposVar.add_var(fd->Pnombres[i], fd->Ptipos[i]);
  }

  fd->cuerpo->accept(this);

  tiposVar.remove_level();
  entorno.remove_level();

  funcontador[fd->nombre] = parametros + locales;
  return 0;
}

int TypeCheckerVisitor::visit(Body *body) {
  entorno.add_level();
  tiposVar.add_level();

  for (auto dec : body->declarations)
    dec->accept(this);
  for (auto stm : body->StmList)
    stm->accept(this);

  tiposVar.remove_level();
  entorno.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(VarDec *vd) {
  for (auto &nombre : vd->vars) {
    if (entorno.check(nombre)) {
      std::cerr << "[TypeChecker] Advertencia: la variable '" << nombre
                << "' ya fue declarada en este scope"
                << " (en función '" << funcionActual << "').\n";
    }
    entorno.add_var(nombre, 0);
    tiposVar.add_var(nombre, vd->type);
    locales++;
  }
  return 0;
}

int TypeCheckerVisitor::visit(StructDec *sd) {
  if (structFields.count(sd->name))
    throw std::runtime_error("[TypeChecker] Estructura duplicada: '" + sd->name + "'");

  int totalFields = 0;
  std::unordered_map<std::string, bool> names;
  std::unordered_map<std::string, int>  offsets;
  for (auto field : sd->fields) {
    for (auto &name : field->vars) {
      if (names.count(name))
        throw std::runtime_error("[TypeChecker] Campo duplicado '" + name +
                                 "' en estructura '" + sd->name + "'");
      names[name]   = true;
      offsets[name] = totalFields * 8;
      totalFields++;
    }
  }

  structFields[sd->name]       = totalFields;
  structFieldOffsets[sd->name] = offsets;
  return 0;
}

int TypeCheckerVisitor::visit(IdExp *exp) {
  if (!entorno.check(exp->value))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->value + "' usada en la función '" +
                             funcionActual + "'");
  return 0;
}

int TypeCheckerVisitor::visit(AssignStm *stm) {
  stm->target->accept(this);
  stm->e->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpListSize *stm) {
  stm->size->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpListVals *stm) {
  if (structFields.count(stm->type)) {
    int esperados = structFields[stm->type];
    int recibidos = int(stm->values.size());
    if (recibidos != 0 && recibidos != esperados)
      throw std::runtime_error("[TypeChecker] La estructura '" + stm->type +
                               "' espera " + std::to_string(esperados) +
                               " valor(es), pero se pasaron " +
                               std::to_string(recibidos));
  }
  for (auto value : stm->values)
    value->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpMatrixSize *stm) {
  stm->rows->accept(this);
  stm->cols->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpMatrixVals *stm) {
  stm->rows->accept(this);
  stm->cols->accept(this);
  for (auto value : stm->values)
    value->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(FcallExp *fcall) {
  if (funAridad.find(fcall->nombre) == funAridad.end())
    throw std::runtime_error("[TypeChecker] Función no definida: '" +
                             fcall->nombre + "' llamada en '" + funcionActual + "'");

  int esperados = funAridad[fcall->nombre];
  int recibidos = int(fcall->argumentos.size());
  if (recibidos != esperados)
    throw std::runtime_error("[TypeChecker] La función '" + fcall->nombre +
                             "' espera " + std::to_string(esperados) +
                             " argumento(s), pero se pasaron " +
                             std::to_string(recibidos));

  for (auto arg : fcall->argumentos)
    arg->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(IfStm *stm) {
  stm->condition->accept(this);

  int base = locales;

  locales = 0;
  stm->then->accept(this);
  int maxLocales = locales;

  if (stm->els) {
    locales = 0;
    stm->els->accept(this);
    maxLocales = std::max(maxLocales, locales);
  }

  locales = base + maxLocales;
  return 0;
}

int TypeCheckerVisitor::visit(WhileStm *stm) {
  stm->condition->accept(this);
  stm->b->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(PrintStm *stm) {
  stm->e->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ReturnStm *r) {
  r->e->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(BinaryExp *exp) {
  exp->left->accept(this);
  exp->right->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(UnaryExp *exp) {
  exp->operand->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(IndexExp *exp) {
  if (!entorno.check(exp->name))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->name + "' usada en la función '" +
                             funcionActual + "'");
  exp->index->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(MatrixExp *exp) {
  if (!entorno.check(exp->name))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->name + "' usada en la función '" +
                             funcionActual + "'");
  exp->row->accept(this);
  exp->col->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(FieldExp *exp) {
  std::string type;
  if (!tiposVar.lookup(exp->object, type))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->object + "' usada en la función '" +
                             funcionActual + "'");
  if (!structFieldOffsets.count(type))
    throw std::runtime_error("[TypeChecker] La variable '" + exp->object +
                             "' no es una estructura");
  if (!structFieldOffsets[type].count(exp->field))
    throw std::runtime_error("[TypeChecker] La estructura '" + type +
                             "' no tiene campo '" + exp->field + "'");
  return 0;
}

int TypeCheckerVisitor::visit(NumberExp *) { return 0; }
int TypeCheckerVisitor::visit(Program *)     { return 0; }

int TypeCheckerVisitor::visit(DoWhileStm *stm) {
  stm->condition->accept(this);
  stm->b->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(BreakStm *) { return 0; }

int TypeCheckerVisitor::visit(SwitchStm *stm) {
  stm->e->accept(this);
  for (auto c : stm->cases)
    for (auto s : c->body)
      s->accept(this);
  for (auto s : stm->default_body)
    s->accept(this);
  return 0;
}

// =============================================================================
// GenCodeVisitor — Generación de código ensamblador x86-64 (AT&T syntax)
// =============================================================================

// -----------------------------------------------------------------------------
// captureLVal — visita exp en "modo captura" y devuelve el LVal resultante.
//
// Antes: bool captureMode + 11 campos sueltos en la clase.
// Ahora: un puntero lvalTarget apunta al LVal que se está llenando; los
//        visit() que actúan como lvalue detectan lvalTarget != nullptr y
//        solo rellenan el struct en lugar de emitir código.
// -----------------------------------------------------------------------------
LVal GenCodeVisitor::captureLVal(Exp *exp) {
  LVal lv;
  lvalTarget = &lv;
  exp->accept(this);
  lvalTarget = nullptr;
  return lv;
}

// -----------------------------------------------------------------------------
// storeTarget — switch sobre lv.kind en lugar del mapa de punteros a función.
// -----------------------------------------------------------------------------
int GenCodeVisitor::storeTarget(const LVal &lv) {
  switch (lv.kind) {
  case LValKind::Id:     return storeId(lv);
  case LValKind::Index:  return storeIndex(lv);
  case LValKind::Matrix: return storeMatrix(lv);
  case LValKind::Field:  return storeField(lv);
  default:
    throw std::runtime_error("Asignacion a una expresion que no es lvalue");
  }
}

// -----------------------------------------------------------------------------
// generar — punto de entrada de la generación
// -----------------------------------------------------------------------------
int GenCodeVisitor::generar(Program *program) {
  tipos.TypeChecker(program);
  funcontador       = tipos.funcontador;
  structFields      = tipos.structFields;
  structFieldOffsets= tipos.structFieldOffsets;
  for (auto fd : program->fdlist) {
    funParamNames[fd->nombre] = fd->Pnombres;
    funParamTypes[fd->nombre] = fd->Ptipos;
  }
  program->accept(this);
  return 0;
}

// -----------------------------------------------------------------------------
// visit(Program)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(Program *program) {
  out << ".data\n";
  out << "print_fmt: .string \"%ld \\n\"\n";

  for (auto dec : program->vdlist)
    dec->accept(this);

  for (std::unordered_map<std::string, bool>::const_iterator it =
           memoriaGlobal.begin();
       it != memoriaGlobal.end(); ++it)
    out << it->first << ": .quad 0\n";
  for (auto fd : program->fdlist) {
    for (size_t i = 0; i < fd->Ptipos.size(); ++i) {
      if (fd->Ptipos[i] == "matrix")
        out << "__cols_" << fd->nombre << "_" << fd->Pnombres[i]
            << ": .quad 0\n";
    }
  }

  out << "\n.text\n";
  for (auto fd : program->fdlist)
    fd->accept(this);

if (needPotencia) {
    out << "\n.globl potencia\n";
    out << "potencia:\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    out << "  cmpq $0, %rsi\n";
    out << "  je potencia_n_zero\n";
    out << "  cmpq $1, %rsi\n";
    out << "  je potencia_n_one\n";
    out << "  pushq %rdi\n";
    out << "  movq %rsi, %rdx\n";
    out << "  andq $1, %rdx\n";
    out << "  pushq %rdx\n";
    out << "  movq %rdi, %rax\n";
    out << "  imulq %rdi, %rax\n";
    out << "  movq %rax, %rdi\n";
    out << "  sarq $1, %rsi\n";
    out << "  call potencia\n";
    out << "  popq %rdx\n";
    out << "  popq %rcx\n";
    out << "  cmpq $0, %rdx\n";
    out << "  je potencia_end\n";
    out << "  imulq %rcx, %rax\n";
    out << "  jmp potencia_end\n";
    out << "potencia_n_zero:\n";
    out << "  movq $1, %rax\n";
    out << "  jmp potencia_end\n";
    out << "potencia_n_one:\n";
    out << "  movq %rdi, %rax\n";
    out << "  jmp potencia_end\n";
    out << "potencia_end:\n";
    out << "  leave\n";
    out << "  ret\n";
}
  
  out << "\n.section .note.GNU-stack,\"\",@progbits\n";
  return 0;
}

int GenCodeVisitor::visit(StructDec *) { return 0; }

// -----------------------------------------------------------------------------
// visit(VarDec) — registra variables en memoria (global o local)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(VarDec *stm) {
  for (auto &var : stm->vars) {
    if (!entornoFuncion) {
      memoriaGlobal[var]  = true;
      variableTypes[var]  = stm->type;
      if (structFields.count(stm->type))
        structAllocated[var] = false;
    } else {
      memoria[var]        = offset;
      variableTypes[var]  = stm->type;
      if (structFields.count(stm->type))
        structAllocated[var] = false;
      offset -= 8;
    }
  }
  return 0;
}

// -----------------------------------------------------------------------------
// visit(NumberExp)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(NumberExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind       = LValKind::Number;
    lvalTarget->numIsConst = true;
    lvalTarget->numVal     = exp->value;
    return 0;
  }
  out << "  movq $" << exp->value << ", %rax\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(IdExp)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(IdExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind = LValKind::Id;
    lvalTarget->name = exp->value;
    return 0;
  }
  if (memoriaGlobal.count(exp->value))
    out << "  movq " << exp->value << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[exp->value] << "(%rbp), %rax\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(IndexExp)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(IndexExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind  = LValKind::Index;
    lvalTarget->name  = exp->name;
    lvalTarget->index = exp->index;
    return 0;
  }
  exp->index->accept(this);
  out << "  movq %rax, %rdi\n";
  if (memoriaGlobal.count(exp->name))
    out << "  movq " << exp->name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[exp->name] << "(%rbp), %rax\n";
  out << "  movq (%rax, %rdi, 8), %rax\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(MatrixExp)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(MatrixExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind = LValKind::Matrix;
    lvalTarget->name = exp->name;
    lvalTarget->row  = exp->row;
    lvalTarget->col  = exp->col;
    return 0;
  }
  exp->row->accept(this);
  out << "  pushq %rax\n";
  exp->col->accept(this);
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";

  if (matrixColumns.count(exp->name))
    out << "  movq $" << matrixColumns[exp->name] << ", %rcx\n";
  else if (currentMatrixParamLabels.count(exp->name))
    out << "  movq " << currentMatrixParamLabels[exp->name] << "(%rip), %rcx\n";
  else
    out << "  movq $0, %rcx\n";

  out << "  imulq %rcx, %rax\n";
  out << "  addq %rdi, %rax\n";
  out << "  movq %rax, %rdi\n";

  if (memoriaGlobal.count(exp->name))
    out << "  movq " << exp->name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[exp->name] << "(%rbp), %rax\n";

  out << "  movq (%rax, %rdi, 8), %rax\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(FieldExp)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(FieldExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind   = LValKind::Field;
    lvalTarget->object = exp->object;
    lvalTarget->field  = exp->field;
    return 0;
  }
  std::string type     = variableTypes[exp->object];
  int fieldIndex       = structFieldOffsets[type][exp->field] / 8;

  out << "  movq $" << fieldIndex << ", %rdi\n";
  if (memoriaGlobal.count(exp->object))
    out << "  movq " << exp->object << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[exp->object] << "(%rbp), %rax\n";
  out << "  movq (%rax, %rdi, 8), %rax\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(ExpListSize)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(ExpListSize *stm) {
  if (lvalTarget) {
    lvalTarget->kind  = LValKind::ListSize;
    lvalTarget->type  = stm->type;
    lvalTarget->index = stm->size;
    return 0;
  }
  stm->size->accept(this);
  out << "  movq $8, %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  movq %rax, %rdi\n"
      << "  call malloc@PLT\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(ExpListVals)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(ExpListVals *stm) {
  if (lvalTarget) {
    lvalTarget->kind   = LValKind::ListVals;
    lvalTarget->type   = stm->type;
    lvalTarget->values = stm->values;
    return 0;
  }
  int n = stm->values.size();
  out << "  movq $" << n << ", %rax\n";
  out << "  movq $8, %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  call malloc@PLT\n";
  out << "  pushq %rax\n";
  for (size_t i = 0; i < (size_t)n; ++i) {
    stm->values[i]->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  movq $" << i << ", %rdi\n";
    out << "  popq %rax\n";
    out << "  movq %rcx, (%rax, %rdi, 8)\n";
    if (i + 1 < (size_t)n)
      out << "  pushq %rax\n";
  }
  return 0;
}

// -----------------------------------------------------------------------------
// visit(ExpMatrixSize)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(ExpMatrixSize *stm) {
  if (lvalTarget) {
    lvalTarget->kind = LValKind::MatrixSize;
    lvalTarget->type = stm->type;
    lvalTarget->row  = stm->rows;
    lvalTarget->col  = stm->cols;
    return 0;
  }
  stm->cols->accept(this);
  out << "  pushq %rax\n";
  stm->rows->accept(this);
  out << "  popq %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  salq $3, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  call malloc@PLT\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(ExpMatrixVals)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(ExpMatrixVals *stm) {
  if (lvalTarget) {
    lvalTarget->kind   = LValKind::MatrixVals;
    lvalTarget->type   = stm->type;
    lvalTarget->row    = stm->rows;
    lvalTarget->col    = stm->cols;
    lvalTarget->values = stm->values;
    return 0;
  }
  stm->cols->accept(this);
  out << "  pushq %rax\n";
  stm->rows->accept(this);
  out << "  popq %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  salq $3, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  call malloc@PLT\n";
  out << "  pushq %rax\n";
  for (size_t i = 0; i < stm->values.size(); ++i) {
    out << "  pushq %rax\n";
    stm->values[i]->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  popq %rax\n";
    out << "  movq %rcx, " << (i * 8) << "(%rax)\n";
  }
  out << "  popq %rax\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(AssignStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(AssignStm *stm) {
  LVal target = captureLVal(stm->target);

  // Solo capturar el RHS si es un nodo de inicialización especial.
  // Para cualquier otra expresión (BinaryExp, IdExp, FcallExp, etc.)
  // captureLVal emitiría código real porque esos visit() no tienen rama
  // lvalTarget — en ese caso se va directo al caso general al final.
  bool specialRhs = stm->e->isSpecialRhs();

  if (target.kind == LValKind::Id && specialRhs) {
    const std::string &targetName = target.name;
    LVal rhs = captureLVal(stm->e);

    if (rhs.kind == LValKind::ListVals) {
      if (structFields.count(rhs.type)) {
        // Struct literal con llaves
        int fields = structFields[rhs.type];
        out << "  movq $" << (fields * 8) << ", %rdi\n";
        out << "  call malloc@PLT\n";
        if (memoriaGlobal.count(targetName))
          out << "  movq %rax, " << targetName << "(%rip)\n";
        else
          out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";
        structAllocated[targetName] = true;

        for (size_t i = 0; i < rhs.values.size(); ++i) {
          rhs.values[i]->accept(this);
          out << "  pushq %rax\n";
          out << "  movq $" << i << ", %rax\n";
          out << "  movq %rax, %rdi\n";
          out << "  popq %rax\n";
          out << "  movq %rax, %rcx\n";
          if (memoriaGlobal.count(targetName))
            out << "  movq " << targetName << "(%rip), %rax\n";
          else
            out << "  movq " << memoria[targetName] << "(%rbp), %rax\n";
          out << "  movq %rcx, (%rax, %rdi, 8)\n";
        }
        return 0;
      }

      // Lista con valores literales
      int n = rhs.values.size();
      out << "  movq $" << n << ", %rax\n";
      out << "  movq $8, %rcx\n";
      out << "  imulq %rcx, %rax\n";
      out << "  movq %rax, %rdi\n";
      out << "  call malloc@PLT\n";
      if (memoriaGlobal.count(targetName))
        out << "  movq %rax, " << targetName << "(%rip)\n";
      else
        out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";

      for (size_t i = 0; i < rhs.values.size(); ++i) {
        rhs.values[i]->accept(this);
        out << "  pushq %rax\n";
        out << "  movq $" << i << ", %rax\n";
        out << "  movq %rax, %rdi\n";
        out << "  popq %rax\n";
        out << "  movq %rax, %rcx\n";
        if (memoriaGlobal.count(targetName))
          out << "  movq " << targetName << "(%rip), %rax\n";
        else
          out << "  movq " << memoria[targetName] << "(%rbp), %rax\n";
        out << "  movq %rcx, (%rax, %rdi, 8)\n";
      }
      return 0;
    }

    if (rhs.kind == LValKind::MatrixSize) {
      // Capturar columnas para acceso matricial posterior
      LVal colLV = captureLVal(rhs.col);
      if (colLV.numIsConst)
        matrixColumns[targetName] = colLV.numVal;

      rhs.col->accept(this);
      out << "  pushq %rax\n";
      rhs.row->accept(this);
      out << "  popq %rcx\n";
      out << "  imulq %rcx, %rax\n";
      out << "  salq $3, %rax\n";
      out << "  movq %rax, %rdi\n";
      out << "  call malloc@PLT\n";
      if (memoriaGlobal.count(targetName))
        out << "  movq %rax, " << targetName << "(%rip)\n";
      else
        out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";
      return 0;
    }

    if (rhs.kind == LValKind::MatrixVals) {
      LVal colLV = captureLVal(rhs.col);
      int cols = colLV.numIsConst ? colLV.numVal : 0;
      if (cols > 0)
        matrixColumns[targetName] = cols;

      rhs.col->accept(this);
      out << "  pushq %rax\n";
      rhs.row->accept(this);
      out << "  popq %rcx\n";
      out << "  imulq %rcx, %rax\n";
      out << "  salq $3, %rax\n";
      out << "  movq %rax, %rdi\n";
      out << "  call malloc@PLT\n";
      if (memoriaGlobal.count(targetName))
        out << "  movq %rax, " << targetName << "(%rip)\n";
      else
        out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";

      for (size_t i = 0; i < rhs.values.size(); ++i) {
        rhs.values[i]->accept(this);
        out << "  pushq %rax\n";
        if (cols > 0) {
          out << "  movq $" << (i / cols) << ", %rax\n";
          out << "  pushq %rax\n";
          out << "  movq $" << (i % cols) << ", %rax\n";
        } else {
          out << "  movq $0, %rax\n";
          out << "  pushq %rax\n";
          out << "  movq $" << i << ", %rax\n";
        }
        out << "  movq %rax, %rdi\n";
        out << "  popq %rax\n";
        if (matrixColumns.count(targetName))
          out << "  movq $" << matrixColumns[targetName] << ", %rcx\n";
        else if (currentMatrixParamLabels.count(targetName))
          out << "  movq " << currentMatrixParamLabels[targetName] << "(%rip), %rcx\n";
        else
          out << "  movq $0, %rcx\n";
        out << "  imulq %rcx, %rax\n";
        out << "  addq %rdi, %rax\n";
        out << "  movq %rax, %rdi\n";
        out << "  popq %rcx\n";
        if (memoriaGlobal.count(targetName))
          out << "  movq " << targetName << "(%rip), %rax\n";
        else
          out << "  movq " << memoria[targetName] << "(%rbp), %rax\n";
        out << "  movq %rcx, (%rax, %rdi, 8)\n";
      }
      return 0;
    }
  }  // fin if (target.kind == LValKind::Id && isSpecialRhs)

  // Caso general: evalúa RHS → %rax y almacena según el tipo de lvalue
  stm->e->accept(this);
  storeTarget(target);
  return 0;
}

// -----------------------------------------------------------------------------
// storeId
// -----------------------------------------------------------------------------
int GenCodeVisitor::storeId(const LVal &lv) {
  if (memoriaGlobal.count(lv.name))
    out << "  movq %rax, " << lv.name << "(%rip)\n";
  else
    out << "  movq %rax, " << memoria[lv.name] << "(%rbp)\n";
  return 0;
}

// -----------------------------------------------------------------------------
// storeIndex
// -----------------------------------------------------------------------------
int GenCodeVisitor::storeIndex(const LVal &lv) {
  out << "  pushq %rax\n";
  lv.index->accept(this);
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";
  out << "  movq %rax, %rcx\n";
  if (memoriaGlobal.count(lv.name))
    out << "  movq " << lv.name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[lv.name] << "(%rbp), %rax\n";
  out << "  movq %rcx, (%rax, %rdi, 8)\n";
  return 0;
}

// -----------------------------------------------------------------------------
// storeMatrix
// -----------------------------------------------------------------------------
int GenCodeVisitor::storeMatrix(const LVal &lv) {
  out << "  pushq %rax\n";
  lv.row->accept(this);
  out << "  pushq %rax\n";
  lv.col->accept(this);
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";

  if (matrixColumns.count(lv.name))
    out << "  movq $" << matrixColumns[lv.name] << ", %rcx\n";
  else if (currentMatrixParamLabels.count(lv.name))
    out << "  movq " << currentMatrixParamLabels[lv.name] << "(%rip), %rcx\n";
  else
    out << "  movq $0, %rcx\n";

  out << "  imulq %rcx, %rax\n";
  out << "  addq %rdi, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  popq %rcx\n";

  if (memoriaGlobal.count(lv.name))
    out << "  movq " << lv.name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[lv.name] << "(%rbp), %rax\n";
  out << "  movq %rcx, (%rax, %rdi, 8)\n";
  return 0;
}

// -----------------------------------------------------------------------------
// storeField
// -----------------------------------------------------------------------------
int GenCodeVisitor::storeField(const LVal &lv) {
  std::string type = variableTypes[lv.object];
  int fieldIndex   = structFieldOffsets[type][lv.field] / 8;

  out << "  pushq %rax\n";
  out << "  movq $" << fieldIndex << ", %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";
  out << "  movq %rax, %rcx\n";

  if (structAllocated.count(lv.object) && !structAllocated[lv.object]) {
    int fields = structFields[type];
    out << "  pushq %rcx\n";
    out << "  pushq %rdi\n";
    out << "  movq $" << (fields * 8) << ", %rdi\n";
    out << "  call malloc@PLT\n";
    if (memoriaGlobal.count(lv.object))
      out << "  movq %rax, " << lv.object << "(%rip)\n";
    else
      out << "  movq %rax, " << memoria[lv.object] << "(%rbp)\n";
    out << "  popq %rdi\n";
    out << "  popq %rcx\n";
    structAllocated[lv.object] = true;
  }

  if (memoriaGlobal.count(lv.object))
    out << "  movq " << lv.object << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[lv.object] << "(%rbp), %rax\n";
  out << "  movq %rcx, (%rax, %rdi, 8)\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(BinaryExp)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(BinaryExp *exp) {


  if (exp->isConstant) {
    out << "  movq $" << exp->constantValue << ", %rax\n";
    return 0;
  }

  if (exp->op == POW_OP) {
    bool rightIsConst = exp->right->isConstant;
    int expVal = rightIsConst ? exp->right->constantValue : -1;

    if (rightIsConst && expVal == 2) {
      // Reducción de fuerza: x**2 = x * x
      exp->left->accept(this);
      out << "  imulq %rax, %rax\n";
    } else if (rightIsConst && expVal == 4) {
      // Efecto Cascada: x**4 = (x**2)**2
      exp->left->accept(this);
      out << "  imulq %rax, %rax\n";
      out << "  imulq %rax, %rax\n";
    } else {
      // call potencia
      exp->left->accept(this);
      out << "  pushq %rax\n";
      exp->right->accept(this);
      out << "  movq %rax, %rcx\n";
      out << "  popq %rax\n";
      out << "  movq %rax, %rdi\n";
      out << "  movq %rcx, %rsi\n";
      out << "  call potencia\n";
      needPotencia = true;
    }
    return 0;
  }
  else {
  int l = exp->left  ? exp->left->label  : 0;
  int r = exp->right ? exp->right->label : 0;

  if (l >= r) {
    exp->left->accept(this);
    out << "  pushq %rax\n";
    exp->right->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  popq %rax\n";
  } else {
    exp->right->accept(this);
    out << "  pushq %rax\n";
    exp->left->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  popq %rax\n";
    out << "  xchgq %rax, %rcx\n";
  }

  switch (exp->op) {
  case PLUS_OP:  out << "  addq %rcx, %rax\n";  break;
  case MINUS_OP: out << "  subq %rcx, %rax\n";  break;
  case MUL_OP:   out << "  imulq %rcx, %rax\n"; break;
  case DIV_OP:
    out << "  cqto\n";
    out << "  idivq %rcx\n";
    break;
  case LE_OP:
    out << "  cmpq %rcx, %rax\n";
    out << "  movq $0, %rax\n";
    out << "  setl %al\n";
    out << "  movzbq %al, %rax\n";
    break;
  case GT_OP:
    out << "  cmpq %rcx, %rax\n";
    out << "  movq $0, %rax\n";
    out << "  setg %al\n";
    out << "  movzbq %al, %rax\n";
    break;
  case LEQ_OP:
    out << "  cmpq %rcx, %rax\n";
    out << "  movq $0, %rax\n";
    out << "  setle %al\n";
    out << "  movzbq %al, %rax\n";
    break;
  case GEQ_OP:
    out << "  cmpq %rcx, %rax\n";
    out << "  movq $0, %rax\n";
    out << "  setge %al\n";
    out << "  movzbq %al, %rax\n";
    break;
  case EQ_OP:
    out << "  cmpq %rcx, %rax\n";
    out << "  movq $0, %rax\n";
    out << "  sete %al\n";
    out << "  movzbq %al, %rax\n";
    break;
  case NE_OP:
    out << "  cmpq %rcx, %rax\n";
    out << "  movq $0, %rax\n";
    out << "  setne %al\n";
    out << "  movzbq %al, %rax\n";
    break;
  case AND_OP: out << "  andq %rcx, %rax\n"; break;
  case OR_OP:  out << "  orq %rcx, %rax\n";  break;
  }
  }
  return 0;
}

// -----------------------------------------------------------------------------
// visit(UnaryExp)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(UnaryExp *exp) {
  exp->operand->accept(this);
  int lbl = labelcont++;
  out << "  cmpq $0, %rax\n";
  out << "  je not_true_" << lbl << "\n";
  out << "  movq $0, %rax\n";
  out << "  jmp not_end_" << lbl << "\n";
  out << "not_true_" << lbl << ":\n";
  out << "  movq $1, %rax\n";
  out << "not_end_" << lbl << ":\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(PrintStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(PrintStm *stm) {
  stm->e->accept(this);
  out << "  movq %rax, %rsi\n";
  out << "  leaq print_fmt(%rip), %rdi\n";
  out << "  movq $0, %rax\n";
  out << "  call printf@PLT\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(Body)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(Body *b) {
  for (auto dec : b->declarations)
    dec->accept(this);
  for (auto stm : b->StmList)
    stm->accept(this);
  return 0;
}

// -----------------------------------------------------------------------------
// visit(IfStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(IfStm *stm) {
  int lbl = labelcont++;
  stm->condition->accept(this);
  out << "  cmpq $0, %rax\n";
  out << "  je else_" << lbl << "\n";
  stm->then->accept(this);
  out << "  jmp endif_" << lbl << "\n";
  out << "else_" << lbl << ":\n";
  if (stm->els)
    stm->els->accept(this);
  out << "endif_" << lbl << ":\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(WhileStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(WhileStm *stm) {
  int lbl = labelcont++;
  std::string oldBreak = currentBreakLabel;
  currentBreakLabel = "endwhile_" + std::to_string(lbl);

  out << "while_" << lbl << ":\n";
  stm->condition->accept(this);
  out << "  cmpq $0, %rax\n";
  out << "  je endwhile_" << lbl << "\n";
  stm->b->accept(this);
  out << "  jmp while_" << lbl << "\n";
  out << "endwhile_" << lbl << ":\n";

  currentBreakLabel = oldBreak;
  return 0;
}

// -----------------------------------------------------------------------------
// visit(DoWhileStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(DoWhileStm *stm) {
  int lbl = labelcont++;
  std::string oldBreak = currentBreakLabel;
  currentBreakLabel = "endwhile_" + std::to_string(lbl);

  out << "dowhile_" << lbl << ":\n";
  stm->b->accept(this);
  stm->condition->accept(this);
  out << "  cmpq $0, %rax\n";
  out << "  jne dowhile_" << lbl << "\n";
  out << "endwhile_" << lbl << ":\n";

  currentBreakLabel = oldBreak;
  return 0;
}

// -----------------------------------------------------------------------------
// visit(ReturnStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(ReturnStm *stm) {
  stm->e->accept(this);
  out << "  jmp .end_" << nombreFuncion << "\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(BreakStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(BreakStm *) {
  if (currentBreakLabel.empty()) {
    std::cerr << "Error: break fuera de while, do-while o switch\n";
    exit(1);
  }
  out << "  jmp " << currentBreakLabel << "\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(SwitchStm)
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(SwitchStm *stm) {
  int lbl = labelcont++;
  stm->e->accept(this);
  out << "  movq %rax, %r10\n";

  for (auto c : stm->cases) {
    out << "  movq $" << c->value << ", %rax\n";
    out << "  cmpq %rax, %r10\n";
    out << "  je case_" << lbl << "_" << c->value << "\n";
  }

  if (!stm->default_body.empty())
    out << "  jmp default_" << lbl << "\n";
  else
    out << "  jmp endswitch_" << lbl << "\n";

  std::string oldBreak = currentBreakLabel;
  currentBreakLabel = "endswitch_" + std::to_string(lbl);

  for (auto c : stm->cases) {
    out << "case_" << lbl << "_" << c->value << ":\n";
    for (auto s : c->body)
      s->accept(this);
    out << "  jmp endswitch_" << lbl << "\n";
  }

  if (!stm->default_body.empty()) {
    out << "default_" << lbl << ":\n";
    for (auto s : stm->default_body)
      s->accept(this);
  }

  currentBreakLabel = oldBreak;
  out << "endswitch_" << lbl << ":\n";
  return 0;
}

// -----------------------------------------------------------------------------
// visit(FunDec) — prólogo, cuerpo y epílogo
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(FunDec *f) {
  if (!liveUserFunctions.empty() && !liveUserFunctions.count(f->nombre))
    return 0;

  entornoFuncion = true;
  memoria.clear();
  variableTypes.clear();
  structAllocated.clear();
  matrixColumns.clear();
  currentMatrixParamLabels.clear();
  offset = -8;
  nombreFuncion = f->nombre;

  const std::vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx",
                                            "%rcx", "%r8",  "%r9"};

  out << "\n.globl " << f->nombre << "\n";
  out << f->nombre << ":\n";
  out << "  pushq %rbp\n";
  out << "  movq %rsp, %rbp\n";
  out << "  subq $" << funcontador[f->nombre] * 8 << ", %rsp\n";

  int nParams = int(f->Pnombres.size());
  for (int i = 0; i < nParams; i++) {
    memoria[f->Pnombres[i]]       = offset;
    variableTypes[f->Pnombres[i]] = f->Ptipos[i];
    if (f->Ptipos[i] == "matrix")
      currentMatrixParamLabels[f->Pnombres[i]] =
          "__cols_" + f->nombre + "_" + f->Pnombres[i];
    out << "  movq " << argRegs[i] << ", " << offset << "(%rbp)\n";
    offset -= 8;
  }

  for (auto dec : f->cuerpo->declarations)
    dec->accept(this);

  for (auto stm : f->cuerpo->StmList)
    stm->accept(this);

  out << ".end_" << f->nombre << ":\n";
  out << "  leave\n";
  out << "  ret\n";

  entornoFuncion = false;
  return 0;
}

// -----------------------------------------------------------------------------
// visit(FcallExp) — llamada a función con argumentos en registros ABI SysV
// -----------------------------------------------------------------------------
int GenCodeVisitor::visit(FcallExp *exp) {
  if (exp->isConstant) {
    out << "  movq $" << exp->constantValue << ", %rax\n";
    return 0;
  }
  const std::vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx",
                                            "%rcx", "%r8",  "%r9"};
  int nArgs = int(exp->argumentos.size());
  for (int i = 0; i < nArgs; i++) {
    exp->argumentos[i]->accept(this);
    out << "  movq %rax, " << argRegs[i] << "\n";

    if (funParamTypes.count(exp->nombre) &&
        i < int(funParamTypes[exp->nombre].size()) &&
        funParamTypes[exp->nombre][i] == "matrix") {
      std::string targetLabel =
          "__cols_" + exp->nombre + "_" + funParamNames[exp->nombre][i];
      LVal argLV = captureLVal(exp->argumentos[i]);
      if (argLV.kind == LValKind::Id) {
        const std::string &argName = argLV.name;
        if (matrixColumns.count(argName)) {
          out << "  movq $" << matrixColumns[argName] << ", %r10\n";
          out << "  movq %r10, " << targetLabel << "(%rip)\n";
        } else if (currentMatrixParamLabels.count(argName)) {
          out << "  movq " << currentMatrixParamLabels[argName]
              << "(%rip), %r10\n";
          out << "  movq %r10, " << targetLabel << "(%rip)\n";
        }
      }
    }
  }
  out << "  call " << exp->nombre << "\n";
  return 0;
}

// ---------------------------------------------------------------------------
// Liveness helpers — find non-constant FcallExp reachable from live functions
// ---------------------------------------------------------------------------
static void liveCalls_stm(Stm *,
    const std::unordered_map<std::string, FunDec *> &,
    std::set<std::string> &, std::queue<std::string> &);

static void liveCalls_body(Body *,
    const std::unordered_map<std::string, FunDec *> &,
    std::set<std::string> &, std::queue<std::string> &);

static void liveCalls_exp(Exp *exp,
    const std::unordered_map<std::string, FunDec *> &funDefs,
    std::set<std::string> &live, std::queue<std::string> &work)
{
  if (!exp) return;
  if (auto *fc = dynamic_cast<FcallExp *>(exp)) {
    if (!fc->isConstant && !live.count(fc->nombre)) {
      live.insert(fc->nombre);
      work.push(fc->nombre);
    }
    for (auto arg : fc->argumentos)
      liveCalls_exp(arg, funDefs, live, work);
  } else if (auto *bin = dynamic_cast<BinaryExp *>(exp)) {
    liveCalls_exp(bin->left,  funDefs, live, work);
    liveCalls_exp(bin->right, funDefs, live, work);
  } else if (auto *un = dynamic_cast<UnaryExp *>(exp)) {
    liveCalls_exp(un->operand, funDefs, live, work);
  } else if (auto *idx = dynamic_cast<IndexExp *>(exp)) {
    liveCalls_exp(idx->index, funDefs, live, work);
  } else if (auto *mat = dynamic_cast<MatrixExp *>(exp)) {
    liveCalls_exp(mat->row, funDefs, live, work);
    liveCalls_exp(mat->col, funDefs, live, work);
  } else if (auto *els = dynamic_cast<ExpListSize *>(exp)) {
    liveCalls_exp(els->size, funDefs, live, work);
  } else if (auto *elv = dynamic_cast<ExpListVals *>(exp)) {
    for (auto v : elv->values) liveCalls_exp(v, funDefs, live, work);
  } else if (auto *ems = dynamic_cast<ExpMatrixSize *>(exp)) {
    liveCalls_exp(ems->rows, funDefs, live, work);
    liveCalls_exp(ems->cols, funDefs, live, work);
  } else if (auto *emv = dynamic_cast<ExpMatrixVals *>(exp)) {
    liveCalls_exp(emv->rows, funDefs, live, work);
    liveCalls_exp(emv->cols, funDefs, live, work);
    for (auto v : emv->values) liveCalls_exp(v, funDefs, live, work);
  }
}

static void liveCalls_stm(Stm *stm,
    const std::unordered_map<std::string, FunDec *> &funDefs,
    std::set<std::string> &live, std::queue<std::string> &work)
{
  if (!stm) return;
  if (auto *as = dynamic_cast<AssignStm *>(stm)) {
    liveCalls_exp(as->target, funDefs, live, work);
    liveCalls_exp(as->e,      funDefs, live, work);
  } else if (auto *ps = dynamic_cast<PrintStm *>(stm)) {
    liveCalls_exp(ps->e, funDefs, live, work);
  } else if (auto *rs = dynamic_cast<ReturnStm *>(stm)) {
    liveCalls_exp(rs->e, funDefs, live, work);
  } else if (auto *ws = dynamic_cast<WhileStm *>(stm)) {
    liveCalls_exp(ws->condition, funDefs, live, work);
    liveCalls_body(ws->b,        funDefs, live, work);
  } else if (auto *dws = dynamic_cast<DoWhileStm *>(stm)) {
    liveCalls_body(dws->b,         funDefs, live, work);
    liveCalls_exp(dws->condition,  funDefs, live, work);
  } else if (auto *ifs = dynamic_cast<IfStm *>(stm)) {
    liveCalls_exp(ifs->condition, funDefs, live, work);
    liveCalls_body(ifs->then,     funDefs, live, work);
    if (ifs->els) liveCalls_body(ifs->els, funDefs, live, work);
  } else if (auto *sw = dynamic_cast<SwitchStm *>(stm)) {
    liveCalls_exp(sw->e, funDefs, live, work);
    for (auto c : sw->cases)
      for (auto s : c->body)
        liveCalls_stm(s, funDefs, live, work);
    for (auto s : sw->default_body)
      liveCalls_stm(s, funDefs, live, work);
  }
}

static void liveCalls_body(Body *body,
    const std::unordered_map<std::string, FunDec *> &funDefs,
    std::set<std::string> &live, std::queue<std::string> &work)
{
  if (!body) return;
  for (auto stm : body->StmList)
    liveCalls_stm(stm, funDefs, live, work);
}

void Opt1Visitor::computeLiveness(Program *program)
{
  calledNonConst.clear();
  std::set<std::string> live = {"main"};
  std::queue<std::string> work;
  work.push("main");

  while (!work.empty()) {
    std::string fname = work.front();
    work.pop();
    if (!funDefs.count(fname)) continue;
    liveCalls_body(funDefs[fname]->cuerpo, funDefs, live, work);
  }

  live.erase("main");
  calledNonConst = live;
}
// ---------------------------------------------------------------------------

int Opt1Visitor::Opt1(Program *program)
{
  program->accept(this);
  computeLiveness(program);
  return 0;
}

int Opt1Visitor::visit(BinaryExp *exp)
{
    exp->left->accept(this);
    exp->right->accept(this);
    exp->isConstant = exp->left->isConstant and exp->right->isConstant;
    if (exp->isConstant)
    {
       switch (exp->op) {
      case PLUS_OP: {
       exp->constantValue = exp->left->constantValue + exp->right->constantValue;
       break;}
      case MINUS_OP:
      {
       exp->constantValue = exp->left->constantValue - exp->right->constantValue;
       break;} 
      
       case MUL_OP:
      {
       exp->constantValue = exp->left->constantValue * exp->right->constantValue;
       break;}
      case POW_OP: {
       int result = 1;
       int base   = exp->left->constantValue;
       int expo   = exp->right->constantValue;
       for (int i = 0; i < expo; i++) result *= base;
       exp->constantValue = result;
       break;}
    }
  }
  return 0;
}

int Opt1Visitor::visit(NumberExp *exp)
{
    exp->isConstant=true;
    exp->constantValue= exp->value;
  return 0;
}

int Opt1Visitor::visit(IdExp *exp)
{
  if (inFoldEval && paramConst.count(exp->value)) {
    exp->isConstant    = true;
    exp->constantValue = paramConst[exp->value];
  }
  return 0;
}

int Opt1Visitor::visit(UnaryExp *exp)
{
    return 0;
}

int Opt1Visitor::visit(IndexExp *exp)
{
    return 0;
}

int Opt1Visitor::visit(MatrixExp *exp)
{
    return 0;
}

int Opt1Visitor::visit(FieldExp *exp)
{
    return 0;
}

int Opt1Visitor::visit(Program *p)
{
  for(auto i:p->sdlist){
    i->accept(this);
  }
  for(auto i:p->vdlist){
    i->accept(this);
  }
  for(auto i:p->fdlist){
    i->accept(this);
  }
    return 0;
}

int Opt1Visitor::visit(StructDec *sd)
{
    for(auto i:sd->fields){
      i->accept(this);
    }
  return 0;
}

int Opt1Visitor::visit(PrintStm *stm)
{
    stm->e->accept(this);
    return 0;
}

int Opt1Visitor::visit(AssignStm *stm)
{
  stm->e->accept(this);
  return 0;
}

int Opt1Visitor::visit(ExpListSize *stm)
{
    return 0;
}

int Opt1Visitor::visit(ExpListVals *stm)
{
    return 0;
}

int Opt1Visitor::visit(ExpMatrixSize *stm)
{
    return 0;
}

int Opt1Visitor::visit(ExpMatrixVals *stm)
{
    return 0;
}

int Opt1Visitor::visit(WhileStm *stm)
{
  stm->condition->accept(this);
  stm->b->accept(this);
  return 0;
}

int Opt1Visitor::visit(DoWhileStm *stm)
{
  stm->b->accept(this);
  stm->condition->accept(this);
  return 0;
}

int Opt1Visitor::visit(IfStm *stm)
{
  stm->condition->accept(this);
  stm->then->accept(this);
  if (stm->els) stm->els->accept(this);
  return 0;
}

int Opt1Visitor::visit(BreakStm *stm)
{
    return 0;
}

int Opt1Visitor::visit(SwitchStm *stm)
{   
    stm->e->accept(this);
    for(auto j:stm->default_body){
          j->accept(this);
      }
    for(auto i:stm->cases){
      for(auto j:i->body){
          j->accept(this);
      }
    }
    return 0;
}

int Opt1Visitor::visit(Body *body)
{
    for(auto i:body->declarations){
      i->accept(this);
    }
    for(auto i:body->StmList){
      i->accept(this);
    }
  return 0;
}

int Opt1Visitor::visit(VarDec *vd)
{
  // no hay nada que hacer 
  return 0;
}

int Opt1Visitor::visit(FcallExp *fc)
{
  // Visitar argumentos primero (pueden plegarse recursivamente)
  bool allConst = true;
  for (auto arg : fc->argumentos) {
    arg->accept(this);
    if (!arg->isConstant) allConst = false;
  }

  if (allConst && funDefs.count(fc->nombre)) {
    FunDec *fd = funDefs[fc->nombre];
    // Guardar estado actual
    auto savedParam       = paramConst;
    bool savedFold        = inFoldEval;
    bool savedRetConst    = lastReturnIsConst;
    int  savedRetVal      = lastReturnValue;

    // Vincular parámetros a sus valores constantes
    paramConst.clear();
    for (size_t i = 0; i < fd->Pnombres.size(); i++)
      paramConst[fd->Pnombres[i]] = fc->argumentos[i]->constantValue;

    inFoldEval       = true;
    lastReturnIsConst = false;
    fd->cuerpo->accept(this);

    bool foldedConst = lastReturnIsConst;
    int  foldedVal   = lastReturnValue;

    // Restaurar estado
    paramConst        = savedParam;
    inFoldEval        = savedFold;
    lastReturnIsConst = savedRetConst;
    lastReturnValue   = savedRetVal;

    if (foldedConst) {
      fc->isConstant    = true;
      fc->constantValue = foldedVal;
      return 0;
    }
  }

  calledNonConst.insert(fc->nombre);
  return 0;
}

int Opt1Visitor::visit(ReturnStm *r)
{
  r->e->accept(this);
  if (inFoldEval) {
    lastReturnIsConst = r->e->isConstant;
    lastReturnValue   = r->e->constantValue;
  }
  return 0;
}

int Opt1Visitor::visit(FunDec *fd)
{
  funDefs[fd->nombre] = fd;
  if (!inFoldEval)
    fd->cuerpo->accept(this);
  return 0;
}


int Opt2Visitor::Opt2(Program *program)
{
  program->accept(this);
  return 0;
}

int Opt2Visitor::visit(BinaryExp *exp)
{
  exp->left->accept(this);
  exp->right->accept(this);
  if (exp->left->ishoja)
  {
    exp->left->label=1;
  }
  if (exp->right->ishoja)
  {
    exp->right->label=0;
  }
  if (exp->left->label==exp->right->label)
  {
    exp->label = exp->left->label+1;
  }
  else{
    exp->label = std::max(exp->left->label,exp->right->label);
  }

  return 0;
}

int Opt2Visitor::visit(NumberExp *exp)
{
    exp->ishoja=true;
  return 0;
}

int Opt2Visitor::visit(IdExp *exp)
{
    return 0;
}

int Opt2Visitor::visit(UnaryExp *exp)
{
    return 0;
}

int Opt2Visitor::visit(IndexExp *exp)
{
    return 0;
}

int Opt2Visitor::visit(MatrixExp *exp)
{
    return 0;
}

int Opt2Visitor::visit(FieldExp *exp)
{
    return 0;
}

int Opt2Visitor::visit(Program *p)
{
  for(auto i:p->sdlist){
    i->accept(this);
  }
  for(auto i:p->vdlist){
    i->accept(this);
  }
  for(auto i:p->fdlist){
    i->accept(this);
  }
    return 0;
}

int Opt2Visitor::visit(StructDec *sd)
{
    for(auto i:sd->fields){
      i->accept(this);
    }
  return 0;
}

int Opt2Visitor::visit(PrintStm *stm)
{
    stm->e->accept(this);
    return 0;
}

int Opt2Visitor::visit(AssignStm *stm)
{
  stm->e->accept(this);
  return 0;
}

int Opt2Visitor::visit(ExpListSize *stm)
{
    return 0;
}

int Opt2Visitor::visit(ExpListVals *stm)
{
    return 0;
}

int Opt2Visitor::visit(ExpMatrixSize *stm)
{
    return 0;
}

int Opt2Visitor::visit(ExpMatrixVals *stm)
{
    return 0;
}

int Opt2Visitor::visit(WhileStm *stm)
{
  stm->condition->accept(this);
  stm->b->accept(this);
  return 0;
}

int Opt2Visitor::visit(DoWhileStm *stm)
{
  stm->b->accept(this);
  stm->condition->accept(this);
  return 0;
}

int Opt2Visitor::visit(IfStm *stm)
{
  stm->condition->accept(this);
  stm->then->accept(this);
  if (stm->els) stm->els->accept(this);
  return 0;
}

int Opt2Visitor::visit(BreakStm *stm)
{
    return 0;
}

int Opt2Visitor::visit(SwitchStm *stm)
{   
    stm->e->accept(this);
    for(auto j:stm->default_body){
          j->accept(this);
      }
    for(auto i:stm->cases){
      for(auto j:i->body){
          j->accept(this);
      }
    }
    return 0;
}

int Opt2Visitor::visit(Body *body)
{
    for(auto i:body->declarations){
      i->accept(this);
    }
    for(auto i:body->StmList){
      i->accept(this);
    }
  return 0;
}

int Opt2Visitor::visit(VarDec *vd)
{
  // no hay nada que hacer 
  return 0;
}

int Opt2Visitor::visit(FcallExp *fc)
{
    return 0;
}

int Opt2Visitor::visit(ReturnStm *r)
{
    return 0;
}

int Opt2Visitor::visit(FunDec *fd)
{
    fd->cuerpo->accept(this);
    return 0;
}

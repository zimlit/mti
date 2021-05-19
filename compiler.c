/*
   Copyright 2021 Devin Rockwell
   This file is part of MTI
   MTI is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"


#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PrecNone,
  PrecDeclaration,
  PrecStatement,
  PrecLiteral,
  PrecAssignment,
  PrecOr,
  PrecAnd,
  PrecEquality,
  PrecComparison,
  PrecTerm,
  PrecFactor,
  PrecUnary,
  PrecCall,
  PrecPrimary,
} Precedence;

#define UINT8_COUNT (UINT8_MAX + 1)

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;
Compiler* current = NULL;

static void initCompiler(Compiler* compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  current = compiler;
}

static Chunk* currentChunk() {
  return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TokEOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TokError) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TokError) break;

    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}


static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

static void emitReturn() {
  emitByte(OpReturn);
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static void emitConstant(Value value) {
  emitBytes(OpConstant, makeConstant(value));
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void endCompiler() {
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}


static void expression();
static void parsePrecedence(Precedence precedence);
static ParseRule* getRule(TokenType type);

static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start,
                                         name->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
}

static void declareVariable() {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; 
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already variable with this name in this scope.");
    }
  }

  addLocal(*name);
}

static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TokPlus:          emitByte(OpAdd); break;
    case TokMinus:         emitByte(OpSubtract); break;
    case TokStar:          emitByte(OpMultiply); break;
    case TokSlash:         emitByte(OpDivide); break;
    case TokBangEq:    emitBytes(OpEq, OpNot); break;
    case TokEqEq:   emitByte(OpEq); break;
    case TokGreater:       emitByte(OpGreater); break;
    case TokGreaterEq: emitBytes(OpLess, OpNot); break;
    case TokLess:          emitByte(OpLess); break;
    case TokLessEq:    emitBytes(OpGreater, OpNot); break;
    default: return; // Unreachable.
  }
}

static void literal(bool canAssign) {
  switch (parser.previous.type) {
    case TokFalse: emitByte(OpFalse); break;
    case TokNil: emitByte(OpNil); break;
    case TokTrue: emitByte(OpTrue); break;
    default: return; // Unreachable.
  }
}

static void grouping(bool canAssign) {
  expression();
  consume(TokRightParen, "Expect ')' after expression.");
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PrecUnary);

  // Emit the operator instruction.
  switch (operatorType) {
    case TokBang: emitByte(OpNot); break;
    case TokMinus: emitByte(OpNegate); break;
    default: return; // Unreachable.
  }
}

static void print(bool canAssign) {
  expression();
  emitByte(OpPrint);
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TokIdent, errorMessage);


  declareVariable();
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}

static void markInitialized() {
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}

static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    printf("local\n");
    emitByte(OpCopyValToLocal);
    markInitialized();
    return;
  }

  emitBytes(OpDefineGlobal, global);
}

static void and_(bool canAssign) {
  int endJump = emitJump(OpJumpIfFalse);

  emitByte(OpPop);
  parsePrecedence(PrecAnd);

  patchJump(endJump);
}

static void or_(bool canAssign) {
  int elseJump = emitJump(OpJumpIfFalse);
  int endJump = emitJump(OpJump);

  patchJump(elseJump);
  emitByte(OpPop);

  parsePrecedence(PrecOr);
  patchJump(endJump);
}

static void vardecl(bool canAssign) {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TokEq)) {
    expression();
  } else {
    emitByte(OpNil);
  }

  defineVariable(global);
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OpGetLocal;
    setOp = OpSetLocal;
  } else {
    arg = identifierConstant(&name);
    getOp = OpGetGlobal;
    setOp = OpSetGlobal;
  }
  if (canAssign && match(TokEq)) {
    expression();
    emitBytes(setOp, arg);
  } else {
    emitBytes(getOp, arg);
  }}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;


  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
    emitByte(OpLocalPop);
    current->localCount--;
  }
}

static void block(bool canAssign) {
  beginScope();
  while (!check(TokEnd) && !check(TokEOF)) {
    expression();
  }

  consume(TokEnd, "Expect 'end' after block");
  endScope();
}

static void ifStmt(bool canAssign) {
  consume(TokLeftParen, "Expect '(' after if");
  expression();
  consume(TokRightParen, "Expect ')' after condition");

  int thenJump = emitJump(OpJumpIfFalse);
  emitByte(OpPop);
  bool isElse;
  while (!check(TokEnd) && !check(TokEOF)) {
    expression();
    if (match(TokElse)) {
        isElse = true;
        break;
    }
  }

  int elseJump = emitJump(OpJump);

  patchJump(thenJump);

  emitByte(OpPop);
  if (!isElse) {
    emitByte(OpNil);
  }
  while (!check(TokEnd) && !check(TokEOF)) {
    expression();
  }

  patchJump(elseJump);

  consume(TokEnd, "expect 'end' after if expression");
}

ParseRule rules[] = {
  [TokLeftParen]    = {grouping, NULL,   PrecNone},
  [TokRightParen]   = {NULL,     NULL,   PrecNone},
  [TokEnd]          = {NULL,     NULL,   PrecNone},
  [TokDo]           = {block,    NULL,   PrecStatement},
  [TokComma]        = {NULL,     NULL,   PrecNone},
  [TokDot]          = {NULL,     NULL,   PrecNone},
  [TokMinus]        = {unary,    binary, PrecTerm},
  [TokPlus]         = {NULL,     binary, PrecTerm},
  [TokSemicolon]    = {NULL,     NULL,   PrecNone},
  [TokSlash]        = {NULL,     binary, PrecFactor},
  [TokStar]         = {NULL,     binary, PrecFactor},
  [TokBang]         = {unary,    NULL,   PrecNone},
  [TokBangEq]       = {NULL,     binary, PrecEquality},
  [TokEq]           = {NULL,     NULL,   PrecNone},
  [TokEqEq]         = {NULL,     binary, PrecEquality},
  [TokGreater]      = {NULL,     binary, PrecComparison},
  [TokGreaterEq]    = {NULL,     binary, PrecComparison},
  [TokLess]         = {NULL,     binary, PrecComparison},
  [TokLessEq]       = {NULL,     binary, PrecComparison},
  [TokIdent]        = {variable, NULL,   PrecLiteral},
  [TokString]       = {string,   NULL,   PrecLiteral},
  [TokNumber]       = {number,   NULL,   PrecNone},
  [TokAnd]          = {NULL,     and_,   PrecAnd},
  [TokClass]        = {NULL,     NULL,   PrecNone},
  [TokElse]         = {NULL,     NULL,   PrecNone},
  [TokFalse]        = {literal,  NULL,   PrecNone},
  [TokFn]           = {NULL,     NULL,   PrecNone},
  [TokIf]           = {ifStmt,   NULL,   PrecStatement},
  [TokNil]          = {literal,  NULL,   PrecNone},
  [TokOr]           = {NULL,     or_,    PrecOr},
  [TokPrint]        = {print,    NULL,   PrecStatement},
  [TokReturn]       = {NULL,     NULL,   PrecNone},
  [TokSuper]        = {NULL,     NULL,   PrecNone},
  [TokSelf]         = {NULL,     NULL,   PrecNone},
  [TokTrue]         = {literal,  NULL,   PrecNone},
  [TokLet]          = {vardecl,  vardecl,PrecDeclaration},
  [TokWhile]        = {NULL,     NULL,   PrecNone},
  [TokError]        = {NULL,     NULL,   PrecNone},
  [TokEOF]          = {NULL,     NULL,   PrecNone},
};



static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PrecAssignment;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TokEq)) {
    error("Invalid assignment target");
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TokEOF) {
    switch (parser.current.type) {
      case TokLet:
      case TokIf:
      case TokWhile:
      case TokPrint:
      case TokReturn:
        return;

      default:
        ; // Do nothing.
    }

    advance();
  }
}



static void expression() {
  parsePrecedence(PrecAssignment);
  if (parser.panicMode) synchronize();
}

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();
  while (!match(TokEOF)) {
    expression();
  }
  endCompiler();
  return !parser.hadError;
}

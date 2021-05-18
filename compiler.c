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

#include "common.h"
#include "compiler.h"
#include "scanner.h"


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
  PrecAssignment,
  PrecOr,
  PrecAnd,
  PrecEquality,
  PrecComparison,
  PrecTerm,
  PrecFactor,
  PrecUnary,
  PrecCall,
  PrecPrimary
} Precedence;

typedef void (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

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

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
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

static void binary() {
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

static void literal() {
  switch (parser.previous.type) {
    case TokFalse: emitByte(OpFalse); break;
    case TokNil: emitByte(OpNil); break;
    case TokTrue: emitByte(OpTrue); break;
    default: return; // Unreachable.
  }
}

static void grouping() {
  expression();
  consume(TokRightParen, "Expect ')' after expression.");
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void unary() {
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

ParseRule rules[] = {
  [TokLeftParen]    = {grouping, NULL,   PrecNone},
  [TokRightParen]   = {NULL,     NULL,   PrecNone},
  [TokEnd]          = {NULL,     NULL,   PrecNone},
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
  [TokIdent]        = {NULL,     NULL,   PrecNone},
  [TokString]       = {NULL,     NULL,   PrecNone},
  [TokNumber]       = {number,   NULL,   PrecNone},
  [TokAnd]          = {NULL,     NULL,   PrecNone},
  [TokClass]        = {NULL,     NULL,   PrecNone},
  [TokElse]         = {NULL,     NULL,   PrecNone},
  [TokFalse]        = {literal,  NULL,   PrecNone},
  [TokFn]           = {NULL,     NULL,   PrecNone},
  [TokIf]           = {NULL,     NULL,   PrecNone},
  [TokNil]          = {literal,  NULL,   PrecNone},
  [TokOr]           = {NULL,     NULL,   PrecNone},
  [TokPrint]        = {NULL,     NULL,   PrecNone},
  [TokReturn]       = {NULL,     NULL,   PrecNone},
  [TokSuper]        = {NULL,     NULL,   PrecNone},
  [TokSelf]         = {NULL,     NULL,   PrecNone},
  [TokTrue]         = {literal,  NULL,   PrecNone},
  [TokLet]          = {NULL,     NULL,   PrecNone},
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

  prefixRule();

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

static void expression() {
  parsePrecedence(PrecAssignment);
}

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();
  expression();
  consume(TokEOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}

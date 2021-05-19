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

#ifndef mti_chunk_h
#define mti_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OpReturn,
  OpConstant,
  OpNegate,
  OpAdd,
  OpSubtract,
  OpMultiply,
  OpDivide,
  OpNil,
  OpFalse,
  OpTrue,
  OpNot,
  OpEq,
  OpGreater,
  OpLess,
  OpPrint,
  OpDefineGlobal,
  OpGetGlobal,
  OpSetGlobal,
  OpGetLocal,
  OpSetLocal,
  OpPop,
  OpLocalPop,
  OpCopyValToLocal,
  OpJumpIfFalse,
  OpJump,
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif

#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk,
                               int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 &&
      chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OpReturn:
      return simpleInstruction("OpRETURN", offset);
    case OpConstant:
      return constantInstruction("OpCONSTANT", chunk, offset);
    case OpNegate:
      return simpleInstruction("OpNEGATE", offset);
    case OpAdd:
      return simpleInstruction("OpADD", offset);
    case OpSubtract:
      return simpleInstruction("OpSUBTRACT", offset);
    case OpMultiply:
      return simpleInstruction("OpMULTIPLY", offset);
    case OpDivide:
      return simpleInstruction("OpDIVIDE", offset);
    case OpNil:
      return simpleInstruction("OpNil", offset);
    case OpTrue:
      return simpleInstruction("OpTrue", offset);
    case OpFalse:
      return simpleInstruction("OpFalse", offset);
    case OpNot:
      return simpleInstruction("OpNOT", offset);
    case OpEq:
      return simpleInstruction("OpEQUAL", offset);
    case OpGreater:
      return simpleInstruction("OpGREATER", offset);
    case OpLess:
      return simpleInstruction("OpLESS", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

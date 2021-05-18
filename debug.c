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
      return simpleInstruction("OP_RETURN", offset);
    case OpConstant:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OpNegate:
      return simpleInstruction("OP_NEGATE", offset);
    case OpAdd:
      return simpleInstruction("OP_ADD", offset);
    case OpSubtract:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OpMultiply:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OpDivide:
      return simpleInstruction("OP_DIVIDE", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

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

#ifndef mti_value_h
#define mti_value_h

#include "common.h"

typedef enum {
  ValBool,
  ValNil,
  ValNum,
} ValueType;


typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as; 
} Value;


#define IS_BOOL(value)    ((value).type == ValBool)
#define IS_NIL(value)     ((value).type == ValNil)
#define IS_NUMBER(value)  ((value).type == ValNum)

#define BOOL_VAL(value)   ((Value){ValBool, {.boolean = value}})
#define NIL_VAL           ((Value){ValNil, {.number = 0}})
#define NUMBER_VAL(value) ((Value){ValNum, {.number = value}})

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif



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

#ifndef mti_scanner_h
#define mti_scanner_h

typedef enum {
  TokLeftParen, TokRightParen,
  TokComma, TokDot, TokMinus, TokPlus,
  TokStar, TokSlash, TokSemicolon,
  
  TokBang, TokBangEq,
  TokEq, TokEqEq,
  TokGreater, TokGreaterEq,
  TokLess, TokLessEq,

  TokIdent, TokString, TokNumber,

  TokAnd, TokClass, TokElse, TokFalse,
  TokWhile, TokFn, TokIf, TokNil, TokOr,
  TokPrint, TokReturn, TokSuper, TokSelf,
  TokTrue, TokLet, TokEnd, TokDo,

  TokError, TokEOF
} TokenType;


typedef struct {
  TokenType type;
  const char* start;
  int length;
  int line;
} Token;

void initScanner(const char* source);
Token scanToken();

#endif



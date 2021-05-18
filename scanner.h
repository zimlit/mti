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
  TokTrue, TokLet, TokEnd,

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



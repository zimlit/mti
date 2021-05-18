#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isAtEnd() {
  return *scanner.current == '\0';
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static char peek() {
  return *scanner.current;
}

static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++;
  return true;
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token errorToken(const char* message) {
  Token token;
  token.type = TokError;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        scanner.line++;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          // A comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static Token string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') scanner.line++;
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TokString);
}

static Token number() {
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume the ".".
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TokNumber);
}

static TokenType checkKeyword(int start, int length,
    const char* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TokIdent;
}

static TokenType identifierType() {
  switch (scanner.start[0]) {
    case 'a': return checkKeyword(1, 2, "nd", TokAnd);
    case 'c': return checkKeyword(1, 4, "lass", TokClass);
    case 'w': return checkKeyword(1, 4, "hile", TokWhile);
    case 'i': return checkKeyword(1, 1, "f", TokIf);
    case 'n': return checkKeyword(1, 2, "il", TokNil);
    case 'o': return checkKeyword(1, 1, "r", TokOr);
    case 'p': return checkKeyword(1, 4, "rint", TokPrint);
    case 'r': return checkKeyword(1, 5, "eturn", TokReturn);
    case 't': return checkKeyword(1, 3, "rue", TokTrue);
    case 'l': return checkKeyword(1, 2, "et", TokLet);
    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'a': return checkKeyword(2, 3, "lse", TokFalse);
          case 'n': return checkKeyword(2, 0, "", TokFn);
        }
      }
      break;
    case 's':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'u': return checkKeyword(2, 3, "per", TokSuper);
          case 'e': return checkKeyword(2, 2, "lf", TokSelf);
        }
      }
      break;
    case 'e':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'l': return checkKeyword(2, 2, "se", TokElse);
          case 'n': return checkKeyword(2, 1, "d", TokEnd);
        }
      }
      break;

  }

  return TokIdent;
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}

Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;

  if (isAtEnd()) return makeToken(TokEOF);

  char c = advance();
  if (isDigit(c)) return number();
  if (isAlpha(c)) return identifier();

  switch (c) {
    case '(': return makeToken(TokLeftParen);
    case ')': return makeToken(TokRightParen);
    case ';': return makeToken(TokSemicolon);
    case ',': return makeToken(TokComma);
    case '.': return makeToken(TokDot);
    case '-': return makeToken(TokMinus);
    case '+': return makeToken(TokPlus);
    case '/': return makeToken(TokStar);
    case '*': return makeToken(TokStar);
    case '!':
      return makeToken(
          match('=') ? TokBangEq : TokBang);
    case '=':
      return makeToken(
          match('=') ? TokEqEq : TokEq);
    case '<':
      return makeToken(
          match('=') ? TokLessEq : TokLess);
    case '>':
      return makeToken(
          match('=') ? TokGreaterEq : TokGreater);
    case '"': return string();
  }

  return errorToken("Unexpected character.");
}



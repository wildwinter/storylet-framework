/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

 #ifndef WRITER_H
 #define WRITER_H
 
 namespace ExpressionParser {
 
 enum class STRING_FORMAT {
     SINGLEQUOTE = 0,
     ESCAPED_SINGLEQUOTE = 1,
     DOUBLEQUOTE = 2,
     ESCAPED_DOUBLEQUOTE = 3
 };
 
 class Writer {
 public:
     Writer() = delete; // Prevent instantiation
 
     static STRING_FORMAT getStringFormat();
     static void setStringFormat(STRING_FORMAT format);
 
 private:
     static STRING_FORMAT _stringFormat;
 };
 
 } // namespace ExpressionParser
 
 #endif // WRITER_H
/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

#include "expression_parser/writer.h"

namespace ExpressionParser {

STRING_FORMAT Writer::_stringFormat = STRING_FORMAT::SINGLEQUOTE;

STRING_FORMAT Writer::getStringFormat() {
    return _stringFormat;
}

void Writer::setStringFormat(STRING_FORMAT format) {
    _stringFormat = format;
}

} // namespace ExpressionParser
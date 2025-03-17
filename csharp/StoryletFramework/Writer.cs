/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

namespace ExpressionParser
{
    public static class Writer
    {
        public enum STRING_FORMAT {
            SINGLEQUOTE = 0,
            ESCAPED_SINGLEQUOTE = 1,
            DOUBLEQUOTE = 2,
            ESCAPED_DOUBLEQUOTE = 3
        }

        private static STRING_FORMAT _stringFormat = STRING_FORMAT.SINGLEQUOTE;
        public static STRING_FORMAT StringFormat
        {
            get { return _stringFormat; }
            set { _stringFormat = value; }
        }
    }
}
# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

STRING_FORMAT_SINGLEQUOTE = 0
STRING_FORMAT_ESCAPED_SINGLEQUOTE = 1
STRING_FORMAT_DOUBLEQUOTE = 2
STRING_FORMAT_ESCAPED_DOUBLEQUOTE = 3

class _Writer:
    def __init__(self):
        self._string_format = STRING_FORMAT_SINGLEQUOTE

    @property
    def string_format(self) -> int:
        return self._string_format

    @string_format.setter
    def string_format(self, value: int) -> None:
        self._string_format = value

# Import this as a singleton
Writer = _Writer()
# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import unittest
import sys
import os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../src")))

from expression_parser.parser import Parser
from expression_parser.writer import Writer, STRING_FORMAT_SINGLEQUOTE, STRING_FORMAT_ESCAPED_SINGLEQUOTE, STRING_FORMAT_DOUBLEQUOTE, STRING_FORMAT_ESCAPED_DOUBLEQUOTE

class TestWriter(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None  # Allow full diff output for every test case

    def _load_file(self, file_name):
        try:
            with open(f"../tests/{file_name}", "r", encoding="utf-8") as file:
                return file.read()
        except Exception as e:
            self.fail(f"Error loading {file_name}: {e}")

    def test_simple(self):
        
        parser = Parser()
        expression = parser.parse("get_name()=='fred' and counter>0 and 5/5.0!=0")

        result = expression.write()
        self.assertEqual(result, "get_name() == 'fred' and counter > 0 and 5 / 5 != 0", "Expression doesn't match.")
        Writer.string_format = STRING_FORMAT_DOUBLEQUOTE
        result = expression.write()
        self.assertEqual(result, "get_name() == \"fred\" and counter > 0 and 5 / 5 != 0", "Expression doesn't match.")
        Writer.string_format = STRING_FORMAT_ESCAPED_DOUBLEQUOTE
        result = expression.write()
        self.assertEqual(result, "get_name() == \\\"fred\\\" and counter > 0 and 5 / 5 != 0", "Expression doesn't match.")
        Writer.string_format = STRING_FORMAT_ESCAPED_SINGLEQUOTE
        result = expression.write()
        self.assertEqual(result, "get_name() == \\'fred\\' and counter > 0 and 5 / 5 != 0", "Expression doesn't match.")
        Writer.string_format = STRING_FORMAT_SINGLEQUOTE

    def test_writer(self):

        source = self._load_file("Writer.txt")

        # Split into lines
        lines = source.splitlines()

        parser = Parser()

        processed_lines = []
        for line in lines:
            if (line.startswith("//")):
                processed_lines.append(line)
                continue

            processed_lines.append(f'"{line}"')

            node = parser.parse(line)
            
            processed_lines.append(node.write())

            processed_lines.append("")

        output = "\n".join(processed_lines)

        #print(output)

        match = self._load_file("Writer-Output.txt")
        self.assertMultiLineEqual(match, output)
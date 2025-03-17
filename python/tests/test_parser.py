# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import unittest
import sys
import os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../src")))

from expression_parser.parser import Parser

class TestParser(unittest.TestCase):

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

        context = {
            "get_name":lambda: "fred",
            "counter": 1
        }

        result = expression.evaluate(context)

        self.assertEqual(result, True, "Expression should return True")


    def test_parse(self):

        source = self._load_file("Parse.txt")

        # Split into lines
        lines = source.splitlines()

        context = {
            "C":15,
            "D":False,
            "get_name":lambda: "fred",
            "end_func":lambda: True,
            "whisky": lambda id, n: str(int(n))+"whisky_"+id,
            "counter":1
        }

        parser = Parser()

        processed_lines = []
        for line in lines:
            if (line.startswith("//")):
                processed_lines.append(line)
                continue

            processed_lines.append(f'"{line}"')
            try:
                node = parser.parse(line)
                processed_lines.append(node.dump_structure())
                dump_eval = []
                node.evaluate(context, dump_eval)
                processed_lines.append("\n".join(dump_eval))
                
            except Exception as e:
                processed_lines.append(str(e))

            processed_lines.append("")

        output = "\n".join(processed_lines)

        #print(output)

        match = self._load_file("Parse-Output.txt")
        self.assertMultiLineEqual(match, output)

if __name__ == "__main__":
    unittest.main()
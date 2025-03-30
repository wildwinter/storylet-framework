# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import unittest
import sys
import os
from test_utils import load_json_file

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../src")))

from storylet_framework.storylets import Deck

class TestStorylets(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None  # Allow full diff output for every test case

    def test_basic_calls(self):

        dump_eval = []
    
        json_data = load_json_file("Streets.jsonc")
        deck = Deck.from_json(json_data, {}, True, dump_eval)
    
        card = deck.draw()
        self.assertIsNotNone(card)
    
        card = deck.draw()
        self.assertIsNotNone(card)
    
        print("\n".join(dump_eval))
    
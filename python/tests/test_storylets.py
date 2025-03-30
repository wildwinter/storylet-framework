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


    def test_barks(self):
        context = {
            "street_id": "",
            "street_wealth": 1,
            "encounter_tag": lambda tag: False
        }
    
        barks = Deck.from_json(load_json_file("Barks.jsonc"), context)
        #print(barks.dump_draw_pile())
        self.assertIsNotNone(barks.draw())

    def test_street_system(self):
        context = {
            "street_id": "",
            "street_wealth": 0,
            "street_tag": lambda tag: False,
            "encounter_tag": lambda tag: False
        }
    
        streets = Deck.from_json(load_json_file("Streets.jsonc"), context)
        encounters = Deck.from_json(load_json_file("Encounters.jsonc"), context)
        barks = Deck.from_json(load_json_file("Barks.jsonc"), context)
    
        def set_street(street):
            context["street_id"] = street.id
            context["street_wealth"] = street.content["wealth"]
            context["street_tag"] = lambda tag: tag in street.content["tags"]
            print(f'Location: "{street.content["title"]}"')
    
        def do_encounter(street):
            set_street(street)
            # We're on a new street, so shuffle the encounters deck to only include relevant cards.
            encounters.reshuffle()
            # print(encounters.dump_draw_pile())
            encounter = encounters.draw()
            context["encounter_tag"] = lambda tag: tag in encounter.content.get("tags", [])
            print(f'  Encounter: "{encounter.content["title"]}"')
            barks.reshuffle()
            # print(barks.dump_draw_pile())
            bark = barks.draw()
            if bark:
                print(f'  Comment: "{bark.content["comment"]}"')
    
        # First encounter - this should pull out a "start" location.
        streets.reshuffle(lambda street: "start" in street.content["tags"])
        street = streets.draw()
        do_encounter(street)
    
        self.assertIn(street.id, ["docks", "market", "bridge"])
    
        # Reshuffle the deck so that all streets are fair game.
        streets.reshuffle()
    
        path = []
    
        # Walk through the street deck and pull an encounter for each location
        for _ in range(11):
            street = streets.draw()
            path.append(street.id)
            do_encounter(street)
    
        self.assertTrue(any(street_id in ["market", "slums", "bridge"] for street_id in path))

    def test_aysnc_reshuffles(self):
        dump_eval = []
    
        context = {
            "street_id": "",
            "street_wealth": 1,
            "encounter_tag": lambda tag: False
        }
    
        barks = Deck.from_json(load_json_file("Barks.jsonc"), context, reshuffle=False)
        barks.reshuffle_async(lambda: print("Async reshuffle complete."), None, dump_eval)
    
        while barks.async_reshuffle_in_progress():
            barks.update()
    
        print(barks.dump_draw_pile())
    
        card = barks.draw()
        self.assertIsNotNone(card)
    
        card = barks.draw()
        self.assertIsNotNone(card)
    
        print("\n".join(dump_eval))
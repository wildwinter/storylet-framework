# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import unittest
import sys
import os
from test_utils import load_json_file

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../src")))

from storylet_framework.json_loader import deck_from_json

class TestStorylets(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None  # Allow full diff output for every test case

    def test_basic_calls(self):

        dump_eval = []
    
        json_data = load_json_file("Streets.jsonc")
        deck = deck_from_json(json_data, {}, dump_eval)
        drawn = deck.draw_and_play(2)
        self.assertIsNotNone(drawn.pop(0))
        self.assertIsNotNone(drawn.pop(0))

        print("\n".join(dump_eval))


    def test_barks(self):
        context = {
            "street_id": "",
            "street_wealth": 1,
            "encounter_tag": lambda tag: False
        }
    
        barks = deck_from_json(load_json_file("Barks.jsonc"), context)
        #print(barks.dump_draw_pile())
        self.assertIsNotNone(barks.draw_and_play_single())

    def test_street_system(self):
        context = {
            "street_id": "",
            "street_wealth": 0,
            "street_tag": lambda tag: False,
            "encounter_tag": lambda tag: False
        }
    
        streets = deck_from_json(load_json_file("Streets.jsonc"), context)
        encounters = deck_from_json(load_json_file("Encounters.jsonc"), context)
        barks = deck_from_json(load_json_file("Barks.jsonc"), context)
    
        dump_eval = []
        
        def set_street(street):
            context["street_id"] = street.id
            context["street_wealth"] = street.content["wealth"]
            context["street_tag"] = lambda tag: tag in street.content["tags"]
            print(f'Location: "{street.content["title"]}"')
    
        def do_encounter(street):
            set_street(street)
            encounter = encounters.draw_and_play_single(dump_eval=dump_eval)
            context["encounter_tag"] = lambda tag: tag in encounter.content.get("tags", [])
            print(f'  Encounter: "{encounter.content["title"]}"')
            bark = barks.draw_and_play_single(dump_eval=dump_eval)
            if bark:
                print(f'  Comment: "{bark.content["comment"]}"')
    
        # First encounter - this should pull out a "start" location.
        street = streets.draw_and_play_single(lambda street: "start" in street.content["tags"], dump_eval=dump_eval)
        do_encounter(street)
    
        self.assertIn(street.id, ["docks", "market", "bridge"])
    
        # Reshuffle the deck so that all streets are fair game.
        streets_drawn = streets.draw()
    
        path = []
    
        # Walk through the street deck and pull an encounter for each location
        for street in streets_drawn:
            street.play()
            path.append(street.id)
            do_encounter(street)
    
        self.assertTrue(any(street_id in ["market", "slums", "bridge"] for street_id in path))
        self.assertTrue(context["noble_storyline"]>0)
        #print("\n".join(dump_eval))

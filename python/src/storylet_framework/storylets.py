# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import sys
import os
from typing import Any, Dict, List, Optional
from .utils import update_object, shuffle_array, copy_object
from .context import init_context, update_context

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../../lib/expression-parser")))

from expression_parser.parser import Parser

expression_parser = Parser()

REDRAW_ALWAYS = 0
REDRAW_NEVER = -1

class Storylet:
    def __init__(self, id):
        # Unique ID of the storylet
        self.id = id

        # Application-defined content
        self.content = {}

        # Redraw setting:
        #   >0 = how many draws before this is available again,
        #   REDRAW_ALWAYS = always available,
        #   REDRAW_NEVER = a one-shot
        self.redraw = REDRAW_ALWAYS

        # Updates to context, processed when this storylet is drawn
        self.update_on_drawn = None

        # Precompiled
        self._condition = None
        # Might be absolute value, might be an expression
        self._priority = 0
        # The next draw this should be available
        self._next_draw = 0

    # Reset the redraw counter
    def reset(self):
        self._next_draw = 0

    # Set text of expression. It'll be precompiled
    @property
    def condition(self):
        return self._condition

    @condition.setter
    def condition(self, text):
        self._condition = None
        if text:
            self._condition = expression_parser.parse(text)

    # Evaluate condition using current context. Return boolean. If no condition, returns True
    def check_condition(self, context, dump_eval=None):
        if not self._condition:
            return True
        if dump_eval is not None:
            dump_eval.append(f"Evaluating condition for {self.id}")
        return self._condition.evaluate(context, dump_eval)

    # Set priority to a fixed number, or to an expression which will be precompiled
    @property
    def priority(self):
        return self._priority

    @priority.setter
    def priority(self, num_or_expression):
        if isinstance(num_or_expression, (int, float)):
            self._priority = num_or_expression
        else:
            self._priority = expression_parser.parse(num_or_expression)

    # Evaluate priority using current context
    # If use_specificity is True, will produce a higher number if the storylet's expression is more complex
    def calc_current_priority(self, context, use_specificity=True, dump_eval=None):
        working_priority = 0
        if isinstance(self._priority, (int, float)):
            working_priority = self._priority
        else:
            if dump_eval is not None:
                dump_eval.append(f"Evaluating priority for {self.id}")
            working_priority = self._priority.evaluate(context, dump_eval)

        if use_specificity:
            working_priority *= 100
            if self._condition is not None:
                working_priority += self._condition.specificity

        return working_priority

    # True if the card is available to draw according to its redraw rules
    def can_draw(self, current_draw):
        if self.redraw == REDRAW_NEVER and self._next_draw < 0:
            return False
        if self.redraw == REDRAW_ALWAYS:
            return True
        return current_draw >= self._next_draw

    # Call when actually drawn - updates the redraw counter
    def drawn(self, current_draw):
        if self.redraw == REDRAW_NEVER:
            self._next_draw = -1
        else:
            self._next_draw = current_draw + self.redraw

    # Basic parsing
    @staticmethod
    def from_json(json, defaults):
        if "id" not in json:
            raise ValueError("No 'id' property in the storylet JSON.")

        config = defaults.copy()
        update_object(config, json)

        storylet = Storylet(config["id"])
        if "redraw" in config:
            val = config["redraw"]
            if val == "always":
                storylet.redraw = REDRAW_ALWAYS
            elif val == "never":
                storylet.redraw = REDRAW_NEVER
            else:
                storylet.redraw = int(val)

        if "condition" in config:
            storylet.condition = config["condition"]
        if "priority" in config:
            storylet.priority = config["priority"]
        if "updateOnDrawn" in config:
            storylet.update_on_drawn = config["updateOnDrawn"]
        if "content" in config:
            storylet.content = config["content"]

        return storylet
    

class Deck:
    def __init__(self, context=None):
        if context is None:
            context = {}

        # If true, storylet.priority is still used as the base, but
        # within priorities, more complex conditions (thus more specific) are
        # treated as higher priority
        self.use_specificity = False

        # How many storylets to process on each call to update(), if using async reshuffles
        self.async_reshuffle_count = 10

        # Complete set of storylets
        self._all = {}

        # Currently calculated pile
        self._draw_pile = []

        # Keeps a count of the number of draws. Used to keep redraw rules correct.
        # Call reset() to start from scratch.
        self._current_draw = 0

        # Context to be used for all expression evaluations.
        self._context = context

        # Used to deal with in-progress reshuffles.
        self._reshuffle_state = {
            "callback": None,
            "to_process": [],
            "filter": None,
            "priority_map": {},
            "dump_eval": None,
        }

    @staticmethod
    def from_json(json, context=None, reshuffle=True, dump_eval=None):
        if context is None:
            context = {}

        deck = Deck(context)
        deck.load_json(json, dump_eval)
        if reshuffle:
            deck.reshuffle(None, dump_eval)
        return deck

    def load_json(self, json, dump_eval=None):
        self._read_packet_from_json(json, {}, dump_eval)

    def _read_packet_from_json(self, json, defaults, dump_eval=None):
        if "context" in json:
            init_context(self._context, json["context"], dump_eval)

        if "defaults" in json:
            for var_name, value in json["defaults"].items():
                defaults[var_name] = value

        if "storylets" in json:
            self._read_storylets_from_json(json["storylets"], copy_object(defaults), dump_eval)

    def _read_storylets_from_json(self, json, defaults, dump_eval=None):
        for item in json:
            # Is this a storylet? Or is it a packet?
            if "storylets" in item or "defaults" in item or "context" in item:
                self._read_packet_from_json(item, defaults, dump_eval)
                continue

            # Read as storylet
            if "id" not in item:
                raise ValueError("Json item is not a storylet or packet")

            storylet = Storylet.from_json(item, defaults)
            if storylet.id in self._all:
                raise ValueError(f"Duplicate storylet id: '{storylet.id}'")

            self._all[storylet.id] = storylet
            if dump_eval is not None:
                dump_eval.append(f"Added storylet '{storylet.id}'")

    def reset(self):
        self._current_draw = 0
        for storylet in self._all.values():
            storylet.reset()

    def reshuffle(self, filter=None, dump_eval=None):
        if self.async_reshuffle_in_progress():
            raise RuntimeError("Async reshuffle in progress, can't call reshuffle()")

        self._reshuffle_prep(filter, dump_eval)
        # Reshuffle everything at once
        self._reshuffle_do_chunk(len(self._reshuffle_state["to_process"]))
        self._reshuffle_finalize()

    def reshuffle_async(self, callback, filter=None, dump_eval=None):
        if self.async_reshuffle_in_progress():
            raise RuntimeError("Async reshuffle in progress, can't call reshuffle_async()")

        self._reshuffle_state["callback"] = callback
        self._reshuffle_prep(filter, dump_eval)

    def async_reshuffle_in_progress(self):
        return self._reshuffle_state["callback"] is not None

    def update(self):
        # If an async reshuffle is in progress
        if self.async_reshuffle_in_progress():
            self._reshuffle_do_chunk(self.async_reshuffle_count)
            if not self._reshuffle_state["to_process"]:
                self._reshuffle_finalize()

    def _reshuffle_prep(self, filter, dump_eval=None):
        # Empty the draw pile
        self._draw_pile = []
        self._reshuffle_state["dump_eval"] = dump_eval
        self._reshuffle_state["filter"] = filter
        # Temp map to hold lists by priority
        self._reshuffle_state["priority_map"] = {}
        # All the cards to walk
        self._reshuffle_state["to_process"] = list(self._all.values())

    def _reshuffle_do_chunk(self, count):
        number_to_do = min(count, len(self._reshuffle_state["to_process"]))

        while number_to_do > 0:
            number_to_do -= 1

            storylet = self._reshuffle_state["to_process"].pop(0)

            if not storylet.can_draw(self._current_draw):
                # Storylet fails the draw rules - skip
                continue

            # Apply filter, if available
            if self._reshuffle_state["filter"] is not None:
                if not self._reshuffle_state["filter"](storylet):
                    # Storylet fails the filter - skip
                    continue

            if not storylet.check_condition(self._context, self._reshuffle_state["dump_eval"]):
                # Storylet fails the condition - skip
                continue

            # Get the current priority for the storylet
            priority = storylet.calc_current_priority(
                self._context, self.use_specificity, self._reshuffle_state["dump_eval"]
            )

            if priority not in self._reshuffle_state["priority_map"]:
                # Does a priority list for this priority value exist in the temp map? If not, make it
                self._reshuffle_state["priority_map"][priority] = []

            # Add our card to this priority list
            self._reshuffle_state["priority_map"][priority].append(storylet)

    def _reshuffle_finalize(self):
        # Now sort all the resultant priorities
        sorted_priorities = sorted(self._reshuffle_state["priority_map"].keys(), reverse=True)

        # Shuffle each set of storylets that are the same priority. Then add them all to the master draw pile.
        # Result will be - higher priorities will be in the pile at the front, and we go down from there.
        for priority in sorted_priorities:
            bucket = self._reshuffle_state["priority_map"][priority]
            shuffled_bucket = shuffle_array(bucket)
            self._draw_pile.extend(shuffled_bucket)

        self._reshuffle_state["priority_map"] = None
        self._reshuffle_state["to_process"] = []
        self._reshuffle_state["filter"] = None
        callback = self._reshuffle_state["callback"]
        self._reshuffle_state["callback"] = None
        self._reshuffle_state["dump_eval"] = None
        if callback:
            callback()

    def dump_draw_pile(self):
        if self.async_reshuffle_in_progress():
            raise RuntimeError("Async reshuffle in progress, can't call dump_draw_pile()")

        return ",".join(storylet.id for storylet in self._draw_pile)

    def draw(self):
        if self.async_reshuffle_in_progress():
            raise RuntimeError("Async reshuffle in progress, can't call draw()")

        self._current_draw += 1

        if not self._draw_pile:
            return None

        storylet = self._draw_pile.pop(0)
        if storylet.update_on_drawn:
            update_context(self._context, storylet.update_on_drawn)
        storylet.drawn(self._current_draw)
        return storylet
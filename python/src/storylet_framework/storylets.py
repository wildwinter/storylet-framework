# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import sys
import os
from .utils import shuffle_array
from .context import update_context

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
        self.update_on_played = None

        # Precompiled
        self._condition = None
        # Might be absolute value, might be an expression
        self._priority = 0
        # The next draw this should be available
        self._next_play = 0

        self.deck = None

    # Reset the redraw counter
    def reset(self):
        self._next_play = 0

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
        if self.redraw == REDRAW_NEVER and self._next_play < 0:
            return False
        if self.redraw == REDRAW_ALWAYS:
            return True
        return current_draw >= self._next_play

    # Call when actually drawn - updates the redraw counter
    def on_played(self, current_play):
        if self.redraw == REDRAW_NEVER:
            self._next_play = -1
        else:
            self._next_play = current_play + self.redraw
        if self.update_on_played:
            update_context(self.deck.context, self.update_on_played)
    
    def play(self):
        if self.deck is None:
            raise RuntimeError("Storylet not in deck.")
        self.deck.play(self)


class Deck:
    def __init__(self, context={}):

        # If true, storylet.priority is still used as the base, but
        # within priorities, more complex conditions (thus more specific) are
        # treated as higher priority
        self.use_specificity = False

        # Complete set of storylets
        self._all = {}

        # Keeps a count of the number of draws. Used to keep redraw rules correct.
        # Call reset() to start from scratch.
        self._current_play = 0

        # Context to be used for all expression evaluations.
        self.context = context

    def reset(self):
        self._current_play = 0
        for storylet in self._all.values():
            storylet.reset()

    def draw(self, count=-1, filter=None, dump_eval=None):
        # Temp map to hold lists by priority
        priority_map = {}
        # All the cards to walk
        to_process = list(self._all.values())

        while len(to_process) > 0:
            storylet = to_process.pop(0)

            if not storylet.can_draw(self._current_play):
                # Storylet fails the draw rules - skip
                continue

            # Apply filter, if available
            if filter is not None:
                if not filter(storylet):
                    # Storylet fails the filter - skip
                    continue

            if not storylet.check_condition(self.context, dump_eval):
                # Storylet fails the condition - skip
                continue

            # Get the current priority for the storylet
            priority = storylet.calc_current_priority(self.context, self.use_specificity, dump_eval)

            if priority not in priority_map:
                # Does a priority list for this priority value exist in the temp map? If not, make it
                priority_map[priority] = []

            # Add our card to this priority list
            priority_map[priority].append(storylet)

        # Now sort all the resultant priorities
        sorted_priorities = sorted(priority_map.keys(), reverse=True)
        draw_pile = []

        # Shuffle each set of storylets that are the same priority. Then add them all to the master draw pile.
        # Result will be - higher priorities will be in the pile at the front, and we go down from there.
        for priority in sorted_priorities:
            bucket = priority_map[priority]
            shuffle_array(bucket)
            draw_pile.extend(bucket)
            if count > -1 and len(draw_pile) >= count:
                break

        return draw_pile[:count] if count > -1 else draw_pile

    def draw_and_play(self, count=-1, filter=None, dump_eval=None):
        drawn = self.draw(count, filter, dump_eval)
        for i in range(len(drawn)):
            drawn[i].play()
        return drawn
    
    def draw_single(self, filter=None, dump_eval=None):
        drawn = self.draw(1, filter, dump_eval)
        if len(drawn) == 0:
            return None
        return drawn[0]
    
    def draw_and_play_single(self, filter=None, dump_eval=None):
        drawn = self.draw_single(filter, dump_eval)
        if drawn is None:
            return None
        drawn.play()
        return drawn

    def get_storylet(self, id):
        return self._all.get(id)
    
    def add_storylet(self, storylet):
        if storylet.id in self._all:
            raise ValueError(f"Storylet with id '{storylet.id}' already exists in the deck.")
        self._all[storylet.id] = storylet
        storylet.deck = self

  # Mark this storylet as played. If it's got an updateOnPlayed, make that happen.
    def play(self, storylet):
        self._current_play += 1
        storylet.on_played(self._current_play)
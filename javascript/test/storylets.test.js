// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadTestFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {Deck} from "../src/storylets.js";

describe('Storylets', () => {

  describe('Simple', () => {
    it('should match', () => {

      const source = loadTestFile("Streets.json");
      const json = JSON.parse(source);

      const deck = new Deck();
      deck.loadJson(json);

      const context = {};
      deck.refresh(context);

      const cards = deck.draw(5);

      for (const storylet of cards) {
        console.log(storylet.content.title);
      }

      //assert.equal(result, true);
    });
  });
});
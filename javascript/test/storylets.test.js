// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadTestFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {Deck} from "../src/storylets.js";

describe('Storylets', () => {

  describe('SimpleStreets', () => {
    it('should match', () => {

      const source = loadTestFile("Streets.json");
      const json = JSON.parse(source);
      const deck = new Deck();
      deck.loadJson(json);

      const context = {};
      deck.refresh(context);
      let card = deck.draw();

      assert.equal(true, card.id=="docks");

      card = deck.draw();
      assert.equal(true, card.id!="docks");

    });

    it('should match', () => {

      const streets = new Deck();
      streets.loadJson(JSON.parse(loadTestFile("Streets.json")));

      const encounters = new Deck();
      encounters.loadJson(JSON.parse(loadTestFile("Encounters.json")));

      const context = {
        street_wealth:0,
        street_tag:null
      };

      streets.refresh(context);
      let street = streets.draw();
      assert.equal(true, street.id=="docks");
      
      console.log(street.content.title);

      context.street_wealth = street.content.wealth;
      context.street_tag = (tag) => street.context.tags.includes(tag);

      encounters.refresh(context);
      let encounter = encounters.draw();
      console.log(encounter.content.title);

    });
  });  
});
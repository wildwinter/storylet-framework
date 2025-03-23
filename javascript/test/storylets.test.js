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
      const deck = Deck.fromJson(json);

      const context = {};
      deck.refresh(context);
      let card = deck.draw();

      assert.equal(true, card.id=="docks");

      card = deck.draw();
      assert.equal(true, card.id!="docks");

    });

    it('should match', () => {

      const streets = Deck.fromJson(JSON.parse(loadTestFile("Streets.json")));
      const encounters = Deck.fromJson(JSON.parse(loadTestFile("Encounters.json")));

      const context = {
        street_wealth:0,
        street_tag:null
      };

      let setStreet = (street) => {
        context.street_wealth = street.content.wealth;
        context.street_tag = (tag) => {return street.content.tags.includes(tag)};
        console.log(`Location:"${street.content.title}"`);
      };

      streets.refresh(context);

      let street;
      for (let i=0;i<5;i++) {
          street = streets.draw();
          setStreet(street);
          encounters.refresh(context);
          let encounter = encounters.draw();
          console.log("  ", encounter.content.title);
      }

    });
  });  
});
// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadJsonFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {Deck} from "../src/storylets.js";

describe('Storylets', () => {

  describe('SimpleStreets', () => {
    it('should match', () => {

      const json = loadJsonFile("Streets.jsonc");
      const deck = Deck.fromJson(json);

      deck.refresh();
      let card = deck.draw();

      assert.equal(true, card.id=="docks");

      card = deck.draw();
      assert.equal(true, card.id!="docks");

    });

    it('should match', () => {

      const context = {
        street_id:"",
        street_wealth:0,
        street_tag:null
      };

      const streets = Deck.fromJson(loadJsonFile("Streets.jsonc"), context);
      const encounters = Deck.fromJson(loadJsonFile("Encounters.jsonc"), context);

      let setStreet = (street) => {
        context.street_id = street.id;
        context.street_wealth = street.content.wealth;
        context.street_tag = (tag) => {return street.content.tags.includes(tag)};
        console.log(`Location:"${street.content.title}"`);
      };

      let doEncounter = (encounter) => {
        console.log("  ", encounter.content.title);
      }

      streets.refresh();

      let street;
      for (let i=0;i<5;i++) {
          street = streets.draw();
          setStreet(street);
          encounters.refresh();
          let encounter = encounters.draw();
          doEncounter(encounter);
      }

    });
  });  
});
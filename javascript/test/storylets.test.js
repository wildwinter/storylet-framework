// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadJsonFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {Deck, evalExpression} from "../src/storylets.js";

describe('Storylets', () => {

  describe('SimpleStreets', () => {
    it('should match', () => {

      const json = loadJsonFile("Streets.jsonc");
      const deck = Deck.fromJson(json);

      const context = {};
      deck.refresh(context);
      let card = deck.draw();

      assert.equal(true, card.id=="docks");

      card = deck.draw();
      assert.equal(true, card.id!="docks");

    });

    it('should match', () => {

      const streets = Deck.fromJson(loadJsonFile("Streets.jsonc"));
      const encounters = Deck.fromJson(loadJsonFile("Encounters.jsonc"));

      const context = {
        met_noble:false,
        street_id:"",
        street_wealth:0,
        street_tag:null
      };

      let setStreet = (street) => {
        context.street_id = street.id;
        context.street_wealth = street.content.wealth;
        context.street_tag = (tag) => {return street.content.tags.includes(tag)};
        console.log(`Location:"${street.content.title}"`);
      };

      let doEncounter = (encounter) => {
        console.log("  ", encounter.content.title);
        if (encounter.content.contextUpdates) {
          for (var update of encounter.content.contextUpdates) {
            for (const [contextVar, value] of Object.entries(update)) {
              const result = evalExpression(value, context);
              console.log(`Setting ${contextVar} to ${result}`);
              context[contextVar] = result;
            }
          }
        }
      }

      streets.refresh(context);//, (street)=>street.content.wealth>=0);

      let street;
      for (let i=0;i<5;i++) {
          street = streets.draw();
          setStreet(street);
          encounters.refresh(context);
          let encounter = encounters.draw();
          doEncounter(encounter);
      }

    });
  });  
});
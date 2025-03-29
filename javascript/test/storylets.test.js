// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadJsonFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {Deck} from "../src/storylets.js";

describe('Storylets', () => {

  describe('SimpleStreets', () => {
    it('basic calls', () => {

      const json = loadJsonFile("Streets.jsonc");
      const deck = Deck.fromJson(json);

      deck.refresh();
      let card = deck.draw();

      assert.notEqual(null, card);

      card = deck.draw();
      assert.notEqual(null, card);

    });

    it('testing barks', () => {

      const context = {
        street_id:"",
        street_wealth:1,
        encounter_tag:(tag) => false
      };

      const barks = Deck.fromJson(loadJsonFile("Barks.jsonc"), context);
      barks.refresh();
      assert.notEqual(null, barks.draw());
    });

    it('testing street system', () => {

      const context = {
        street_id:"",
        street_wealth:0,
        street_tag:(tag) => false,
        encounter_tag:(tag) => false
      };

      const streets = Deck.fromJson(loadJsonFile("Streets.jsonc"), context);
      const encounters = Deck.fromJson(loadJsonFile("Encounters.jsonc"), context);
      const barks = Deck.fromJson(loadJsonFile("Barks.jsonc"), context);

      let setStreet = (street) => {
        context.street_id = street.id;
        context.street_wealth = street.content.wealth;
        context.street_tag = (tag) => {return street.content.tags.includes(tag)};
        console.log(`Location: "${street.content.title}"`);
      };

      let doEncounter = (street) => {
        setStreet(street);
        // We're on a new street, so shuffle the encounters deck to only include relevant cards.
        encounters.refresh();
        //console.log(encounters.dumpDrawPile());
        let encounter = encounters.draw();
        context.encounter_tag = (tag) => {
          if (!encounter.content.tags)
            return false;
          return encounter.content.tags.includes(tag);
        };
        console.log(`  Encounter: "${encounter.content.title}"`);
        barks.refresh();
        //console.log(barks.dumpDrawPile());
        let bark = barks.draw();
        if (bark) {
          console.log(`  Comment: "${bark.content.comment}"`);
        }
      }

      // First encounter - this should pull out a "start" location.
      streets.refresh((street)=>{return street.content.tags.includes("start")});
      let street = streets.draw();
      doEncounter(street);

      assert.equal(true, street.id=="docks"||street.id=="market"||street.id=="bridge");

      // Reshuffle the deck so that all streets are fair game.
      streets.refresh();

      // Walk through the street deck and pull an encounter for each location
      for (let i=0;i<11;i++) {
          street = streets.draw();
          doEncounter(street);
      }

      // We should have encountered the noble at least once!
      //assert.equal(true, context.noble_storyline>0);

    });

  });  
});
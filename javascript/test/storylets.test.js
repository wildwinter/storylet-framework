// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadJsonFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {Deck} from "../src/storylets.js";

describe('Storylets', () => {

  describe('SimpleStreets', () => {
    it('basic calls', () => {

      let dump_eval = [];

      const json = loadJsonFile("Streets.jsonc");
      const deck = Deck.fromJson(json, {}, true, dump_eval);

      let card = deck.draw();
      assert.notEqual(null, card);

      card = deck.draw();
      assert.notEqual(null, card);

      console.log(dump_eval.join('\n'));

    });

    it('testing barks', () => {

      const context = {
        street_id:"",
        street_wealth:1,
        encounter_tag:(tag) => false
      };

      const barks = Deck.fromJson(loadJsonFile("Barks.jsonc"), context);
      //console.log(barks.dumpDrawPile());
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
        encounters.reshuffle();
        //console.log(encounters.dumpDrawPile());
        let encounter = encounters.draw();
        context.encounter_tag = (tag) => {
          if (!encounter.content.tags)
            return false;
          return encounter.content.tags.includes(tag);
        };
        console.log(`  Encounter: "${encounter.content.title}"`);
        barks.reshuffle();
        //console.log(barks.dumpDrawPile());
        let bark = barks.draw();
        if (bark) {
          console.log(`  Comment: "${bark.content.comment}"`);
        }
      }

      // First encounter - this should pull out a "start" location.
      streets.reshuffle((street)=>{return street.content.tags.includes("start")});
      let street = streets.draw();
      doEncounter(street);

      assert.equal(true, street.id=="docks"||street.id=="market"||street.id=="bridge");

      // Reshuffle the deck so that all streets are fair game.
      streets.reshuffle();

      let path = [];

      // Walk through the street deck and pull an encounter for each location
      for (let i=0;i<11;i++) {
          street = streets.draw();
          path.push(street.id);
          doEncounter(street);
      }

      // We should have encountered the noble at least once!
      assert.equal(true, path.includes("market")||path.includes("slums")||path.includes("bridge"));
    });

  });  

  describe('AsyncReshuffles', () => {
    it('basic async', () => {

      let dump_eval = [];

      const context = {
        street_id:"",
        street_wealth:1,
        encounter_tag:(tag) => false
      };

      const barks = Deck.fromJson(loadJsonFile("Barks.jsonc"), context, /* reshuffle */ false);
      barks.reshuffleAsync(()=>console.debug("Async reshuffle complete."),null,dump_eval);

      while (barks.asyncReshuffleInProgress()) {
        barks.update();
      }

      console.log(barks.dumpDrawPile());

      let card = barks.draw();
      assert.notEqual(null, card);

      card = barks.draw();
      assert.notEqual(null, card);

      console.log(dump_eval.join('\n'));

    });
  });

});
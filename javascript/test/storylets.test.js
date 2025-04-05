// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadJsonFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {deckFromJson} from "../src/jsonLoader.js";

describe('Storylets', () => {

  describe('SimpleStreets', () => {
    it('basic calls', () => {

      let dump_eval = [];

      const json = loadJsonFile("Streets.jsonc");
      const deck = deckFromJson(json, {}, dump_eval);

      const drawn = deck.drawAndPlay(2);
      assert.notEqual(null, drawn.shift());
      assert.notEqual(null, drawn.shift());

      console.log(dump_eval.join('\n'));

    });

    it('testing barks', () => {

      const context = {
        street_id:"",
        street_wealth:1,
        encounter_tag:(tag) => false
      };

      const barks = deckFromJson(loadJsonFile("Barks.jsonc"), context);
      //console.log(barks.dumpDrawPile());
      assert.notEqual(null, barks.drawAndPlaySingle());
    });

    it('testing street system', () => {

      const context = {
        street_id:"",
        street_wealth:0,
        street_tag:(tag) => false,
        encounter_tag:(tag) => false
      };

      const streets = deckFromJson(loadJsonFile("Streets.jsonc"), context);
      const encounters = deckFromJson(loadJsonFile("Encounters.jsonc"), context);
      const barks = deckFromJson(loadJsonFile("Barks.jsonc"), context);

      let setStreet = (street) => {
        context.street_id = street.id;
        context.street_wealth = street.content.wealth;
        context.street_tag = (tag) => {return street.content.tags.includes(tag)};
        console.log(`Location: "${street.content.title}"`);
      };

      let doEncounter = (street) => {
        setStreet(street);
        // We're on a new street, so shuffle the encounters deck to only include relevant cards.
        let encounter = encounters.drawAndPlaySingle();
        context.encounter_tag = (tag) => {
          if (!encounter.content.tags)
            return false;
          return encounter.content.tags.includes(tag);
        };
        console.log(`  Encounter: "${encounter.content.title}"`);
        let bark = barks.drawAndPlaySingle();
        if (bark) {
          console.log(`  Comment: "${bark.content.comment}"`);
        }
      }

      // First encounter - this should pull out a "start" location.
      let street = streets.drawAndPlaySingle((street)=>{return street.content.tags.includes("start")});
      doEncounter(street);

      assert.equal(true, street.id=="docks"||street.id=="market"||street.id=="bridge");

      // Reshuffle the deck so that all streets are fair game.
      let streetsDrawn = streets.draw();

      let path = [];

      // Walk through the street deck and pull an encounter for each location
      for (let i=0;i<streetsDrawn.length;i++) {
          street = streetsDrawn[i];
          street.play();
          path.push(street.id);
          doEncounter(street);
      }

      assert.equal(true, path.includes("market")||path.includes("slums")||path.includes("bridge"));
    });

  });  

});
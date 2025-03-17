// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import {loadTestFile} from '../test/testUtils.js';
import {strict as assert} from 'assert';
import {Storylets} from "../src/storylets.js";

describe('Storylets', () => {

  describe('Simple', () => {
    it('should match', () => {

      const source = loadTestFile("Streets.json");
      const json = JSON.parse(source);

      const storylets = new Storylets();
      storylets.loadJson(json);

      //assert.equal(result, true);
    });
  });
});
// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

export class Storylet {

  constructor(id) {
    this.id = id;
    this.tags = [];
    this.condition = "";
    this.priority = 0;
    this.content = {};
  }

  static fromJson(json) {

    if (!("id" in json))
      throw new Error("No 'id' property in the storylet JSON.", json);

    const storylet = new Storylet(json.id);

    if ("tags" in json)
      storylet.tags = json.tags;
    if ("condition" in json)
      storylet.condition = json.condition;
    if ("priority" in json)
      storylet.priority = json.priority;
    if ("content" in json)
      storylet.content = json.content;
    return storylet;

  }

}

export class Storylets {

  constructor() {
    this._all = [];
    this._toProcess = [];
    this._available = new Map();
    this._onComplete = null;
    this._drawCount = 0;
  }

  loadJson(json) {

    for (const item of json) {
      const storylet = Storylet.fromJson(item);
      this._all.push(storylet);
    }
  }

  startDraw(context, tags, count, onComplete) {
    this._available.clear();
    this._onComplete = onComplete;
    this._toProcess = this._all.slice();
    this._drawCount = count;
  }

  update() {
    this._processChunk();
    if (this._toProcess.length>0)
      return true;
    return false;
  }

  _processChunk() {

    if (this._toProcess==0)
      return;

    const CHUNK_SIZE=2;

    for (let i=0;i<CHUNK_SIZE && this._toProcess.length>0;i++) {
      const storylet = this._toProcess.shift();
      if (!this._available.has(storylet.priority))
        this._available.set(storylet.priority, []);
      this._available.get(storylet.priority).push(storylet);
    }

    if (this._toProcess.length>0)
      return;

    const drawCandidates = [];
    const sortedPriorities = [...this._available.keys()].sort((a, b) => a - b);
    for (const priority of sortedPriorities) {
      const bucket = this._available.get(priority);
      const shuffledBucket = shuffle(bucket);
      drawCandidates.push(...shuffledBucket);
    }

   const drawResults = drawCandidates.slice(0, this._drawCount);
    if (this._onComplete) {
      const onComplete = this._onComplete;
      this._onComplete = null;
      onComplete(drawResults);
    }
  }

}

function shuffle(array) {
  for (let i = array.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [array[i], array[j]] = [array[j], array[i]];
  }
  return array;
}
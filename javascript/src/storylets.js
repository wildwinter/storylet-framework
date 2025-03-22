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

export class Deck {

  constructor() {
    this._all = new Map();

    this._shuffled = [];
  }

  loadJson(json) {
    for (const item of json) {
      const storylet = Storylet.fromJson(item);
      if (this._all.has(storylet.id)) {
        throw new Error(`Duplicate storylet id: '${storylet.id}'.`, item);
      }
      this._all.set(storylet.id, storylet);
    }
  }

  async refresh(context) {

    this._shuffled = [];
    const priorityAvailable = new Map();
    const toProcess = [...this._all.values()];

    while (toProcess.length>0) {
      const storylet = toProcess.shift();
      if (!priorityAvailable.has(storylet.priority))
        priorityAvailable.set(storylet.priority, []);
      priorityAvailable.get(storylet.priority).push(storylet);
    }

    const sortedPriorities = [...priorityAvailable.keys()].sort((a, b) => a - b);
    for (const priority of sortedPriorities) {
      const bucket = priorityAvailable.get(priority);
      const shuffledBucket = shuffle(bucket);
      this._shuffled.push(...shuffledBucket);
    }

  }

  draw(count) {
    return this._shuffled.slice(0, count);
  }

  take(id) {
    const index = this._available.findIndex(storylet => storylet.id === id);
    if (index !== -1) {
      this._available.splice(index, 1);
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
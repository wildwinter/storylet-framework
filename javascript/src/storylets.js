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
    this._all = {}
  }

  loadJson(json) {

    for (const item of json) {
      const storylet = Storylet.fromJson(item);
      this._all[storylet.id] = storylet;
    }

    console.log(this._all);

  }

}
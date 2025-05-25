# storylet-framework
**storylet-framework** is a multi-language implementation of a simple storylet framework. It allows you to specify sets of storylets in JSON files, and run code that will evaluate at runtime which storylets should be available in your current context.

This is designed to be cross-platform and implementation agnostic when it comes to the actual contents of your storylets.

What's it for? To do things like this:

```javascript
import { deckFromJson } from './storyletFramework.js';

const context = {
    floor_level:0,
    on_fire:false
};

const rooms = deckFromJson(loadJsonFile("Rooms.jsonc"), context);

console.log(rooms.drawAndPlaySingle().content.title); // "The Kitchen"
console.log(rooms.drawAndPlaySingle().content.title); // "The Scullery"
console.log(rooms.drawAndPlaySingle().content.title); // "An Ordinary Cupboard"
console.log(rooms.drawAndPlaySingle().content.title); // "The Entry Hall"

context.floor_level = 1;

const moreCards = rooms.drawAndPlay(4);
console.log(moreCards[0].content.title); // "The Bedroom"
console.log(moreCards[1].content.title); // "An Ordinary Cupboard"
console.log(moreCards[2].content.title); // "The Bathroom"
console.log(moreCards[4].content.title); // "The Upstairs Lounge"

context.on_fire = true;

console.log(rooms.drawAndPlaySingle().content.title); // "The Bedroom, But It's On Fire!"
console.log(rooms.drawAndPlaySingle().content.title); // "An Ordinary Cupboard"
console.log(rooms.drawAndPlaySingle().content.title); // "The Upstairs Lounge"

```

```jsonc
{
  "storylets":
  [
    // Some downstairs rooms
    {"id": "downstairs1", "condition":"floor_level==0", "content": { "title":"The Kitchen" }},
    {"id": "downstairs2", "condition":"floor_level==0", "content": { "title":"The Scullery" }},
    {"id": "downstairs3", "condition":"floor_level==0", "content": { "title":"The Entry Hall" }},
    // Could be anywhere
    {"id": "cupboard", "content": { "title":"An Ordinary Cupboard" }},
    // Some upstairs rooms - let's add them in a batch since they share a setting
    {
        "defaults": { "condition":"floor_level==1"},
        "storylets": [
            {"id": "upstairs1", "content": { "title":"The Bathroom" }},
            {"id": "upstairs2", "content": { "title":"The Upstairs Lounge" }},
        ]
    }
    {"id": "upstairs3_normal", "condition":"floor_level==1 and not on_fire", "content": { "title":"The Bedroom" }},
    {"id": "upstairs3_burny", "condition":"floor_level==1 and on_fire", "content": { "title":"The Bedroom, But It's On Fire!" }},   
  ]
}
```
Or this:

```javascript
import { deckFromJson } from './storyletFramework.js';

const context = {
    monsters_nearby:false
};

const barks = deckFromJson(loadJsonFile("Barks.jsonc"), context);

console.log(barks.drawAndPlaySingle().content.text); // "This place looks fine, no monsters here."
console.log(barks.drawAndPlaySingle().content.text); // "Can't see a monster. Isn't this great?"

context.monsters_nearby = true;

console.log(barks.drawAndPlaySingle().content.text); // "Whoah, monsters! Look out!"
console.log(barks.drawAndPlaySingle().content.text); // "Monsters, sir! Thousands of 'em!"

```

### Contents
* [The Basics](#the-basics)
* [Source Code](#source-code)
* [Releases](#releases)
* [Usage](#usage)
    * [Overview](#overview)
    * [Storylet JSON format](#storylet-json-format)
    * [Expressions](#expressions)
    * [Specificity](#specificity)
    * [Filtering](#filtering)
    * [Debugging](#debugging)
    * [Javascript as an ES6 module](#javascript-as-an-es6-module)
    * [Javascript in a browser](#javascript-in-a-browser)
    * [Python](#python)
    * [C#](#c)
    * [C++](#c-1)
* [Contributors](#contributors)
* [License](#license)

## The Basics
A **storylet** is a little chunk of experience. A scene, or a line, or a random encounter - whatever you want it to be. The important thing is that it has a *condition* on the front which gets tested to figure out if that chunk of experience is available right now.

See my article [over here](https://wildwinter.medium.com/storylets-explained-ff5a24842bd9) for more info.

The best metaphor I've heard for these are a set of *cards* in a *deck*. Think of the boardgames you know where you draw a card for the next room, or the next counter, or the item that you've picked up.

This framework lets you define the conditions for a storylet, and when you ask to *draw* storylets from a deck, it is reshuffled into a draw pile of *what is a valid card right now* before your cards are issued.

When you *play* a storylet, it is marked within the deck as to how long it will be before it can be drawn again, depending on the *redraw* rules you give it.

The framework also contains rules for **managing priority** (which card is most appropriate right now? bubble it to the top), and various other bits and pieces.

The code here is available as Javascript, C++, C#, and Python and should behave the same way for each.

## Source Code
The source can be found on [Github](https://github.com/wildwinter/storylet-framework), and is available under the MIT license.

## Releases
Releases are available in the releases area in [Github](https://github.com/wildwinter/storylet-framework/releases) and are available for multiple platforms:
* Javascript - a JS file for use in ESM modules, and a minified JS file for use in a browser.
* Python - a Python package for import into other Python files.
* C# - a DotNET DLL for use in any C# project
* C++ - a set of source files for you to compile yourself

## Usage

### Overview

* Create your `context`, which can hold properties with values, or hold functions that can be called by expressions. This is implemented by my [expression-parser](https://github.com/wildwinter/expression-parser).
* Load a deck of cards from JSON into a `Deck`, passing it your context.
* Draw cards from the deck, as needed, by calling `draw()`, `drawSingle()` etc.
* When you use a card, call `play()` on it to make the deck aware that that card can't be drawn again until it fits your rules.

### Storylet JSON Format

#### Basic Storylet
The basic storylet format is this:
```jsonc
{
    // Mandatory - textual ID of your choice, unique within your deck
    "id": "xxx", 
    // Optional - if false when the deck is shuffled, the storylet is ignored. Defaults to true.
    "condition": "xxx or yyy and some_call()", 
    // Optional. Can be an expression instead of a number, in which case it's evaluated at shuffle time. The higher the priority, the closer the card is to the top of the deck. Cards of the same priority are in random order. Defaults to 0.
    "priority": 1, 
    // Optional. How many times can draw() be called before this storylet becomes valid again?
    // Can be strings "always", "never", or a number > 0. Defaults to "always".
    "redraw": 4,
    // Optional - your app-specific content, whatever that might be.
    "content": {/*something*/},
    // Optional - gives you possible outcomes/side-effects for when the story is played.
    // Each outcome consists of a set of changes to the context values.
    // If there is an outcome called 'default' it will be triggered automatically when
    // play() happens. Call play("otherOutcome") to make something else happen.
    "outcomes:"{
        "default": {
            "someVal":15,
            "someFlag":true,
            "someIncrementalProp":"someIncrementalProp+1"
        }
    }

}
```

As you can see, a minimal storylet could just be:
```javascript
{"id":"myStory"}
```

But that isn't much use to anyone. Your `content` is probably most important.

#### Packets
The default JSON file is arranged into a **Packet**, which is shaped like this:
```jsonc
{
    "storylets":
    [
        {"id":"myStory1", "content":{"title":"Storylet 1"}},
        {"id":"myStory2", "content":{"title":"Storylet 2"}},
        {"id":"myStory3", "content":{"title":"Storylet 3"}}
    ]
}
```
A packet can also contain a `context` section, which is declaring initial variable values for new properties to add to your context (otherwise if you refer to those variables in tests the system will throw an error). You only need to do this for variables which aren't already in your context. For example:
```jsonc
{
    "context": {
        "played_funky_storylet":false // Declare this var so expressions can use it legitimately later
    },
    "storylets":
    [
        {"id":"myStory1", 
            "content":{"title":"Storylet 1"}, 
            "outcomes":{"default":{"played_funky_storylet":true}}  // Set this var when this storylet is played
        },
        {"id":"myStory2", 
            "condition":"!played_funky_storylet", // Only make this storylet available when funky storylet hasn't played!
            "content":{"title":"Storylet 2"}
        },
        {"id":"myStory3",
            "condition":"playerType=='orc'",    // No need to declare this in the context above, as the context supplied by the engine has it already.
            "content":{"title":"Storylet 3"}
        }
    ]
}
```
(The above assumes that the context you supplied when creating the deck in code was something like `{playerType:"orc"}`).

A packet can also contain a `defaults` section, which means that is gives default values for all of the storylets in the packet. For example:
```jsonc
{
    "context": {
        "played_funky_storylet":false // Declare this var so expressions can use it legitimately later
    },
    "defaults": {
        "redraw":"never"
    },
    "storylets":
    [
        {"id":"myStory1", 
            /* "redraw": "never" is automatically applied now. */
            "content":{"title":"Storylet 1"}, 
            "outcomes":{"default":{"played_funky_storylet":true}}  // Set this var when this storylet is played
        },
        {"id":"myStory2", 
            /* "redraw": "never" is automatically applied now. */
            "condition":"!played_funky_storylet", // Only make this storylet available when funky storylet hasn't played!
            "content":{"title":"Storylet 2"}
        },
        {"id":"myStory3", 
            /* "redraw": "never" is automatically applied now. */
            "content":{"title":"Storylet 3"}
        }
    ]
}
```

#### Packets All The Way Down 

At any point, instead of specifying a storylet, you can specify a packet of storylets. In the end it still means that individual storylets get added to the deck - this is just a simple way of applying the same defaults to a whole collection of storylets. So this is valid:
```jsonc
{
    "storylets":
    [
        {"id":"myStory1", "priority":3, "content":{"title":"Storylet 1"}},
        {"id":"myStory2", "priority":"level+1", "content":{"title":"Storylet 2"}},
        {
            "defaults": {
                "condition":"player_on_fire"
            },
            "storylets": [
                /* player_on_fire condition applies to all of these */
                {"id":"myStory3", "content":{"title":"Storylet 3"}},
                {"id":"myStory4", "content":{"title":"Storylet 4"}},
                {"id":"myStory5", "content":{"title":"Storylet 5"}},
                {
                    "defaults": {"priority":1},
                    "storylets": [
                        /* player_on_fire condition AND priority 1 applies to all of these */
                        {"id":"myStory6", "content":{"title":"Storylet 6"}},
                        {"id":"myStory7", "content":{"title":"Storylet 7"}},
                    ]
                }
            ]
        }
    ]
}
```
### Expressions
Expressions used in conditions or priorities or `updateOnPlayed` are implemented by my [expression-parser](https://github.com/wildwinter/expression-parser) library.

This lets you have simple expressions and also get values from your `context` or call functions from your `context`. 
Common operators such as `+, -, /, *, and, or, !, not, ==, !=, >=`, brackets and so on all work as expected.
```jsonc
"condition":"color=='red'"
"condition":"playerRace('orc') and (on_fire or water_level>=5)"

"outcomes":{
    "default": {
        "someCounter":"someCounter+1"
    }
}
```

### Specificity
There are some situations (for example, bark implementations) where it's quite useful to have the priority of a storylet to be defined by how *specific* the condition is. That is, the more clauses in the condition, the more relevant the storylet probably is to the current situation.

You can turn this on for the deck using `useSpecificity=true`. Priorities still take precedence, but within each priority storylets are sorted by specificity.

So in this:
```jsonc
[
{"id":"fireComment", 
    "condition":"on_fire", "content":{"comment":"There's a fire!"}}
{"id":"hatOnFireComment", 
    "condition":"on_fire and wearing_hat", 
    "content":{"comment":"Your hat is on fire!"}},
{"id":"hatComment", 
    "condition":"wearing_hat", "content":{"comment":"Nice hat!"}}
]
```
'Your hat is on fire' will *always* be prioritised assuming those two conditions are true.

### Filtering
`draw()` lets you pass a `filter` function which can be used to check each storylet (and its contexts). If the filter function returns false, that storylet will be ignored.

### Outcomes
You can specify a set of **outcomes** which are the side effects that should happen when the storylet is played.

Each outcome consists of a set of *changes to the context* that you can make, i.e. updates to the world state.

If you specify an outcome called "default" then the contents of that will happen, by default.

However, your storylet might have player choices. Kill troll, or not kill the troll. In which case, when you call `play()`, you pass an outcome e.g. `play("killTroll")` and the resulting updates to the storylet are made.

```jsonc
    {"id":"trollEncounter", 
        "content":{"title":"Storylet 1"}, 
        "outcomes":{
            "killTroll:{
                "troll_dead":true, 
                "reputation":"reputation+1", 
                "war_started":true
            },
            "saveTroll:"{
                "hero_to_trolls":true,
                "reputation":"reputation-1", 
            }
        }
    }
```

**Motivation:** Explicit world state changes like this make it much easier to figure out which storylets affect the world state, and everything becomes easier to debug and to write tooling for! Think of it as similar to pre/postconditions.

### Debugging
There are some hopefully helpful features for debugging. 

You can pass an empty array of strings in the `dump_eval` param to `draw()` or various other places and information on evaluations and processing that have been performed on different storylets will be added to the array.

### Javascript as an ES6 module
```javascript
import { deckFromJson } from './storyletFramework.js';

const text = readFileSync("Rooms.jsonc", 'utf-8');
const json = JSON.parse(text);

const context = {
    some_value:0,
    some_fn: (param) => {return some_response(param);}
}

const deck = deckFromJson(json, context);
let card = deck.drawAndPlaySingle();
console.log(card.id);
```

### Javascript in a browser
Either you can use the same module / ESM format (`storyletFramework.js`):
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Storylet Framework</title>
</head>
<body>
    <script type="module">
        import { Deck } from './storyletFramework.js';

        const json = gotSomeJsonFromSomewhere();

        const context = {
            some_value:0,
            some_fn: (param) => {return some_response(param);}
        }

        const deck = deckFromJson(json, context);
        let card = deck.drawAndPlaySingle();
        console.log(card.id);
    </script>
</body>
</html>
```
Or you can use a minified IIFE version (`storyletFramework.min.js`):
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Storylet Framework</title>
    <script src="storyletFramework.min.js"></script>
</head>
<body>
    <script>
        const json = gotSomeJsonFromSomewhere();

        const context = {
            some_value:0,
            some_fn: (param) => {return some_response(param);}
        }

        // Access the global StoryletFramework object
        const deck = StoryletFramework.deckFromJson(json, context);
        let card = deck.drawAndPlaySingle();
        console.log(card.id);
    </script>
</body>
</html>
```

### Python
```Python
from storylet_framework.json_loader import deck_from_json

json_data = load_json_file("Rooms.jsonc")

context = {
    "some_value": 0,
    "some_fn": lambda param: some_response(param)
}

deck = deck_from_json(json_data, context)
card = deck.draw_and_play_single()
print(card.id)

```

### C#
Install the DLL in your project, and use it like so:
```CSharp
using System; 
using StoryletFramework;

class Program
{
    static void Main(string[] args)
    {
        var context = new Dictionary<string, object>
        {
            { "some_value", 0 },
            { "some_fn", new Func<string, bool>(param => someResponse(param)) }
        };

        Dictionary<string, object> json = loadJsonAsDictSomehow("Rooms.jsonc);
        var deck = JsonLoader.DeckFromJson(json, context);
        var card = deck.DrawAndPlaySingle();
        Console.WriteLine(card.Id);
    }
}
```

### C++
I haven't supplied any built libs (because building multiplatform libs is outside my scope right now). Instead I have supplied source code in the zip - you should be able to build and use it with your project.

```cpp
#include "storylet_framework/json_loader.h"
#include <iostream>
#include <string>

using namespace StoryletFramework;

int main() {

    nlohmann::json json = loadJsonFile("Streets.jsonc");
    StoryletFramework::Context context;
    context["some_value"] = 0;
    context["some_fn"] = ExpressionParser::make_function_wrapper(
        [](const std::string& param) { return some_response(param); });

    std::shared_ptr<Deck> deck = DeckFromJson(json, &context, &dumpEval);
    auto card = deck->DrawAndPlaySingle();
    std::cout << card->id << '\n';
    return 0;
}
```

## Contributors
* [wildwinter](https://github.com/wildwinter) - original author

## License
```
MIT License

Copyright (c) 2025 Ian Thomas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
# expression-parser
**expression-parser** is a set of libraries to parse and evaluate simple comparison expressions from text, including variable references and function calls.

It is used to have a simple expression language for conditionals that is identical across different plaforms, so that data that includes conditionals can be agnostic. These libraries are written in **C++**, **Javascript**, **Python**, and **C#**.

```
(location=="spain" and is_day_time) or spell_power("jamie")>12
```

### Contents
* [The Basics](#the-basics)
* [Source Code](#source-code)
* [Releases](#releases)
* [Usage](#usage)
    * [Overview](#overview)
    * [Javascript as an ES6 module](#javascript-as-an-es6-module)
    * [Javascript in a browser](#javascript-in-a-browser)
    * [Python](#python)
    * [C#](#c)
    * [C++](#c-1)
* [Contributors](#contributors)
* [License](#license)

## The Basics
I have some use cases where I have external data stored with a **conditional expression** attached to that data. For example:
```XML
<story>
    <available_when>day=="saturday" and (character=="dave" or relationships_stats("orcs")>1)</valid_when>
    <title>The Orcish Love Song</title>
    :
</story>

<dialogue>
    <play_when>time_elapsed>=3.0 and batsignal_called()</play_when>
    <character>Gordon</character>
    <line>Is that... the bat signal?</line>
</dialogue>
```

I don't want to have to embed Lua in the engine(s) that are consuming that data to validate the expression. I don't want to have to constantly re-invent the wheel. And I don't want a complex set of nested condition nodes in my format.

So I've hived off a very simple generic expression parser and executor which allows your code to parse the expression and then evaluate it, calling out to your context to get variables and functions as needed.

Here's a noddy example, in Javascript:
```Javascript
let parser = new ExpressionParser();
let expression = parser.parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

let context = {
"get_name": () => "fred",
"counter": 1
};

let result = expression.evaluate(context);
// Result will be true!
```

The code here is available as Javascript, C++, C#, and Python and should behave the same way for each.

## Source Code
The source can be found on [Github](https://github.com/wildwinter/expresison-parser), and is available under the MIT license.

## Releases
Releases are available in the releases area in [Github](https://github.com/wildwinter/expression-parser/releases) and are available for multiple platforms:
* Javascript - a JS file for use in ESM modules, and a minified JS file for use in a browser.
* Python - a Python package for import into other Python files.
* C# - a DotNET DLL for use in any C# project
* C++ - a set of source files for you to compile yourself

## Usage

### Overview
* Use `ExpressionParser.Parser`'s method `parse("some string")` to return an `ExpressionNode` object.
* Create a context - a named map or dictionary of variables and function calls.
* You can then call `evaluate(context)`, which will return the result of the expression.

Expressions can handle **string**, **numbers**, or **boolean** types. If you're returning variables or function calls you should be dealing in those types.

Supported values are:
| Value(s)         | Description                                                                                          |
|---------------------|------------------------------------------------------------------------------------------------------|
| `"string"` or `'string'`           | Literal string.|
| numeric e.g. `5.0`, `-27`           | Literal number.|
| `true` or `false`                  | Literal boolean.|
| `some_variable` | Variable name which will be looked up and the value retrieved from the **context** supplied to `evaluate()`|
| `some_function("args", are_optional, 57)` | Function call which will be looked up and called via the **context** supplied to `evaluate()`|

The expression evaluation is fairly permissive - for operations typically the evaluator figures out that if you are e.g. comparing a number with another value, the second value should also be a number and will try to convert it. 

Supported operators are:
| Operator(s)         | Description                                                                                          |
|---------------------|------------------------------------------------------------------------------------------------------|
| `(`, `)`            | Parentheses used to group expressions and control evaluation order.                                |
| `+`                 | Addition.                                                                        |
| `-`                 | Subtraction or negation (i.e. `-bucket` is valid, taking the value of the bucket var and making it negative).                           |
| `*`                 | Multiplication.                                                             |
| `/`                 | Division.                                                                     |
| `==` or `=`         | Equality; both forms are accepted (with `=` treated like `==`).               |
| `!=`                | Not-equal.                                                                  |
| `>`                 | Greater-than.                                                            |
| `<`                 | Less-than.                                                                  |
| `>=`                | Greater-than-or-equal.                                             |
| `<=`                | Less-than-or-equal.                                                   |
| `and` or `&&`       | Logical AND =.                                                                      |
| `or` or `\|\|`        | Logical OR.                                                                       |
| `not` or `!`        | Logical NOT.                                                        |

#### Helpful Extras
* **For debugging:** You can use `expression.dump_structure()` to see what the parser has actually parsed.
* **For debugging:** When evaluating, you can pass a `dump_eval` string array, and the parser will then write down the steps its taken as it evaluates.
* **Writing:** If for some crazy reason you want to, you can use the `write()` function to turn the parsed expression back into a string. The `Writer.StringFormat` settings will let you say how you want string values to be delimited.

### Javascript as an ES6 module
```javascript
import { ExpressionParser } from './expressionParser.js';

const parser = new ExpressionParser();
let expression = parser.parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

let context = {
"get_name": () => "fred",
"counter": 1
};

let result = expression.evaluate(context);

assert.equal(result, true);
```

### Javascript in a browser
Either you can use the same module / ESM format (`expressionParser.js`):
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Expression Parser</title>
</head>
<body>
    <script type="module">
        import { ExpressionParser } from './expressionParser.js';

        const parser = new ExpressionParser();
        let expression = parser.parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

        let context = {
        "get_name": () => "fred",
        "counter": 1
        };

        let result = expression.evaluate(context);

        assert.equal(result, true);
    </script>
</body>
</html>
```
Or you can use a minified IIFE version (`expressionParser.min.js`):
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Expression Parser</title>
    <script src="expressionParser.min.js"></script>
</head>
<body>
    <script>
        // Access the global ExpressionParser object
        const parser = new ExpressionParser.Parser();
        let expression = parser.parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

        let context = {
        "get_name": () => "fred",
        "counter": 1
        };

        let result = expression.evaluate(context);

        assert.equal(result, true);
    </script>
</body>
</html>
```

### Python
```Python
from expression_parser.parser import Parser

parser = Parser()
expression = parser.parse("get_name()=='fred' and counter>0 and 5/5.0!=0")

context = {
    "get_name":lambda: "fred",
    "counter": 1
}

result = expression.evaluate(context)

```

### C#
Install the DLL in your project, and use it like so:
```CSharp
using System; 
using ExpressionParser;

class Program
{
    static void Main(string[] args)
    {
        var parser = new Parser();
        var expression = parser.Parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

        var context = new Dictionary<string, object>
        {
            { "get_name", new Func<string>(() => "fred") },
            { "counter", 1 }
        };

        var result = expression.Evaluate(context);
    }
}
```

### C++
I haven't supplied any built libs (because building multiplatform libs is outside my scope right now). Instead I have supplied source code in the zip - you should be able to build and use it with your project.

```cpp
#include "expression_parser/parser.h"
#include <iostream>
#include <string>

using namespace ExpressionParser;

int main() {
    // Create an instance of ExpressionParser::Parser
    Parser parser;

    auto expression = parser.Parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

    Context context;
    // make_function_wrapper is a handy helper to make supplying a function 
    //    easier and less error-prone at runtime.
    context["get_name"] = make_function_wrapper([]() -> std::string {
        return "fred";
    });
    context["counter"] = 1;

    std::any result = expression->Evaluate(context);

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
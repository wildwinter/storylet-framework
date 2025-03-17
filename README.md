# storylet-framework
**storylet-framework** blah blah.

```
exmaple here.
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
XXXX

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
XXX

### Javascript as an ES6 module
```javascript
import { StoryletFramework } from './storyletFramework.js';

const storylets = new StoryletFramework();

XXX

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
        import { StoryletFramework } from './storyletFramework.js';

        const parser = new StoryletFramework();
        TBD
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
        // Access the global StoryletFramework object
        const parser = new StoryletFramework.Parser();
        TBD
    </script>
</body>
</html>
```

### Python
```Python
from storylet_framework.storylets import Storylets

XXX

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
        var storylets = new Storylets();
XXX
    }
}
```

### C++
I haven't supplied any built libs (because building multiplatform libs is outside my scope right now). Instead I have supplied source code in the zip - you should be able to build and use it with your project.

```cpp
#include "storylet_framework/storylets.h"
#include <iostream>
#include <string>

using namespace StoryletFramework;

int main() {
    // Create an instance of StoryletFramework::Storylets
    Storylets storylets;

    XXX

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
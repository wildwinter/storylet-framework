// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2024 Ian Thomas

import {readFileSync} from 'fs';

function stripJSONComments(jsonText) {
    const withoutBlockComments = jsonText.replace(/\/\*[\s\S]*?\*\//g, '');
    const withoutComments = withoutBlockComments.replace(/\/\/.*$/gm, '');
    return withoutComments;
}

export function loadTestFile(fileName) {
    return readFileSync("../tests/"+fileName, 'utf-8');
}

export function loadJsonFile(fileName) {
    let text = loadTestFile(fileName);
   text = stripJSONComments(text);
    return JSON.parse(text);
}
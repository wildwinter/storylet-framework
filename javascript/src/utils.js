// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

export function shuffleArray(array) {
  for (let i = array.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [array[i], array[j]] = [array[j], array[i]];
  }
  return array;
}

export function strToNumeric(str) {
    const numericRegex = /^-?\d+(\.\d+)?$/;
    if (numericRegex.test(str)) {
      return parseFloat(str);
    }
    return null;
}
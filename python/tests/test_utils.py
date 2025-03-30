# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import json
import re
from pathlib import Path

def strip_json_comments(json_text):
    """Remove block and line comments from a JSON string."""
    without_block_comments = re.sub(r'/\*[\s\S]*?\*/', '', json_text)
    without_comments = re.sub(r'//.*$', '', without_block_comments, flags=re.MULTILINE)
    return without_comments

def load_test_file(file_name):
    """Load the content of a test file."""
    test_file_path = Path("../tests") / file_name
    with test_file_path.open("r", encoding="utf-8") as file:
        return file.read()

def load_json_file(file_name):
    """Load and parse a JSON file, stripping comments."""
    text = load_test_file(file_name)
    text = strip_json_comments(text)
    return json.loads(text)
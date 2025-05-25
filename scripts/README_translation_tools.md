# Translation Checker Tools

This directory contains tools for checking and managing translations in the TaterClient-ddnet project.

## Overview

The translation checking tools help maintain translation completeness by:

1. Extracting `TCLocalize()` strings from source code
2. Comparing with translation files
3. Generating reports for missing and obsolete translations
4. Supporting configurable search paths and output options

## Files

### Main Tools

- **`check_translations.py`** - Main integrated tool that combines extraction and comparison
- **`check_missing_translations.py`** - Legacy comparison tool (already exists)

### Configuration and Documentation

- **`check_translations_config.py`** - Configuration examples for the integrated tool
- **`README_translation_tools.md`** - This documentation file

## Quick Start

### Basic Usage

Run the integrated checker with default settings:

```bash
cd /workspaces/TaterClient-ddnet
python scripts/check_translations.py
```

### Custom Configuration

1. Edit the configuration section in `check_translations.py`
2. Modify search paths, translation file, and output options
3. Run the script

Example configuration:

```python
# Search specific files
SEARCH_PATHS = [
    os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "chat.cpp"),
    os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "tclient"),  # entire directory
]

# Translation file
TRANSLATION_FILE = os.path.join(WORKSPACE_ROOT, "data", "tclient", "languages", "simplified_chinese.txt")

# Output options
SAVE_EXTRACTION_RESULT = True   # Save extracted strings
SAVE_COMPARISON_RESULT = True   # Save comparison results
```

## Configuration Options

### Search Paths

You can configure the tool to search:

1. **Specific files**: Individual .cpp files
2. **Directories**: Recursively search all .cpp files in a directory
3. **Mixed**: Combination of files and directories

### Translation Files

Currently supports TClient translation file format:
- Original string on one line
- Translation on the next line starting with `== `

### Output Options

- **Save extraction results**: Optional intermediate file with all extracted strings
- **Save comparison results**: Optional detailed comparison report
- **Console output**: Always shows summary and missing translations

## Output Files

### Extraction Results (`extracted_tc_localize_strings.txt`)

Contains all extracted TCLocalize strings grouped by source file:

```
File: src/game/client/components/chat.cpp (1 matches)
"Translated text hidden due to streamer mode"
------------------------------

Total extracted 283 TCLocalize strings.
Including 254 unique strings and 29 duplicates.
```

### Comparison Results (`translation_comparison_result.txt`)

Contains detailed comparison between source and translation files:

```
Translation Comparison Results
==================================================

Total strings in source code: 254
Total original strings in translation file: 230

Missing translations (exist in source but not in translation file):
--------------------------------------------------
  1. "Background Draw"
  2. "Font Size"
  ...

Total 36 missing translations.
```

## Features

- ✅ **Integrated functionality**: One script for extraction and comparison
- ✅ **Flexible search**: Support for files and directories
- ✅ **Optional output**: Choose whether to save intermediate files
- ✅ **Detailed reporting**: Console and file output
- ✅ **Duplicate handling**: Identifies and counts duplicate strings
- ✅ **Path flexibility**: Works with relative and absolute paths

## Use Cases

### Development Workflow

1. **After adding new features**: Run the checker to identify new translatable strings
2. **Regular maintenance**: Periodic checks to ensure translation completeness
3. **Translation cleanup**: Identify obsolete translations that can be removed

### Translation Management

1. **New language support**: Extract all strings that need translation
2. **Update existing translations**: Find missing translations for existing languages
3. **Quality assurance**: Ensure no translatable strings are missed

## Advanced Usage

### Custom Search Patterns

The tool searches for patterns matching:
```regex
TCLocalize\s*\(\s*"((?:\\.|[^"\\])*)"(?:\s*,\s*"(?:\\.|[^"\\])*")?\s*\)
```

This matches:
- `TCLocalize("text")`
- `TCLocalize("text", "context")`
- Handles escaped quotes and various whitespace

### Path Configuration

Paths are automatically resolved relative to the workspace root. You can use:
- Relative paths: `"src/game/client/components/chat.cpp"`
- Absolute paths: `"/workspaces/TaterClient-ddnet/src/..."`
- Directory paths: `"src/game/client/components/tclient"` (searches recursively)

## Troubleshooting

### Common Issues

1. **File not found errors**: Check that paths are correct relative to workspace root
2. **No strings extracted**: Verify that files contain `TCLocalize()` calls
3. **Translation file format**: Ensure translation file follows the expected format

### Debug Information

The tool provides detailed output showing:
- Number of files processed
- Total extractions vs unique strings
- Specific missing and obsolete translations

## Integration

This tool is designed to integrate with:
- Development workflows (pre-commit hooks, CI/CD)
- Translation management systems
- Code quality checks

For automation, the tool exits with appropriate status codes and provides machine-readable output options.

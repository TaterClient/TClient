# Translation Update Tool

## Overview

The `update_translations.py` script automatically updates translation files by:
1. Reading missing translations from comparison result files
2. Adding missing translation keys to the end of translation files
3. Moving obsolete translations to a dedicated obsolete section
4. Maintaining proper translation file format

## Usage

### Basic Usage
```bash
# Use default files (translation_comparison_result.txt and simplified_chinese.txt)
python scripts/update_translations.py

# Show help
python scripts/update_translations.py --help
```

### Advanced Usage
```bash
# Specify custom result file
python scripts/update_translations.py my_comparison_result.txt

# Specify both result file and translation file
python scripts/update_translations.py result.txt data/tclient/languages/french.txt
```

## File Format

The script expects:
- **Result file**: Output from translation comparison tools with sections for "Missing translations" and "Potentially obsolete translations"
- **Translation file**: Standard TClient translation format with header and `##### translated strings #####` section

## Features

- ✅ Adds missing translations with placeholder values
- ✅ Moves obsolete translations to `##### obsolete strings #####` section
- ✅ Prevents duplicate entries
- ✅ Maintains file structure and formatting
- ✅ Works with any TClient language file
- ✅ Provides detailed progress reporting

## Output

The script will:
1. Add all missing translations to the end of the active translations section
2. Create or update the obsolete section with moved translations
3. Report the number of added and moved translations

## Example

```
Translation Update Tool
======================
Result file: /workspaces/TaterClient-ddnet/translation_comparison_result.txt
Translation file: /workspaces/TaterClient-ddnet/data/tclient/languages/simplified_chinese.txt

Parsing comparison result file...
Found 36 missing translations
Found 12 obsolete translations

Updating translation file...
Successfully updated translation file: /workspaces/TaterClient-ddnet/data/tclient/languages/simplified_chinese.txt
Added 36 missing translations
Moved 12 obsolete translations to obsolete section.

Update completed successfully!
```

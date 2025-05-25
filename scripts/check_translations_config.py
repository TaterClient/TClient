"""
Integrated Translation Checker Configuration Examples
====================================================

This file shows how to configure various options for the check_translations.py script.
"""

import os

# ==================== Basic Configuration ====================

# Workspace root directory (automatically detected from script location)
WORKSPACE_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ==================== Search Path Configuration ====================

# Option 1: Search specific files
SEARCH_SPECIFIC_FILES = [
    "src/game/client/components/tclient/bg_draw.cpp",
    "src/game/client/components/tclient/menus_tclient.cpp",
    "src/game/client/components/chat.cpp",
    "src/game/client/components/menus_settings.cpp",
    "src/game/client/components/tclient/tclient.cpp",
]

# Option 2: Search entire directories (recursively search all .cpp files)
SEARCH_DIRECTORIES = [
    "src/game/client/components/tclient",  # Search entire tclient directory
    "src/game/client/components",  # Search entire components directory
]

# Option 3: Mixed search (files + directories)
SEARCH_MIXED = [
    "src/game/client/components/tclient",  # Entire directory
    "src/game/client/components/chat.cpp",  # Specific file
    "src/game/client/components/menus_settings.cpp",  # Specific file
]

# ==================== Translation File Configuration ====================

# Current translation file
TRANSLATION_FILE = "data/tclient/languages/simplified_chinese.txt"

# Other language translation file examples:
# TRANSLATION_FILE = "data/tclient/languages/english.txt"
# TRANSLATION_FILE = "data/tclient/languages/japanese.txt"

# ==================== Output Options Configuration ====================

# Whether to save intermediate result files
SAVE_EXTRACTION_RESULT = True  # Save extracted strings
SAVE_COMPARISON_RESULT = True  # Save comparison results

# Output file names
EXTRACTION_OUTPUT_FILE = "extracted_tc_localize_strings.txt"
COMPARISON_OUTPUT_FILE = "translation_comparison_result.txt"

# ==================== Usage Instructions ====================

"""
To use these configurations, modify the configuration section in check_translations.py:

1. Copy your desired configuration to the script's configuration area
2. Adjust paths and options as needed
3. Run the script: python scripts/check_translations.py

Example modification:

# In the main() function of check_translations.py, find the configuration area, then:

SEARCH_PATHS = [
    os.path.join(WORKSPACE_ROOT, path) for path in SEARCH_SPECIFIC_FILES
]

# Or use directory search:

SEARCH_PATHS = [
    os.path.join(WORKSPACE_ROOT, path) for path in SEARCH_DIRECTORIES
]
"""

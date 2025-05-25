#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Translation Update Tool
======================

This script automatically updates translation files by:
1. Reading missing translations from comparison result file
2. Adding missing translation keys to the end of translation file
3. Moving obsolete translations to the bottom with obsolete marker
4. Maintaining proper translation file format

Usage:
    python update_translations.py [result_file] [translation_file]

    If no arguments provided, uses default paths:
    - result_file: translation_comparison_result.txt
    - translation_file: data/tclient/languages/simplified_chinese.txt

Examples:
    # Use default files
    python scripts/update_translations.py

    # Specify custom result file
    python scripts/update_translations.py my_comparison_result.txt

    # Specify both files
    python scripts/update_translations.py result.txt data/tclient/languages/other_language.txt

Author: GitHub Copilot
Date: May 25, 2025
"""

import os
import re
import sys
from typing import List, Set, Tuple


class TranslationUpdater:
    """Translation file updater that processes comparison results and updates translation files"""

    def __init__(self, workspace_root: str):
        """
        Initialize the translation updater

        Args:
            workspace_root: Workspace root directory
        """
        self.workspace_root = workspace_root

    def parse_comparison_result(self, result_file: str) -> Tuple[List[str], List[str]]:
        """
        Parse translation comparison result file to extract missing and obsolete translations

        Args:
            result_file: Path to the comparison result file

        Returns:
            Tuple of (missing_translations, obsolete_translations)
        """
        missing_translations = []
        obsolete_translations = []
        current_section = None

        try:
            with open(result_file, "r", encoding="utf-8") as f:
                for line in f:
                    line = line.strip()

                    if "Missing translations" in line:
                        current_section = "missing"
                        continue
                    elif "Potentially obsolete translations" in line:
                        current_section = "obsolete"
                        continue
                    elif line.startswith("--") or line.startswith("Total") or not line:
                        continue

                    # Extract translation string from numbered list
                    match = re.match(r'\s*\d+\.\s*"(.+)"', line)
                    if match:
                        translation_key = match.group(1)
                        if current_section == "missing":
                            missing_translations.append(translation_key)
                        elif current_section == "obsolete":
                            obsolete_translations.append(translation_key)

        except FileNotFoundError:
            print(f"Error: Result file '{result_file}' not found.")
            sys.exit(1)
        except Exception as e:
            print(f"Error reading result file: {e}")
            sys.exit(1)

        return missing_translations, obsolete_translations

    def parse_translation_file(self, translation_file: str) -> Tuple[List[str], List[str], Set[str]]:
        """
        Parse translation file to extract header, content, and existing keys

        Args:
            translation_file: Path to the translation file

        Returns:
            Tuple of (header_lines, content_lines, existing_keys)
        """
        header_lines = []
        content_lines = []
        existing_keys = set()
        in_header = True

        try:
            with open(translation_file, "r", encoding="utf-8") as f:
                lines = f.readlines()

                for i, line in enumerate(lines):
                    line_stripped = line.strip()

                    # Check if we're still in header section
                    if in_header and line_stripped == "##### translated strings #####":
                        header_lines.append(line)
                        in_header = False
                        continue

                    if in_header:
                        header_lines.append(line)
                    else:
                        content_lines.append(line)

                        # Extract translation key - look for lines followed by "== translation"
                        if not line_stripped.startswith("#") and line_stripped != "" and i < len(lines) - 1 and lines[i + 1].strip().startswith("=="):
                            existing_keys.add(line_stripped)

        except FileNotFoundError:
            print(f"Error: Translation file '{translation_file}' not found.")
            sys.exit(1)
        except Exception as e:
            print(f"Error reading translation file: {e}")
            sys.exit(1)

        return header_lines, content_lines, existing_keys

    def update_translation_file(self, translation_file: str, missing_translations: List[str], obsolete_translations: List[str]) -> None:
        """
        Update translation file by adding missing translations and marking obsolete ones

        Args:
            translation_file: Path to the translation file
            missing_translations: List of missing translation keys
            obsolete_translations: List of obsolete translation keys
        """
        header_lines, content_lines, existing_keys = self.parse_translation_file(translation_file)

        # Filter out translations that already exist or are obsolete
        new_missing = [t for t in missing_translations if t not in existing_keys and t not in obsolete_translations]

        # Separate current content from obsolete content
        current_content = []
        existing_obsolete_content = []
        in_obsolete_section = False
        obsolete_entries_to_move = []
        moved_obsolete_keys = set()

        i = 0
        while i < len(content_lines):
            line = content_lines[i]
            line_stripped = line.strip()

            if line_stripped == "##### obsolete strings #####":
                in_obsolete_section = True
                i += 1
                continue

            if not in_obsolete_section:
                # Check if this line is an obsolete translation key
                if line_stripped in obsolete_translations and i < len(content_lines) - 1 and content_lines[i + 1].strip().startswith("=="):
                    # Collect the key line and translation line
                    entry_lines = []
                    entry_lines.append(line)  # Key line
                    entry_lines.append(content_lines[i + 1])  # Translation line
                    moved_obsolete_keys.add(line_stripped)

                    # Add empty line if it exists
                    if i < len(content_lines) - 2 and content_lines[i + 2].strip() == "":
                        entry_lines.append(content_lines[i + 2])
                        i += 2  # Skip translation and empty line
                    else:
                        i += 1  # Skip translation line

                    obsolete_entries_to_move.extend(entry_lines)
                    i += 1
                    continue

                current_content.append(line)
            else:
                existing_obsolete_content.append(line)

            i += 1

        # Build the new file content
        new_content = []

        # Add header
        new_content.extend(header_lines)

        # Add current translations (excluding obsolete ones)
        new_content.extend(current_content)

        # Add missing translations
        if new_missing:
            new_content.append("\n")
            for translation_key in new_missing:
                new_content.append(f"{translation_key}\n")
                new_content.append(f"== {translation_key}\n")
                new_content.append("\n")

        # Add obsolete section if there are obsolete translations
        if existing_obsolete_content or obsolete_entries_to_move:
            new_content.append("##### obsolete strings #####\n")
            new_content.append("\n")

            # Add moved obsolete entries
            new_content.extend(obsolete_entries_to_move)
            if obsolete_entries_to_move and not obsolete_entries_to_move[-1].endswith("\n"):
                new_content.append("\n")

            # Add existing obsolete content
            new_content.extend(existing_obsolete_content)

        # Write the updated content back to file
        try:
            with open(translation_file, "w", encoding="utf-8") as f:
                f.writelines(new_content)

            print(f"Successfully updated translation file: {translation_file}")
            if new_missing:
                print(f"Added {len(new_missing)} missing translations:")
                for translation in new_missing:
                    print(f"  - {translation}")
            else:
                print("No new missing translations to add.")

            moved_obsolete = len(moved_obsolete_keys)
            if moved_obsolete > 0:
                print(f"Moved {moved_obsolete} obsolete translations to obsolete section.")
            else:
                print("No obsolete translations to move.")

        except Exception as e:
            print(f"Error writing to translation file: {e}")
            sys.exit(1)


def main():
    """Main function"""
    # Check for help option
    if len(sys.argv) > 1 and sys.argv[1] in ["-h", "--help", "help"]:
        print(__doc__)
        return

    # Get workspace root (parent directory of scripts)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    workspace_root = os.path.dirname(script_dir)

    # Parse command line arguments
    if len(sys.argv) >= 3:
        result_file = sys.argv[1]
        translation_file = sys.argv[2]
    elif len(sys.argv) == 2:
        result_file = sys.argv[1]
        translation_file = os.path.join(workspace_root, "data/tclient/languages/simplified_chinese.txt")
    else:
        result_file = os.path.join(workspace_root, "translation_comparison_result.txt")
        translation_file = os.path.join(workspace_root, "data/tclient/languages/simplified_chinese.txt")

    # Convert relative paths to absolute paths
    if not os.path.isabs(result_file):
        result_file = os.path.join(workspace_root, result_file)
    if not os.path.isabs(translation_file):
        translation_file = os.path.join(workspace_root, translation_file)

    print("Translation Update Tool")
    print("======================")
    print(f"Result file: {result_file}")
    print(f"Translation file: {translation_file}")
    print()

    # Create updater and process files
    updater = TranslationUpdater(workspace_root)

    # Parse comparison result
    print("Parsing comparison result file...")
    missing_translations, obsolete_translations = updater.parse_comparison_result(result_file)

    print(f"Found {len(missing_translations)} missing translations")
    print(f"Found {len(obsolete_translations)} obsolete translations")
    print()

    # Update translation file
    print("Updating translation file...")
    updater.update_translation_file(translation_file, missing_translations, obsolete_translations)
    print()
    print("Update completed successfully!")


if __name__ == "__main__":
    main()

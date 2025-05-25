#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Translation Checker Tool
========================

This script provides integrated functionality for checking translation completeness:
1. Extract TCLocalize strings from source code files
2. Compare with translation files to find missing translations
3. Configurable search paths and output options
4. Optional saving of intermediate result files

Author: GitHub Copilot
Date: May 25, 2025
"""

import os
import re
from typing import Dict, List, Optional, Set


class TranslationChecker:
    """Translation checker class that integrates string extraction and translation comparison functionality"""

    def __init__(self, workspace_root: str):
        """
        Initialize the translation checker

        Args:
            workspace_root: Workspace root directory
        """
        self.workspace_root = workspace_root
        self.regex = r'TCLocalize\s*\(\s*"((?:\\.|[^"\\])*)"(?:\s*,\s*"(?:\\.|[^"\\])*")?\s*\)'

    def extract_tc_localize_strings(self, search_paths: List[str], output_file: Optional[str] = None) -> Dict[str, List[str]]:
        """
        Extract TCLocalize("...") strings from specified C++ files or directories

        Args:
            search_paths: List of file or directory paths to search
            output_file: Optional output file path, saves extraction results if provided

        Returns:
            Dictionary with file paths as keys and list of extracted strings as values
        """
        all_extracted_strings = {}
        files_to_process = []

        # Collect all C++ files to process
        for path in search_paths:
            if os.path.isfile(path) and path.endswith(".cpp"):
                files_to_process.append(path)
            elif os.path.isdir(path):
                for root, _, files in os.walk(path):
                    for filename in files:
                        if filename.endswith(".cpp"):
                            files_to_process.append(os.path.join(root, filename))
            else:
                print(f"Warning: Path '{path}' is not a valid C++ file or directory, skipped.")

        if not files_to_process:
            print("Error: No .cpp files found for processing. Please check your input paths.")
            return {}

        # Process each file
        for filepath in files_to_process:
            extracted_from_file = []
            try:
                with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                    matches = re.findall(self.regex, content)
                    if matches:
                        extracted_from_file.extend(matches)
            except Exception as e:
                print(f"Error reading file {filepath}: {e}")

            if extracted_from_file:
                try:
                    display_path = os.path.relpath(filepath, os.getcwd())
                except ValueError:
                    display_path = os.path.abspath(filepath)
                all_extracted_strings[display_path] = extracted_from_file

        # Save extraction results if output file is specified
        if output_file:
            self._save_extracted_strings(all_extracted_strings, output_file)

        return all_extracted_strings

    def _save_extracted_strings(self, extracted_strings: Dict[str, List[str]], output_file: str) -> None:
        """Save extracted strings to file"""
        try:
            with open(output_file, "w", encoding="utf-8") as outfile:
                if not extracted_strings:
                    outfile.write("No TCLocalize strings found in the specified .cpp files.\n")
                    print("No TCLocalize strings found in the specified .cpp files.")
                    return

                total_overall_matches = 0
                all_unique_strings = set()
                for filepath, strings in extracted_strings.items():
                    count = len(strings)
                    total_overall_matches += count
                    outfile.write(f"File: {filepath} ({count} matches)\n")
                    for s in strings:
                        outfile.write(f'"{s}"\n')
                        all_unique_strings.add(s)
                    outfile.write("-" * 30 + "\n\n")

                unique_count = len(all_unique_strings)
                duplicate_count = total_overall_matches - unique_count

                outfile.write(f"\nTotal extracted {total_overall_matches} TCLocalize strings.\n")
                outfile.write(f"Unique strings: {unique_count}, Duplicate strings: {duplicate_count}.\n")
                print(f"Successfully saved extracted strings to {output_file}")
                print(f"Total extracted {total_overall_matches} TCLocalize strings (unique: {unique_count}, duplicates: {duplicate_count})")

        except Exception as e:
            print(f"Error writing output file {output_file}: {e}")

    def extract_strings_from_extracted_file(self, filepath: str) -> Optional[Set[str]]:
        """
        Parse strings from extraction result file

        Args:
            filepath: Path to extraction result file

        Returns:
            Set of strings, or None if file doesn't exist
        """
        strings = set()
        regex = r'"([^"]*)"'

        try:
            with open(filepath, "r", encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith("//") or "File:" in line or "------------------------------" in line:
                        continue
                    match = re.match(regex, line)
                    if match:
                        extracted_string = match.group(1).replace('\\\\"', '"')
                        strings.add(extracted_string)
        except FileNotFoundError:
            print(f"Error: Source file not found {filepath}")
            return None

        return strings

    def extract_strings_from_memory(self, extracted_strings: Dict[str, List[str]]) -> Set[str]:
        """
        Get all unique strings from in-memory extraction results

        Args:
            extracted_strings: Dictionary of extracted strings

        Returns:
            Set of all unique strings
        """
        all_strings = set()
        for strings in extracted_strings.values():
            for s in strings:
                # Handle escaped quotes
                unescaped_string = s.replace('\\\\"', '"')
                all_strings.add(unescaped_string)
        return all_strings

    def extract_translated_originals(self, filepath: str) -> Optional[Set[str]]:
        """
        Extract original strings from translation file

        Args:
            filepath: Path to translation file

        Returns:
            Set of original strings, or None if file doesn't exist
        """
        originals = set()
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                lines = f.readlines()
                i = 0
                while i < len(lines):
                    line = lines[i].strip()
                    if line and not line.startswith(("//", "#")):
                        if i + 1 < len(lines) and lines[i + 1].strip().startswith("=="):
                            originals.add(line)
                            i += 1  # Skip translation line
                    i += 1
        except FileNotFoundError:
            print(f"Error: Translation file not found {filepath}")
            return None

        return originals

    def compare_translations(self, source_strings: Set[str], translated_strings: Set[str], output_file: Optional[str] = None) -> Dict[str, Set[str]]:
        """
        Compare source strings and translated strings to find differences

        Args:
            source_strings: Set of strings from source code
            translated_strings: Set of strings from translation file
            output_file: Optional output file path, saves comparison results if provided

        Returns:
            Dictionary containing 'missing' and 'obsolete' keys
        """
        missing_translations = source_strings - translated_strings
        obsolete_translations = translated_strings - source_strings

        result = {"missing": missing_translations, "obsolete": obsolete_translations}

        # Print results to console
        print(f"Total strings in source code: {len(source_strings)}")
        print(f"Total original strings in translation file: {len(translated_strings)}")

        if missing_translations:
            print("\n" + "=" * 50)
            print("Missing translations (exist in source but not in translation file):")
            print("=" * 50)
            for item_num, string in enumerate(sorted(list(missing_translations)), 1):
                print(f'{item_num:3d}. "{string}"')
        else:
            print("\n✅ All strings from source code have corresponding entries in translation file.")

        if obsolete_translations:
            print("\n" + "=" * 50)
            print("⚠️  Potentially obsolete translations (exist in translation file but not in source):")
            print("=" * 50)
            for item_num, string in enumerate(sorted(list(obsolete_translations)), 1):
                print(f'{item_num:3d}. "{string}"')

        # Save comparison results if output file is specified
        if output_file:
            self._save_comparison_result(result, source_strings, translated_strings, output_file)

        return result

    def _save_comparison_result(self, result: Dict[str, Set[str]], source_strings: Set[str], translated_strings: Set[str], output_file: str) -> None:
        """Save comparison results to file"""
        try:
            with open(output_file, "w", encoding="utf-8") as outfile:
                outfile.write("Translation Comparison Results\n")
                outfile.write("=" * 50 + "\n\n")
                outfile.write(f"Total strings in source code: {len(source_strings)}\n")
                outfile.write(f"Total original strings in translation file: {len(translated_strings)}\n\n")

                if result["missing"]:
                    outfile.write("Missing translations (exist in source but not in translation file):\n")
                    outfile.write("-" * 50 + "\n")
                    for item_num, string in enumerate(sorted(list(result["missing"])), 1):
                        outfile.write(f'{item_num:3d}. "{string}"\n')
                    outfile.write(f"\nTotal {len(result['missing'])} missing translations.\n\n")
                else:
                    outfile.write("✅ All strings from source code have corresponding entries in translation file.\n\n")

                if result["obsolete"]:
                    outfile.write("Potentially obsolete translations (exist in translation file but not in source):\n")
                    outfile.write("-" * 50 + "\n")
                    for item_num, string in enumerate(sorted(list(result["obsolete"])), 1):
                        outfile.write(f'{item_num:3d}. "{string}"\n')
                    outfile.write(f"\nTotal {len(result['obsolete'])} potentially obsolete translations.\n")

            print(f"Comparison results saved to {output_file}")
        except Exception as e:
            print(f"Error writing comparison results file {output_file}: {e}")


def main():
    """Main function - configure and execute translation check"""

    # ==================== Configuration Section ====================
    # Modify the following configuration as needed

    # Workspace root directory (parent of scripts directory)
    WORKSPACE_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    # List of source code files or directories to search
    # Can be specific file paths or directory paths (will recursively search for .cpp files)
    SEARCH_PATHS = [
        os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "tclient", "bg_draw.cpp"),
        os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "tclient", "menus_tclient.cpp"),
        os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "chat.cpp"),
        os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "menus_settings.cpp"),
        os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "tclient", "tclient.cpp"),
        # You can also add entire directories, for example:
        # os.path.join(WORKSPACE_ROOT, "src", "game", "client", "components", "tclient"),
    ]

    # Translation file path
    TRANSLATION_FILE = os.path.join(WORKSPACE_ROOT, "data", "tclient", "languages", "simplified_chinese.txt")

    # Whether to save intermediate result files
    SAVE_EXTRACTION_RESULT = True  # Whether to save extracted strings to file
    SAVE_COMPARISON_RESULT = True  # Whether to save comparison results to file

    # Output file paths (relative to workspace root)
    EXTRACTION_OUTPUT_FILE = os.path.join(WORKSPACE_ROOT, "extracted_tc_localize_strings.txt")
    COMPARISON_OUTPUT_FILE = os.path.join(WORKSPACE_ROOT, "translation_comparison_result.txt")

    # ==================== Execution Section ====================

    print("=" * 60)
    print("Translation Checker Tool")
    print("=" * 60)

    # Create translation checker instance
    checker = TranslationChecker(WORKSPACE_ROOT)

    # Step 1: Extract TCLocalize strings from source code
    print("\n📋 Step 1: Extract TCLocalize strings from source code...")
    print(f"Search paths: {len(SEARCH_PATHS)} items")
    for path in SEARCH_PATHS:
        print(f"  - {path}")

    extraction_output = EXTRACTION_OUTPUT_FILE if SAVE_EXTRACTION_RESULT else None
    extracted_strings = checker.extract_tc_localize_strings(SEARCH_PATHS, extraction_output)

    if not extracted_strings:
        print("❌ No TCLocalize strings found, exiting.")
        return

    # Get all unique strings from extraction results
    source_strings = checker.extract_strings_from_memory(extracted_strings)

    # Step 2: Extract original strings from translation file
    print("\n📖 Step 2: Extract original strings from translation file...")
    print(f"Translation file: {TRANSLATION_FILE}")

    translated_strings = checker.extract_translated_originals(TRANSLATION_FILE)
    if translated_strings is None:
        print("❌ Unable to read translation file, exiting.")
        return

    # Step 3: Compare and generate report
    print("\n🔍 Step 3: Compare source strings and translation strings...")

    comparison_output = COMPARISON_OUTPUT_FILE if SAVE_COMPARISON_RESULT else None
    comparison_result = checker.compare_translations(source_strings, translated_strings, comparison_output)

    # Calculate total extractions (including duplicates)
    total_extractions = sum(len(strings) for strings in extracted_strings.values())

    # Summary
    print("\n" + "=" * 60)
    print("📊 Summary")
    print("=" * 60)
    print(f"✅ Processed {len(extracted_strings)} source files")
    print(f"✅ Extracted {total_extractions} TCLocalize strings (including duplicates)")
    print(f"✅ Found {len(source_strings)} unique strings after deduplication")
    print(f"✅ Translation file contains {len(translated_strings)} original strings")
    print(f"🔴 Missing translations: {len(comparison_result['missing'])} items")
    print(f"🟡 Potentially obsolete: {len(comparison_result['obsolete'])} items")

    if SAVE_EXTRACTION_RESULT:
        print(f"💾 Extraction results saved: {os.path.relpath(EXTRACTION_OUTPUT_FILE, WORKSPACE_ROOT)}")
    if SAVE_COMPARISON_RESULT:
        print(f"💾 Comparison results saved: {os.path.relpath(COMPARISON_OUTPUT_FILE, WORKSPACE_ROOT)}")

    print("\n🎉 Check completed!")


if __name__ == "__main__":
    main()

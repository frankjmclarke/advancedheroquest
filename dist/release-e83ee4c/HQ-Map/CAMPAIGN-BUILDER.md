# Campaign Builder

Choose **Options > Campaign Builder...** to create a new, single-level campaign.
This is part of HQ-Map itself; Python is not required.

1. Choose **Start from**. This fills in the component selections and description.
2. Enter a new campaign title and folder name. Folder names use English letters
   and hyphens, start and end with a letter, and contain at most 40 characters.
3. Choose rooms/layout, monsters, hazards, furnishings, traps and treasure.
   **View** displays a component's table text, including encounter quantities,
   rewards and dice ranges. The path in brackets identifies the exact file.
4. Edit the description and choose whether to include room furnishings.
5. Choose **Check Campaign**. The review lists the copied components and inherited
   room text, including named objectives or rewards that a monster swap does not
   change. Edit those in the new campaign's room files after creation if needed.
6. Choose **Create Campaign**. Validation runs again before anything is published.
7. Use **Options > Load Tables** to select the new campaign and generate its map.

For example, start with `faces1_1.tab`, select `dark\orc1.tab` for monsters,
and save as `goblinfort`. This keeps the Changing Faces layout but uses the
greenskin encounters. Review the inherited quest reward, "Alaric's Key".

The builder scans the current tables folder at runtime, including subfolders.
Classification uses table definitions rather than filenames. **Refresh** finds
new files without restarting and preserves edits and surviving selections.
Files supplying multiple categories are marked **(shared)**; selecting one sets
the corresponding dropdowns together. Incompatible subsequent choices are caught
by validation. Starting templates with inline tables or conditional include wiring
are reported as unsupported rather than silently rewritten.

The new entry and directory are created beside existing campaigns:

```
tables/goblinfort.tab
tables/goblinfort/description.txt
tables/goblinfort/faces/room1_1.tab
tables/goblinfort/dark/orc1.tab
tables/goblinfort/standard/...
```

Source-relative subdirectories are retained to avoid filename collisions. Includes
are rewritten to use the new copies. Explicit includes and required supporting
tables are copied too. The dependency resolver first looks at the component's
original campaign includes, then its directory, then other available providers.
Missing encounters such as Sonneklinge's `Wight()` are extracted into small files
under the new campaign's `_support` directory. Only the needed named tables and
their dependencies are added, so your selected hazard and monster sets stay intact.
Conditional supporting files retain their original directives rather than having
individual definitions extracted. The complete result is always validated.
If the original source cannot be determined unambiguously, or a definition is
missing altogether, the builder reports it instead of inventing game rules.
Original files are never edited, and existing campaigns are never overwritten.

Validation uses HQ-Map's actual table parser in a separate, hidden process. It
checks dice coverage, syntax, duplicate definitions, references, recursion and
permitted map features with the selected conditional definitions. Your current
map and loaded tables remain untouched. Validation does not assess game balance.
Temporary files are removed after success or failure; if publication fails after
the directory is moved, the builder attempts to roll it back and reports its
location if that cannot be done.

Developer check (in an x86 MSVC developer shell, after building):

```
python tests/campaign-builder-check.py
```

This tests runtime discovery, the native dialog, every valid shipped campaign,
cross-campaign combinations, copying a generated campaign, furnishing options,
dependency copying, parser failures, path/collision protection, and independent
loading after source files are removed. It also renders the actual dialog to
`obj/campaign-builder-preview.bmp` for visual review.

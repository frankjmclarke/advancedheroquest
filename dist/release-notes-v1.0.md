# HQ-Map 1.0 Release Notes

**A dungeon generator for Advanced HeroQuest.**

Press a couple of keys and it rolls you a whole dungeon — rooms, corridors, doors, traps, wandering monsters and treasure — and draws the map.

Written by Jürgen Albuschies in 1999. This version runs on today's Windows, in English, in colour, and fills the screen.

## Install it

**1.** Download `HQ-Map-1.0-setup.exe` at the bottom of this page.

**2.** Run it. Windows will show a blue box saying "Windows protected your PC". Click **More info**, then **Run anyway**.

> That warning appears because the file isn't code-signed — a certificate costs a few hundred pounds a year. It does not need an administrator password and it installs only for you.

**3.** That's it. It puts HQ-Map on your Desktop and in your Start Menu, and opens it **with a dungeon already on screen**.

## Using it

There is nothing to set up. It starts with a map.

- `Ctrl` + `N` — roll a different dungeon
- `Ctrl` + `T` — pick a different quest
- `F1` — the full list of what it can do

## A few more things

Make the map bigger or smaller: `Ctrl` `+` and `Ctrl` `-`

Fit the whole map on screen: `Ctrl` `9`

Save the map as a picture: `Ctrl` `A`

Pick a different quest: `Ctrl` `T`

Quests included: Sonneklinge, Ravenloft, Sentinel V, Terror, Dark, Ritual, Priests, Oath, Amulett (and an easier version of it), Eyes, Faces and Rivers.

## Sentinel V

A campaign added since the first build of this release, and the only one here that isn't Warhammer.

Sentinel V is an Advanced HeroQuest adventure played with the Milton Bradley **Space Crusade** miniatures and its two expansions, **Eldar Attack** and **Mission Dreadnought**. You are Space Marines, put down on an Imperial watch-station that stopped transmitting six weeks ago, and you go through it a deck at a time:

1. **Docking Spires** — Gretchin and Orks
2. **Cargo Holds** — Orks, Nobz and Chaos Androids
3. **Reactor Sumps** — a Genestealer brood in the coolant
4. **Command Spire** — Chaos Space Marines and Dreadnoughts
5. **The Sentinel Core** — Eldar, and nothing else

Press `Ctrl` `T` and pick a deck, then play them in order. Every model named is one that actually shipped in a box, rewards are in Points rather than gold crowns, and the last deck ends at a webway portal that is both the objective and the only way off Sentinel V.

**Printable sheets come with it**, in `tables\sentinel\` beside the program:

- `space-marine-sheets.pdf` — a character sheet for each kind of Space Marine, Commander down to Fusion Gun, laid out like the Advanced HeroQuest class cards
- `monster-sheets.pdf` — a reference card for all eighteen enemies, laid out like the Advanced HeroQuest monster sheets. Page 3 is the Eldar, which is the whole bestiary for the last deck

Both have the hand-to-hand and ranged hit rolls already worked out, so there is no table to look up mid-game. The briefing and the full rosters are in `tables\sentinel\README.md`.

## Uninstalling

Settings → Apps → HQ-Map → Uninstall. Any maps you saved are kept.

## If you'd rather not install anything

Download `HQ-Map-1.0-win32.zip` instead. **Unpack the whole folder first**, then run `hq_map.exe` from inside it.

> Don't run `hq_map.exe` straight out of the zip window. Windows copies just that one file to a temporary folder, the program can't find its quest tables, and it fails with "Reference tables not available!".

## What's different from the 1999 original

- **In English.** The program was entirely in German.
- **Fills the screen.** It used to be a fixed little 768×537 window.
- **Colour.** Blue water, red fire, gold treasure, green lairs, brown stairs.
- **Zoom.** A room square used to be 8 pixels across. Now you can zoom, or fit the whole map to the window.

Saved maps are still black and white; the colour is on screen only.

## Credits

Original work © Jürgen Albuschies, 1999. This is a preservation mirror: no licence has been granted by the copyright holder and all rights remain reserved. If you are the author or hold the rights, open an issue and any request will be honoured.

_Advanced HeroQuest_ and _Warhammer_ are trademarks of Games Workshop. This project is unaffiliated with and unendorsed by Games Workshop.

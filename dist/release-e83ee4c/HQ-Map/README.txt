HQ-Map - Advanced HeroQuest Map Generator
=========================================

A random dungeon map generator for Games Workshop's Advanced HeroQuest.

Original work (c) Juergen Albuschies, 1999.
See NOTICE.txt for provenance and licensing. No licence has been granted
by the copyright holder; all rights remain reserved.


INSTALLING
----------

There is nothing to install. Unpack the folder anywhere you like - your
Desktop, Documents, a USB stick - and run hq_map.exe.

Everything it needs is in this folder:

    hq_map.exe      the program
    ahq_map.ini     your settings, written back here when you exit
    tables\         the quest tables it rolls maps from
    maps\           where saved maps go by default

Because the settings file lives next to the program, avoid unpacking into
C:\Program Files - Windows will not let a normal user write there, and your
settings would not be saved. Anywhere in your user folder is fine.

No runtime or redistributable is required. The program uses only Windows
system libraries.


USING IT
--------

    Map  >  Generate Map...     Ctrl+K      roll a new dungeon
    Map  >  Next Map            Ctrl+N      roll another
    Map  >  Save Map...         Ctrl+A      write it out
    Options > Load Tables...    Ctrl+T      pick a different quest

    View >  Zoom In             Ctrl +
            Zoom Out            Ctrl -
            Actual Size         Ctrl 0
            Fit to Window       Ctrl 9

Quest tables live in tables\. Load a different one with Ctrl+T to generate
maps for another quest: sonne, dark, terror, faces, ritual, priests, oath,
amulett, eyes and rivers are all included.

Saved maps are monochrome BMP, PCX or IMG. The colour is on screen only.


IF SOMETHING GOES WRONG
-----------------------

"Reference tables not available!" on startup means the program cannot find
tables\. Keep hq_map.exe and tables\ in the same folder, or use
Options > Load Tables... to point at the tables yourself.

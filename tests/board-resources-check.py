"""Verify Windows can load every embedded board BMP from the built EXE.

Run after build-msvc.bat: python tests/board-resources-check.py.
Loads the executable as data only; does not launch the application.
"""
import ctypes as C
from pathlib import Path
import re
import struct

root = Path(__file__).resolve().parents[1]
kernel = C.WinDLL('kernel32', use_last_error=True)
user = C.WinDLL('user32', use_last_error=True)
gdi = C.WinDLL('gdi32', use_last_error=True)
kernel.LoadLibraryExW.argtypes = [C.c_wchar_p, C.c_void_p, C.c_ulong]
kernel.LoadLibraryExW.restype = C.c_void_p
kernel.FreeLibrary.argtypes = [C.c_void_p]
user.LoadBitmapW.argtypes = [C.c_void_p, C.c_void_p]
user.LoadBitmapW.restype = C.c_void_p
gdi.GetObjectW.argtypes = [C.c_void_p, C.c_int, C.c_void_p]
gdi.DeleteObject.argtypes = [C.c_void_p]

class Bitmap(C.Structure):
    _fields_ = [('type', C.c_long), ('width', C.c_long), ('height', C.c_long),
                ('stride', C.c_long), ('planes', C.c_ushort), ('bits', C.c_ushort),
                ('data', C.c_void_p)]

entries = []
for name in ('grafic.rc', 'board-features.rc'):
    entries += re.findall(r'(?m)^(\d+) BITMAP "(board/[^"\n]+)"',
                          (root / 'rsh' / name).read_text())
assert len(entries) == 185
module = kernel.LoadLibraryExW(str(root / 'bin/hq_map.exe'), None, 2)
assert module, C.get_last_error()
try:
    for resource, filename in entries:
        expected = struct.unpack_from('<ii', (root / 'rsh' / filename).read_bytes(), 18)
        bitmap = user.LoadBitmapW(module, int(resource))
        assert bitmap, (resource, filename, C.get_last_error())
        try:
            info = Bitmap()
            assert gdi.GetObjectW(bitmap, C.sizeof(info), C.byref(info))
            assert (info.width, info.height) == expected, filename
        finally:
            gdi.DeleteObject(bitmap)
finally:
    kernel.FreeLibrary(module)
print(f'PASS: Windows loaded all {len(entries)} embedded board resources at their expected sizes.')

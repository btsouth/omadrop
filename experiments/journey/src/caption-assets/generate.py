#!/usr/bin/env python3
"""Generate fixed caption paths using Python's standard library and FreeType.

No Qt, fontTools, font installation, rasterization, or display is required.
The default input is the existing system Noto Sans Medium 2.015 TTF.
"""

import argparse
import ctypes as C
from ctypes.util import find_library
import hashlib
from pathlib import Path
import textwrap


class Vector(C.Structure):
    _fields_ = [("x", C.c_long), ("y", C.c_long)]


class Generic(C.Structure):
    _fields_ = [("data", C.c_void_p), ("finalizer", C.c_void_p)]


class BBox(C.Structure):
    _fields_ = [(name, C.c_long) for name in ("xmin", "ymin", "xmax", "ymax")]


class Face(C.Structure):
    # Public FT_FaceRec prefix through glyph; later members are not accessed.
    _fields_ = (
        [(name, C.c_long) for name in
         ("num_faces", "face_index", "face_flags", "style_flags", "num_glyphs")]
        + [("family_name", C.c_char_p), ("style_name", C.c_char_p),
           ("num_fixed_sizes", C.c_int), ("available_sizes", C.c_void_p),
           ("num_charmaps", C.c_int), ("charmaps", C.c_void_p),
           ("generic", Generic), ("bbox", BBox), ("units_per_em", C.c_ushort)]
        + [(name, C.c_short) for name in
           ("ascender", "descender", "height", "max_advance_width",
            "max_advance_height", "underline_position", "underline_thickness")]
        + [("glyph", C.c_void_p)]
    )


class Outline(C.Structure):
    _fields_ = [("n_contours", C.c_ushort), ("n_points", C.c_ushort),
               ("points", C.POINTER(Vector)), ("tags", C.c_void_p),
               ("contours", C.c_void_p), ("flags", C.c_int)]


class GlyphRoot(C.Structure):
    _fields_ = [("library", C.c_void_p), ("clazz", C.c_void_p),
               ("format", C.c_uint), ("advance", Vector)]


class OutlineGlyph(C.Structure):
    _fields_ = [("root", GlyphRoot), ("outline", Outline)]


PointPointer = C.POINTER(Vector)
Move = C.CFUNCTYPE(C.c_int, PointPointer, C.c_void_p)
Line = Move
Conic = C.CFUNCTYPE(C.c_int, PointPointer, PointPointer, C.c_void_p)
Cubic = C.CFUNCTYPE(C.c_int, PointPointer, PointPointer, PointPointer, C.c_void_p)


class OutlineFunctions(C.Structure):
    _fields_ = [("move_to", Move), ("line_to", Line),
               ("conic_to", Conic), ("cubic_to", Cubic),
               ("shift", C.c_int), ("delta", C.c_long)]


def checked(result):
    if result:
        raise RuntimeError(f"FreeType error {result}")


def freetype():
    library = find_library("freetype")
    if not library:
        raise RuntimeError("Generation requires the existing FreeType shared library")
    ft = C.CDLL(library)
    signatures = {
        "FT_Init_FreeType": [C.POINTER(C.c_void_p)],
        "FT_New_Face": [C.c_void_p, C.c_char_p, C.c_long, C.POINTER(C.POINTER(Face))],
        "FT_Select_Charmap": [C.POINTER(Face), C.c_uint],
        "FT_Get_Char_Index": [C.POINTER(Face), C.c_ulong],
        "FT_Load_Glyph": [C.POINTER(Face), C.c_uint, C.c_int32],
        "FT_Get_Glyph": [C.c_void_p, C.POINTER(C.POINTER(OutlineGlyph))],
        "FT_Get_Advance": [C.POINTER(Face), C.c_uint, C.c_int32, C.POINTER(C.c_long)],
        "FT_Get_Kerning": [C.POINTER(Face), C.c_uint, C.c_uint, C.c_uint, PointPointer],
        "FT_Outline_Decompose": [C.POINTER(Outline), C.POINTER(OutlineFunctions), C.c_void_p],
        "FT_Done_Glyph": [C.POINTER(OutlineGlyph)],
        "FT_Done_Face": [C.POINTER(Face)],
        "FT_Done_FreeType": [C.c_void_p],
    }
    for name, arguments in signatures.items():
        function = getattr(ft, name)
        function.argtypes = arguments
        function.restype = None if name == "FT_Done_Glyph" else C.c_int
    ft.FT_Get_Char_Index.restype = C.c_uint
    return ft


def text_outline(ft, face, text, size, tracking):
    scale = size / face.contents.units_per_em
    commands = []
    pen = 0.0
    previous = 0
    open_contour = False

    def command(code, *points):
        coordinates = []
        for point in points:
            coordinates.extend((pen + point.contents.x * scale, -point.contents.y * scale))
        commands.append((code, coordinates))
        return 0

    def move(point, _):
        nonlocal open_contour
        if open_contour:
            commands.append(("Z", []))
        open_contour = True
        return command("M", point)

    functions = OutlineFunctions(
        Move(move), Line(lambda point, _: command("L", point)),
        Conic(lambda control, target, _: command("Q", control, target)),
        Cubic(lambda first, second, target, _: command("C", first, second, target)),
        0, 0,
    )
    for character in text:
        index = ft.FT_Get_Char_Index(face, ord(character))
        if not index:
            raise ValueError(f"Font is missing {character!r}")
        if previous:
            kerning = Vector()
            checked(ft.FT_Get_Kerning(face, previous, index, 2, C.byref(kerning)))
            pen += kerning.x * scale  # FT_KERNING_UNSCALED
        checked(ft.FT_Load_Glyph(face, index, 1))  # FT_LOAD_NO_SCALE (also no hinting)
        glyph = C.POINTER(OutlineGlyph)()
        checked(ft.FT_Get_Glyph(face.contents.glyph, C.byref(glyph)))
        try:
            if glyph.contents.root.format != 0x6F75746C:  # FT_GLYPH_FORMAT_OUTLINE
                raise ValueError("Only outline fonts are supported")
            checked(ft.FT_Outline_Decompose(C.byref(glyph.contents.outline), C.byref(functions), None))
            if open_contour:
                commands.append(("Z", []))
                open_contour = False
        finally:
            ft.FT_Done_Glyph(glyph)
        advance = C.c_long()
        checked(ft.FT_Get_Advance(face, index, 1, C.byref(advance)))
        pen += advance.value * scale + tracking
        previous = index

    # Optical left alignment for both the label and the two titles.
    left = min(values[i] for _, values in commands for i in range(0, len(values), 2))
    tokens = []
    for code, values in commands:
        tokens.append(code)
        for index, value in enumerate(values):
            value -= left if index % 2 == 0 else 0
            formatted = f"{value:.3f}".rstrip("0").rstrip(".")
            tokens.append("0" if formatted == "-0" else formatted)
    return " ".join(tokens), commands


def generate(font):
    ft = freetype()
    library = C.c_void_p()
    checked(ft.FT_Init_FreeType(C.byref(library)))
    face = C.POINTER(Face)()
    result = [
        "// Generated by generate.py; do not edit. Fixed caption artwork only.",
        "// Outlines derived from Noto Sans Medium; see README.md and OFL.txt.",
        f"// Source TTF SHA-256: {hashlib.sha256(font.read_bytes()).hexdigest()}",
    ]
    try:
        checked(ft.FT_New_Face(library, str(font).encode(), 0, C.byref(face)))
        checked(ft.FT_Select_Charmap(face, 0x756E6963))  # FT_ENCODING_UNICODE
        if face.contents.family_name != b"Noto Sans" or face.contents.style_name != b"Medium":
            raise ValueError("Use Noto Sans Medium so the included attribution stays accurate")
        for name, text, size, tracking in (
            ("themeLabelOutline", "OMARCHY THEME", 11, 2.0),
            ("osakaJadeOutline", "Osaka Jade", 46, 0),
            ("kanagawaOutline", "Kanagawa", 46, 0),
        ):
            data, commands = text_outline(ft, face, text, size, tracking)
            result.append(f"\nconstexpr char {name}[] =")
            lines = textwrap.wrap(data, width=110, break_long_words=False, break_on_hyphens=False)
            result.extend(f'    "{line}{" " if index < len(lines) - 1 else ""}"'
                          for index, line in enumerate(lines))
            result[-1] += ";"
            xs = [values[i] for _, values in commands for i in range(0, len(values), 2)]
            ys = [values[i] for _, values in commands for i in range(1, len(values), 2)]
            print(f"{text}: {len(data)} path bytes, {len(commands)} commands, "
                  f"control bounds {max(xs)-min(xs):.3f} x {max(ys)-min(ys):.3f} px")
    finally:
        if face:
            ft.FT_Done_Face(face)
        ft.FT_Done_FreeType(library)
    return "\n".join(result) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", type=Path, default=Path("/usr/share/fonts/noto/NotoSans-Medium.ttf"))
    parser.add_argument("--output", type=Path, default=Path(__file__).with_name("outlines.inc"))
    parser.add_argument("--check", action="store_true", help="Verify the checked-in asset without writing")
    arguments = parser.parse_args()
    generated = generate(arguments.font)
    if arguments.check:
        if arguments.output.read_text() != generated:
            raise SystemExit("Outline asset differs; regenerate with the recorded source TTF")
        print("Outline asset matches")
    else:
        arguments.output.write_text(generated)


if __name__ == "__main__":
    main()

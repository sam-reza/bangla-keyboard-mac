# Faithful Python mirror of engine/Engine.swift, keyed by Windows scan code.
# Used to empirically validate the algorithm against SPEC.md §7 before/while
# writing the C++ port (no C++ compiler on this box yet). Python iterates str by
# Unicode code point, which matches the Swift engine's unicodeScalars semantics
# (Bengali is all BMP), so this is a behaviour-faithful reference.
import unicodedata

HASANTA = "্"
PREBASE = {"ি", "ে", "ৈ"}                       # ি ে ৈ
POSTBASE = {"া", "ী", "ু", "ূ", "ৃ", "ৗ"}       # া ী ু ূ ৃ ৗ
TRAIL = {"ং", "ঃ", "ঁ"}                          # ং ঃ ঁ
INDEPENDENTS = {"অ","আ","ই","ঈ","উ","ঊ","ঋ","এ","ঐ","ও","ঔ"}
STANDALONE = {"ৎ"}                                         # ৎ
CLUSTERMODS = {"র্", "্র", "্য"}  # র্  ্র  ্য
INDEP = {
    "া":"আ","ি":"ই","ী":"ঈ","ু":"উ","ূ":"ঊ",
    "ৃ":"ঋ","ে":"এ","ৈ":"ঐ","ো":"ও","ৌ":"ঔ",
}

# scan codes (Windows Set-1 make codes) for US-QWERTY positions
SC = {
    "`":0x29,"1":0x02,"2":0x03,"3":0x04,"4":0x05,"5":0x06,"6":0x07,"7":0x08,"8":0x09,"9":0x0A,"0":0x0B,"-":0x0C,"=":0x0D,
    "q":0x10,"w":0x11,"e":0x12,"r":0x13,"t":0x14,"y":0x15,"u":0x16,"i":0x17,"o":0x18,"p":0x19,"[":0x1A,"]":0x1B,"\\":0x2B,
    "a":0x1E,"s":0x1F,"d":0x20,"f":0x21,"g":0x22,"h":0x23,"j":0x24,"k":0x25,"l":0x26,";":0x27,"'":0x28,
    "z":0x2C,"x":0x2D,"c":0x2E,"v":0x2F,"b":0x30,"n":0x31,"m":0x32,",":0x33,".":0x34,"/":0x35,
}
BACKSPACE = 0x0E

# (unshifted, shift) Bangla unit per QWERTY key — translated from Engine.swift map / SPEC §6
LAYOUT = {
    "`":("—","‚"), "q":("ঙ","ং"), "w":("য","য়"), "e":("ড","ঢ"),
    "r":("প","ফ"), "t":("ট","ঠ"), "y":("চ","ছ"), "u":("জ","ঝ"),
    "i":("হ","ঞ"), "o":("গ","ঘ"), "p":("ড়","ঢ়"), "[":("[","{"), "]":("]","}"),
    "a":("ৃ","র্"), "s":("ু","ূ"), "d":("ি","ী"), "f":("া","অ"),
    "g":("্","।"), "h":("ব","ভ"), "j":("ক","খ"), "k":("ত","থ"),
    "l":("দ","ধ"), ";":(";",":"), "'":("'","\""), "\\":("ৎ","ঃ"),
    "z":("্র","্য"), "x":("ও","ৗ"), "c":("ে","ৈ"), "v":("র","ল"),
    "b":("ন","ণ"), "n":("স","ষ"), "m":("ম","শ"), ",":(",","<"), ".":(".",">"), "/":("/","?"),
    "1":("১","!"), "2":("২","°"), "3":("৩","#"), "4":("৪","৳"), "5":("৫","%"),
    "6":("৬","^"), "7":("৭","ঁ"), "8":("৮","*"), "9":("৯","("), "0":("০",")"),
    "-":("-","_"), "=":("=","+"),
}
# scan-code -> (unshifted, shift)
SCANMAP = {SC[k]: v for k, v in LAYOUT.items()}


class Engine:
    def __init__(self):
        self.cons = ""; self.vowel = ""; self.trail = ""

    def render(self):
        return self.cons + self.vowel + self.trail

    def render_final(self):
        if not self.cons and self.vowel:
            return INDEP.get(self.vowel, self.vowel) + self.trail
        return self.render()

    def ends_with_hasanta(self, s):
        return len(s) > 0 and ord(s[-1]) == 0x09CD

    def is_empty(self):
        return not self.cons and not self.vowel and not self.trail

    def reset(self):
        self.cons = ""; self.vowel = ""; self.trail = ""

    def flush(self):
        s = self.render_final(); self.reset(); return s

    def is_consonant(self, u):
        if len(u) != 1: return False
        v = ord(u)
        return (0x0995 <= v <= 0x09B9) or v in (0x09DC, 0x09DD, 0x09DF)

    def combine_vowel(self, existing, add):
        if existing == "ে" and add == "া": return "ো"   # ে + া -> ো
        if existing == "ে" and add == "ৗ": return "ৌ"   # ে + ৗ -> ৌ
        return existing + add

    def process(self, scan, shift):
        pair = SCANMAP.get(scan)
        if pair is None:
            if scan == BACKSPACE and not self.is_empty():
                self.pop_last()
                return ("commit", "") if self.is_empty() else ("compose", self.render())
            return ("ignore", "")
        unit = pair[1] if shift else pair[0]

        if unit == HASANTA:
            self.cons += unit; return ("compose", self.render())
        if unit in CLUSTERMODS:
            # Reph (র্) is typed AFTER its consonant (Bijoy/Windows habit) but in
            # Unicode it leads the cluster, so reorder it to the front of a closed
            # consonant: ক then র্ -> র্ক (macOS .keylayout ground truth: state "ka"
            # + reph -> "র্ক"). Folas (্র ্য) genuinely follow, so they append.
            if unit == "র্" and self.cons and not self.ends_with_hasanta(self.cons):
                self.cons = "র্" + self.cons
            else:
                self.cons += unit
            return ("compose", self.render())
        if unit in PREBASE:
            if not self.is_empty():
                out = self.render_final(); self.reset(); self.vowel = unit
                return ("commitThenCompose", out, self.render())
            self.vowel = unit; return ("compose", self.render())
        if unit in POSTBASE:
            if not self.cons and not self.vowel and not self.trail:
                self.reset(); return ("commit", INDEP.get(unit, unit))
            self.vowel = self.combine_vowel(self.vowel, unit)
            return ("compose", self.render())
        if unit in TRAIL:
            self.trail += unit; return ("compose", self.render())
        if unit in INDEPENDENTS or unit in STANDALONE:
            out = self.render_final() + unit; self.reset()
            return ("commit", out)
        if self.is_consonant(unit):
            if not self.cons:
                self.cons = unit; return ("compose", self.render())
            if self.ends_with_hasanta(self.cons):
                self.cons += unit; return ("compose", self.render())
            out = self.render_final(); self.reset(); self.cons = unit
            return ("commitThenCompose", out, self.render())
        out = self.render_final() + unit; self.reset()
        return ("commit", out)

    def pop_last(self):
        if self.trail: self.trail = self.trail[:-1]
        elif self.vowel: self.vowel = self.vowel[:-1]
        elif self.cons: self.cons = self.cons[:-1]


def type_keys(tokens):
    """tokens: list of (qwerty_key, shift). Returns committed+flush, NFC-normalized."""
    e = Engine()
    committed = ""
    composing = ""
    for key, shift in tokens:
        r = e.process(SC[key], shift)
        if r[0] == "ignore":
            pass
        elif r[0] == "compose":
            composing = r[1]
        elif r[0] == "commit":
            committed += r[1]; composing = ""
        elif r[0] == "commitThenCompose":
            committed += r[1]; composing = r[2]
    committed += e.flush()
    return unicodedata.normalize("NFC", committed)


def K(s):
    """Parse a compact key spec: space-separated tokens, '^' prefix = shift.
    e.g. 'c j' or '^h f n ^a b'."""
    out = []
    for tok in s.split():
        shift = tok.startswith("^")
        key = tok[1:] if shift else tok
        out.append((key, shift))
    return out


# SPEC §7 corpus. (word, key spec, expected)
CORPUS = [
    ("কে",      "c j",          "কে"),
    ("কি",      "d j",          "কি"),
    ("কো",      "c j f",        "কো"),
    ("কৈ",      "^c j",         "কৈ"),
    ("ক্ষি",    "d j g ^n",     "ক্ষি"),
    ("হচ্ছে",   "i c y g ^y",   "হচ্ছে"),
    ("ভার্সন",  "^h f n ^a b",  "ভার্সন"),
    ("কর্ম",    "j m ^a",       "কর্ম"),
    ("ফ্রি",    "d ^r z",       "ফ্রি"),
    ("বাংলা",   "h f ^q ^v f",  "বাংলা"),
    ("আমি",     "f d m",        "আমি"),
    ("ভাই",     "^h f d",       "ভাই"),
    ("বল",      "h ^v",         "বল"),
]


def main():
    passed = 0
    for word, spec, expected in CORPUS:
        got = type_keys(K(spec))
        exp = unicodedata.normalize("NFC", expected)
        ok = got == exp
        passed += ok
        mark = "PASS" if ok else "FAIL"
        line = f"[{mark}] {word:8} keys='{spec}'"
        if not ok:
            line += f"\n        got     : {got!r}  ({' '.join(f'U+{ord(c):04X}' for c in got)})"
            line += f"\n        expected: {exp!r}  ({' '.join(f'U+{ord(c):04X}' for c in exp)})"
        print(line)
    print(f"\n{passed}/{len(CORPUS)} passed")
    return 0 if passed == len(CORPUS) else 1


if __name__ == "__main__":
    raise SystemExit(main())

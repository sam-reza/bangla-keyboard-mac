# Faithful interpreter for Apple .keylayout deadkey state machines.
# Used to (a) cross-validate against the Unicode SPEC corpus and (b) extract the
# Bangla Classic (legacy ANSI Bangla) mapping for the Windows port. The Classic
# layout is a fully-deferred deadkey machine, so rather than hand-port it we run
# its own FSM — same algorithm UCKeyTranslate uses — which guarantees Mac parity.
import xml.etree.ElementTree as ET
import unicodedata, sys, os

# macOS virtual keycode for each US-QWERTY key (for feeding test specs)
KC = {
 'a':0,'s':1,'d':2,'f':3,'h':4,'g':5,'z':6,'x':7,'c':8,'v':9,'b':11,'q':12,'w':13,'e':14,
 'r':15,'y':16,'t':17,'1':18,'2':19,'3':20,'4':21,'6':22,'5':23,'=':24,'9':25,'7':26,'-':27,
 '8':28,'0':29,']':30,'o':31,'u':32,'[':33,'i':34,'p':35,'l':37,'j':38,"'":39,'k':40,';':41,
 '\\':42,',':43,'/':44,'n':45,'m':46,'.':47,'`':50,
}

class KeyLayout:
    def __init__(self, path):
        with open(path, 'r', encoding='utf-8') as fh:
            text = fh.read()
        # Ukelele writes an invalid uppercase '<?XML' decl + an external DOCTYPE;
        # fix the decl and drop the DOCTYPE so ElementTree can parse it.
        text = text.replace('<?XML', '<?xml', 1)
        import re
        text = re.sub(r'<!DOCTYPE[^>]*>', '', text, count=1)
        # XML 1.0 forbids most control-char numeric refs (the keypad/system keys
        # use &#x001B; etc.) — replace out-of-range refs with U+FFFD so it parses.
        def _fix(m):
            cp = int(m.group(1), 16)
            ok = cp in (0x09, 0x0A, 0x0D) or 0x20 <= cp <= 0xD7FF or 0xE000 <= cp <= 0xFFFD or cp >= 0x10000
            return m.group(0) if ok else '&#xFFFD;'
        text = re.sub(r'&#x([0-9A-Fa-f]+);', _fix, text)
        root = ET.fromstring(text)
        # modifier -> keyMap index: find indices for base (no mods) and shift
        self.base_idx, self.shift_idx = 0, 1
        modmap = root.find('modifierMap')
        if modmap is not None:
            for sel in modmap.findall('keyMapSelect'):
                idx = int(sel.get('mapIndex'))
                mods = ' '.join(m.get('keys', '') for m in sel.findall('modifier'))
                # base = the empty-modifier layer; shift = anyShift but NOT
                # anyOption and NOT command (so we skip shift+option / cmd layers).
                toks = set(mods.replace('?', '').split())
                if not toks:
                    self.base_idx = idx
                elif 'anyShift' in toks and 'anyOption' not in toks and 'command' not in toks:
                    self.shift_idx = idx
        # keyMaps (use ANSI set)
        self.keymaps = {}   # index -> { keycode -> ('output'|'action', value) }
        kset = None
        for ks in root.findall('keyMapSet'):
            if ks.get('id') == 'ANSI':
                kset = ks; break
        if kset is None:
            kset = root.find('keyMapSet')
        for km in kset.findall('keyMap'):
            idx = int(km.get('index'))
            d = {}
            for key in km.findall('key'):
                code = int(key.get('code'))
                if key.get('output') is not None:
                    d[code] = ('output', key.get('output'))
                elif key.get('action') is not None:
                    d[code] = ('action', key.get('action'))
            self.keymaps[idx] = d
        # actions: id -> { state -> (output, next) }
        self.actions = {}
        for act in root.findall('actions/action'):
            aid = act.get('id')
            rules = {}
            for w in act.findall('when'):
                rules[w.get('state')] = (w.get('output'), w.get('next'))
            self.actions[aid] = rules
        # terminators: state -> output
        self.terminators = {}
        for w in root.findall('terminators/when'):
            self.terminators[w.get('state')] = w.get('output', '')

    def term(self, state):
        return self.terminators.get(state, '')

    def process(self, state, keycode, shift):
        """Returns (emitted_string, new_state). Apple deadkey algorithm."""
        idx = self.shift_idx if shift else self.base_idx
        entry = self.keymaps.get(idx, {}).get(keycode)
        if entry is None and shift:
            entry = self.keymaps.get(self.base_idx, {}).get(keycode)
        if entry is None:
            return ('', state)
        kind, val = entry
        # unify: a plain output is an action with one rule {none: (output, None)}
        rules = {'none': (val, None)} if kind == 'output' else self.actions.get(val, {})
        if state in rules:
            out, nxt = rules[state]
            return (out or '', nxt if nxt else 'none')
        # no transition from current state: flush its terminator, restart in none
        out = self.term(state)
        if 'none' in rules:
            o2, nxt = rules['none']
            return (out + (o2 or ''), nxt if nxt else 'none')
        return (out, 'none')

    def type_keys(self, tokens):
        """tokens: list of (qwerty, shift). Returns the produced string + final flush."""
        state = 'none'; out = ''
        for key, shift in tokens:
            e, state = self.process(state, KC[key], shift)
            out += e
        out += self.term(state)   # flush dangling deadkey
        return out


def K(spec):
    out = []
    for tok in spec.split():
        sh = tok.startswith('^'); out.append((tok[1:] if sh else tok, sh))
    return out

# Same 13-word corpus as verify.py — used to PROVE the interpreter is correct by
# running it on the Unicode keylayout (must reproduce the SPEC expected output).
CORPUS = [
    ("কে","c j","কে"), ("কি","d j","কি"), ("কো","c j f","কো"), ("কৈ","^c j","কৈ"),
    ("ক্ষি","d j g ^n","ক্ষি"), ("হচ্ছে","i c y g ^y","হচ্ছে"), ("ভার্সন","^h f n ^a b","ভার্সন"),
    ("কর্ম","j m ^a","কর্ম"), ("ফ্রি","d ^r z","ফ্রি"), ("বাংলা","h f ^q ^v f","বাংলা"),
    ("আমি","f d m","আমি"), ("ভাই","^h f d","ভাই"), ("বল","h ^v","বল"),
]

def hexs(s): return ' '.join(f'U+{ord(c):04X}' for c in s)

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    kl_dir = os.path.join(here, '..', '..', 'macos', 'src', 'keylayouts')
    uni = KeyLayout(os.path.join(kl_dir, 'Bangla Unicode.keylayout'))

    print("=== interpreter vs SPEC corpus on the Unicode .keylayout ===")
    passed = 0
    for word, spec, exp in CORPUS:
        got = unicodedata.normalize('NFC', uni.type_keys(K(spec)))
        e = unicodedata.normalize('NFC', exp)
        ok = got == e; passed += ok
        print(f"[{'PASS' if ok else 'FAIL'}] {word:8} {spec!r:18} -> {got!r}" + ('' if ok else f"  expected {e!r}"))
    print(f"{passed}/{len(CORPUS)} (proves the interpreter matches the engine/SPEC)\n")
    return 0 if passed == len(CORPUS) else 1

if __name__ == '__main__':
    raise SystemExit(main())

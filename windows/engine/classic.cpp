#include "classic.h"
#include "classic_table.h"

namespace bangla {

using namespace bangla::classic_table;

bool ClassicEngine::wouldHandle(unsigned scan) const {
    if (scan > 0xFF) return false;
    return KEYMAP[0][scan] >= 0 || KEYMAP[1][scan] >= 0;
}

std::u16string ClassicEngine::flush() {
    std::u16string o = STRINGS[TERM[state_]];
    state_ = 0;
    return o;
}

std::u16string ClassicEngine::process(unsigned scan, bool shift) {
    if (scan > 0xFF) return u"";
    int aid = KEYMAP[shift ? 1 : 0][scan];
    if (aid < 0 && shift) aid = KEYMAP[0][scan];   // shift falls back to base
    if (aid < 0) return u"";
    const Span& a = ACTIONS[aid];

    // transition from the current state
    for (int i = 0; i < a.cnt; ++i) {
        const Rule& r = RULES[a.off + i];
        if (r.state == state_) { std::u16string o = STRINGS[r.out]; state_ = r.next; return o; }
    }
    // no transition: flush this state's terminator, then apply the none-rule
    std::u16string o = STRINGS[TERM[state_]];
    for (int i = 0; i < a.cnt; ++i) {
        const Rule& r = RULES[a.off + i];
        if (r.state == 0) { o += STRINGS[r.out]; state_ = r.next; return o; }
    }
    state_ = 0;
    return o;
}

} // namespace bangla

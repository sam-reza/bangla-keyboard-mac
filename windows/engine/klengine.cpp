#include "klengine.h"

namespace bangla {

bool KLEngine::wouldHandle(unsigned scan) const {
    if (scan > 0xFF) return false;
    return t_->keymap[0][scan] >= 0 || t_->keymap[1][scan] >= 0;
}

std::u16string KLEngine::flush() {
    std::u16string o = t_->strings[t_->term[state_]];
    state_ = 0;
    return o;
}

std::u16string KLEngine::process(unsigned scan, bool shift) {
    if (scan > 0xFF) return u"";
    int aid = t_->keymap[shift ? 1 : 0][scan];
    if (aid < 0 && shift) aid = t_->keymap[0][scan];   // shift falls back to base
    if (aid < 0) return u"";
    const Span& a = t_->actions[aid];

    // transition from the current state
    for (int i = 0; i < a.cnt; ++i) {
        const Rule& r = t_->rules[a.off + i];
        if (r.state == state_) { std::u16string o = t_->strings[r.out]; state_ = r.next; return o; }
    }
    // no transition: flush this state's terminator, then apply the none-rule
    std::u16string o = t_->strings[t_->term[state_]];
    for (int i = 0; i < a.cnt; ++i) {
        const Rule& r = t_->rules[a.off + i];
        if (r.state == 0) { o += t_->strings[r.out]; state_ = r.next; return o; }
    }
    state_ = 0;
    return o;
}

} // namespace bangla

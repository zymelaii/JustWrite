#include "TextBlockLayout.h"

void TextBlockLayout::reset(QStringView text, int start_pos) {
    lines.clear();
    text_ref   = text;
    text_pos   = start_pos;
    cursor_pos = 0;
    cursor_row = 0;
    cursor_col = 0;

    TextLineLayout line;
    line.reset(text_pos);
    lines.push_back(line);
}

void TextBlockLayout::setMaxWidth(int width) {
    max_width      = width;
    lines[0].dirty = true;
}

void TextBlockLayout::checkOrUpdateLeadingSpace(const QFontMetrics &fm) {
    //! NOTE: we depend on the leading space to decide whether font changes
    const auto leading_space = fm.horizontalAdvance(QChar(U'\u3000')) * 2;
    if (auto &first_line = lines[0]; leading_space != first_line.offset) {
        first_line.offset = leading_space;
        first_line.dirty  = true;
    }
}

void TextBlockLayout::shiftOrigin(int offset) {
    text_pos += offset;
    for (auto &line : lines) { line.text_endp += offset; }
}

void TextBlockLayout::quickInsert(qsizetype size) {
    for (int i = cursor_row; i < lines.size(); ++i) { lines[i].text_endp += size; }
    lines[cursor_row].dirty  = true;
    cursor_pos              += size;
    cursor_col              += size;
}

void TextBlockLayout::foldInto(int index) {
    if (index < 0 || index >= lines.size()) { return; }
    if (cursor_row > index) {
        const auto origin  = index == 0 ? text_pos : lines[index - 1].text_endp;
        cursor_col        += lines[cursor_row - 1].text_endp - origin;
        cursor_row         = index;
    }
    lines[index].dirty = true;
    if (index + 1 < lines.size()) {
        lines[index].text_endp = lines.back().text_endp;
        lines.remove(index + 1, lines.size() - index - 1);
    }
}

void TextBlockLayout::render(const QFontMetrics &fm) {
    checkOrUpdateLeadingSpace(fm);

    int index = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].dirty) {
            index = i;
            break;
        }
    }
    if (index == -1) { return; }

    foldInto(index);
    Q_ASSERT(lines.size() == index + 1);

    int pos = index == 0 ? text_pos : lines[index - 1].text_endp;
    while (true) {
        auto      &cur_line   = lines[index];
        const auto text       = line(index);
        int        text_width = max_width - cur_line.offset;
        const int  fit_len    = boundingTextLength(fm, text, text_width);
        Q_ASSERT(fit_len <= text.length());

        cur_line.text_width = text_width;
        cur_line.mean_width = fit_len == text.length() ? 0 : max_width - cur_line.offset - text_width;
        cur_line.dirty      = false;

        if (fit_len == text.length()) { break; }

        const auto endp     = cur_line.text_endp;
        cur_line.text_endp -= text.length() - fit_len;
        Q_ASSERT(index + 1 == lines.size());
        TextLineLayout line;
        line.reset(endp);
        lines.push_back(line);

        if (cursor_row == index && cursor_col >= fit_len) {
            ++cursor_row;
            cursor_col -= fit_len;
        }

        ++index;
    }
}

QStringView TextBlockLayout::line(int index) const {
    if (index < 0 || index >= lines.size()) { return {}; }
    const auto pos  = index == 0 ? text_pos : lines[index - 1].text_endp;
    const auto size = lines[index].text_endp - pos;
    return text_ref.mid(pos, size);
}

TextBlockLayout TextBlockLayout::split() {
    //! FIXME: split but not complete
    TextBlockLayout block;
    block.reset(text_ref, text_pos + cursor_pos);
    for (int i = cursor_row + 1; i < lines.size(); ++i) { block.lines.push_back(lines[i]); }
    if (cursor_col == 0) {
        block.lines[0] = lines[cursor_row];
        if (cursor_row > 0) {
            --cursor_row;
            cursor_col = line(cursor_row).length();
        }
    } else if (auto line_size = line(cursor_row).length(); cursor_col < line_size) {
        block.lines[0].text_endp    += line_size - cursor_col;
        lines[cursor_row].text_endp -= line_size - cursor_col;
    } else if (block.lines.size() > 1) {
        qDebug() << "c";
        block.lines.remove(0);
        block.lines[0].dirty = true;
    }
    int remove_from = cursor_row > 0 && cursor_col == 0 ? cursor_row : cursor_row + 1;
    lines.remove(remove_from, lines.size() - remove_from);
    lines[cursor_row].dirty = true;
    return block;
}

int TextBlockLayout::boundingTextLength(const QFontMetrics &fm, QStringView text, int &width) {
    int text_width = 0;
    int count      = 0;
    for (auto &c : text) {
        if (const auto char_width = fm.horizontalAdvance(c); text_width + char_width <= width) {
            text_width += char_width;
            ++count;
        } else {
            break;
        }
    }
    width = text_width;
    return count;
}

QDebug operator<<(QDebug dbg, const TextBlockLayout &block) {
    dbg.nospace() << "[";
    for (int i = 0; i < block.lines.size(); ++i) {
        dbg.nospace().noquote() << QString(R"({ endp: %1, text: "%2" },)").arg(block.lines[i].text_endp).arg(block.line(i));
    }
    dbg.nospace() << "]";
    return dbg.maybeSpace();
}

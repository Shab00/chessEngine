#include "position.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

static int buf_appendf(char **dst, size_t *rem, const char *fmt, ...)
{
    if (*rem == 0) return -1;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(*dst, *rem, fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if ((size_t)n >= *rem) return -1; 
    *dst += n;
    *rem -= (size_t)n;
    return 0;
}

static int piece_abs_val(int8_t v) { return v < 0 ? -v : v; }

static char piece_value_to_letter(int8_t v)
{
    int a = piece_abs_val(v);
    char base;
    switch (a) {
    case PIECE_PAWN:   base = 'p'; break;
    case PIECE_KNIGHT: base = 'n'; break;
    case PIECE_BISHOP: base = 'b'; break;
    case PIECE_ROOK:   base = 'r'; break;
    case PIECE_QUEEN:  base = 'q'; break;
    case PIECE_KING:   base = 'k'; break;
    default: base = '?'; break;
    }
    return v > 0 ? (char) toupper((unsigned char)base) : base;
}

pos_error_t position_to_fen(const Position *pos, char *buf, size_t buf_size)
{
    if (pos == NULL || buf == NULL || buf_size == 0) return POS_ERR_INVALID_ARG;

    char *p = buf;
    size_t rem = buf_size;

    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            int idx = rank * 8 + file;
            int8_t v = pos->board[idx];
            if (v == PIECE_EMPTY) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    if (buf_appendf(&p, &rem, "%d", empty_count) != 0) return POS_ERR_BUF_SMALL;
                    empty_count = 0;
                }
                char letter = piece_value_to_letter(v);
                if (buf_appendf(&p, &rem, "%c", letter) != 0) return POS_ERR_BUF_SMALL;
            }
        }
        if (empty_count > 0) {
            if (buf_appendf(&p, &rem, "%d", empty_count) != 0) return POS_ERR_BUF_SMALL;
        }
        if (rank > 0) {
            if (buf_appendf(&p, &rem, "/") != 0) return POS_ERR_BUF_SMALL;
        }
    }

    if (buf_appendf(&p, &rem, " ") != 0) return POS_ERR_BUF_SMALL;

    if (pos->side_to_move == COLOR_WHITE) {
        if (buf_appendf(&p, &rem, "w") != 0) return POS_ERR_BUF_SMALL;
    } else {
        if (buf_appendf(&p, &rem, "b") != 0) return POS_ERR_BUF_SMALL;
    }

    if (buf_appendf(&p, &rem, " ") != 0) return POS_ERR_BUF_SMALL;

    if (pos->castling == 0) {
        if (buf_appendf(&p, &rem, "-") != 0) return POS_ERR_BUF_SMALL;
    } else {
        if (pos->castling & CASTLE_WHITE_K) if (buf_appendf(&p, &rem, "K") != 0) return POS_ERR_BUF_SMALL;
        if (pos->castling & CASTLE_WHITE_Q) if (buf_appendf(&p, &rem, "Q") != 0) return POS_ERR_BUF_SMALL;
        if (pos->castling & CASTLE_BLACK_K) if (buf_appendf(&p, &rem, "k") != 0) return POS_ERR_BUF_SMALL;
        if (pos->castling & CASTLE_BLACK_Q) if (buf_appendf(&p, &rem, "q") != 0) return POS_ERR_BUF_SMALL;
    }

    if (buf_appendf(&p, &rem, " ") != 0) return POS_ERR_BUF_SMALL;

    if (pos->en_passant == POS_NO_SQUARE) {
        if (buf_appendf(&p, &rem, "-") != 0) return POS_ERR_BUF_SMALL;
    } else {
        char sqbuf[3];
        position_square_to_coords(pos->en_passant, sqbuf, sizeof sqbuf);
        if (sqbuf[0] == '\0') return POS_ERR_INVALID_ARG;
        if (buf_appendf(&p, &rem, "%s", sqbuf) != 0) return POS_ERR_BUF_SMALL;
    }

    if (buf_appendf(&p, &rem, " ") != 0) return POS_ERR_BUF_SMALL;

    if (buf_appendf(&p, &rem, "%u", pos->halfmove_clock) != 0) return POS_ERR_BUF_SMALL;

    if (buf_appendf(&p, &rem, " ") != 0) return POS_ERR_BUF_SMALL;

    if (buf_appendf(&p, &rem, "%u", pos->fullmove_number) != 0) return POS_ERR_BUF_SMALL;

    return POS_OK;
}

void position_print_ascii(const Position *pos, FILE *out)
{
    if (pos == NULL || out == NULL) return;

    for (int r = 7; r >= 0; --r) {
        fprintf(out, "%d  ", r + 1);
        for (int f = 0; f < 8; ++f) {
            int idx = r * 8 + f;
            int8_t v = pos->board[idx];
            char ch = '.';
            if (v != PIECE_EMPTY) {
                int a = piece_abs_val(v);
                char base;
                switch (a) {
                case PIECE_PAWN:   base = 'P'; break;
                case PIECE_KNIGHT: base = 'N'; break;
                case PIECE_BISHOP: base = 'B'; break;
                case PIECE_ROOK:   base = 'R'; break;
                case PIECE_QUEEN:  base = 'Q'; break;
                case PIECE_KING:   base = 'K'; break;
                default: base = '?'; break;
                }
                ch = (v > 0) ? base : (char) tolower((unsigned char)base);
            }
            fprintf(out, "%c ", ch);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\n   a b c d e f g h\n");

    char epbuf[3] = "-";
    if (pos->en_passant != POS_NO_SQUARE) {
        position_square_to_coords(pos->en_passant, epbuf, sizeof epbuf);
    }
    fprintf(out, "side: %s  castling: ",
            pos->side_to_move == COLOR_WHITE ? "w" : "b");
    if (pos->castling == 0) {
        fprintf(out, "-");
    } else {
        if (pos->castling & CASTLE_WHITE_K) fprintf(out, "K");
        if (pos->castling & CASTLE_WHITE_Q) fprintf(out, "Q");
        if (pos->castling & CASTLE_BLACK_K) fprintf(out, "k");
        if (pos->castling & CASTLE_BLACK_Q) fprintf(out, "q");
    }
    fprintf(out, "  en-passant: %s  halfmove: %u  fullmove: %u\n",
            epbuf, pos->halfmove_clock, pos->fullmove_number);
}

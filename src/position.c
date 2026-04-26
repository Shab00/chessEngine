#include "position.h"
#include "hash.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

static const int knight_moves[8] = {17, 15, 10, 6, -17, -15, -10, -6};
static const int king_moves[8]   = {1, -1, 8, -8, 9, -9, 7, -7};

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

void position_init(Position *pos)
{
    if (pos == NULL) return;

    memset(pos->board, 0, sizeof pos->board);
    pos->side_to_move = COLOR_WHITE;
    pos->castling = 0;
    pos->en_passant = POS_NO_SQUARE;
    pos->halfmove_clock = 0;
    pos->fullmove_number = 1;

    pos->hash = position_hash(pos);
}

int position_square_from_coords(char file_char, char rank_char)
{
    if (file_char < 'a' || file_char > 'h') return POS_NO_SQUARE;
    if (rank_char < '1' || rank_char > '8') return POS_NO_SQUARE;
    int file = file_char - 'a';
    int rank = rank_char - '1';
    return SQ_INDEX(file, rank);
}

void position_square_to_coords(int sq, char *buf, size_t buf_size)
{
    if (buf == NULL || buf_size < 3) return;
    if (sq < 0 || sq >= 64) {
        buf[0] = '\0';
        return;
    }
    int file = SQ_FILE(sq);
    int rank = SQ_RANK(sq);
    buf[0] = (char)('a' + file);
    buf[1] = (char)('1' + rank);
    buf[2] = '\0';
}

bool position_is_square_attacked(const Position *pos, int sq, int attacker_color) {
    int their_pawn   = attacker_color == COLOR_WHITE ? PIECE_PAWN : -PIECE_PAWN;
    int their_knight = attacker_color == COLOR_WHITE ? PIECE_KNIGHT : -PIECE_KNIGHT;
    int their_bishop = attacker_color == COLOR_WHITE ? PIECE_BISHOP : -PIECE_BISHOP;
    int their_rook   = attacker_color == COLOR_WHITE ? PIECE_ROOK   : -PIECE_ROOK;
    int their_queen  = attacker_color == COLOR_WHITE ? PIECE_QUEEN  : -PIECE_QUEEN;
    int their_king   = attacker_color == COLOR_WHITE ? PIECE_KING   : -PIECE_KING;

    int sf = SQ_FILE(sq), sr = SQ_RANK(sq);

    // Pawn attacks
    if (attacker_color == COLOR_WHITE) {
        if (sf > 0 && sr > 0 && pos->board[sq - 9] == their_pawn) return true; // down-left
        if (sf < 7 && sr > 0 && pos->board[sq - 7] == their_pawn) return true; // down-right
    } else {
        if (sf > 0 && sr < 7 && pos->board[sq + 7] == their_pawn) return true; // up-left
        if (sf < 7 && sr < 7 && pos->board[sq + 9] == their_pawn) return true; // up-right
    }
    // Knight attacks
    for (int i = 0; i < 8; ++i) {
        int to = sq + knight_moves[i];
        if (to < 0 || to >= 64) continue;
        int df = abs(SQ_FILE(to) - sf), dr = abs(SQ_RANK(to) - sr);
        if ((df == 1 && dr == 2) || (df == 2 && dr == 1)) {
            if (pos->board[to] == their_knight) return true;
        }
    }

    // Bishop/Queen attacks (diagonals)
    for (int df = -1; df <= 1; df += 2)
        for (int dr = -1; dr <= 1; dr += 2)
            for (int n = 1; ; ++n) {
                int f = sf + n*df, r = sr + n*dr;
                if (f < 0 || f > 7 || r < 0 || r > 7) break;
                int idx = SQ_INDEX(f, r);
                int v = pos->board[idx];
                if (v == PIECE_EMPTY) continue;
                if (v == their_bishop || v == their_queen) return true;
                break;
            }

    // Rook/Queen attacks (straight)
    for (int df = -1; df <= 1; ++df)
        for (int dr = -1; dr <= 1; ++dr) {
            if ((df == 0) == (dr == 0)) continue; // skip (0,0) and diagonals
            for (int n = 1; ; ++n) {
                int f = sf + n*df, r = sr + n*dr;
                if (f < 0 || f > 7 || r < 0 || r > 7) break;
                int idx = SQ_INDEX(f, r);
                int v = pos->board[idx];
                if (v == PIECE_EMPTY) continue;
                if (v == their_rook || v == their_queen) return true;
                break;
            }
        }

    // King attacks: one square away
    for (int i = 0; i < 8; ++i) {
        int to = sq + king_moves[i];
        if (to < 0 || to >= 64) continue;
        int df = abs(SQ_FILE(to) - sf), dr = abs(SQ_RANK(to) - sr);
        if (df <= 1 && dr <= 1) {
            if (pos->board[to] == their_king) return true;
        }
    }
    return false;
}

bool position_king_in_check(const Position *pos, int color) {
    int king = (color == COLOR_WHITE) ? PIECE_KING : -PIECE_KING;
    for (int sq = 0; sq < 64; ++sq) {
        if (pos->board[sq] == king)
            return position_is_square_attacked(pos, sq, color ^ 1); // attacked by the enemy
    }
    // King missing: treat as "in check"
    return true;
}

int position_material(const Position *pos) {
    int total = 0;
    for (int i = 0; i < 64; ++i) {
        int8_t v = pos->board[i];
        int a = abs(v);
        switch (a) {
            case PIECE_PAWN:   total += 1; break;
            case PIECE_KNIGHT: total += 3; break;
            case PIECE_BISHOP: total += 3; break;
            case PIECE_ROOK:   total += 5; break;
            case PIECE_QUEEN:  total += 9; break;
            default: break;
        }
    }
    return total;
}

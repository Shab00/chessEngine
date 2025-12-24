#include "position.h"
#include <string.h> 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

void position_init(Position *pos)
{
    if (pos == NULL) return;

    memset(pos->board, 0, sizeof pos->board);
    pos->side_to_move = COLOR_WHITE;
    pos->castling = 0;
    pos->en_passant = POS_NO_SQUARE;
    pos->halfmove_clock = 0;
    pos->fullmove_number = 1;
}

static int piece_type_from_letter(char c)
{
    switch (c) {
    case 'p': case 'P': return PIECE_PAWN;
    case 'n': case 'N': return PIECE_KNIGHT;
    case 'b': case 'B': return PIECE_BISHOP;
    case 'r': case 'R': return PIECE_ROOK;
    case 'q': case 'Q': return PIECE_QUEEN;
    case 'k': case 'K': return PIECE_KING;
    default: return 0;
    }
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

static pos_error_t parse_placement_field(Position *pos,
                                        const char *s,
                                        const char **out,
                                        char *errbuf, size_t errbuf_size)
{
    if (pos == NULL || s == NULL) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "null argument");
        return POS_ERR_INVALID_ARG;
    }

    int rank = 7;
    int file = 0;
    const char *p = s;

    while (*p && !isspace((unsigned char)*p)) {
        char ch = *p;

        if (ch == '/') {
            if (file != 8) {
                if (errbuf && errbuf_size)
                    snprintf(errbuf, errbuf_size, "bad FEN: rank %d has %d files (expected 8)", 8 - rank, file);
                return POS_ERR_BAD_FEN;
            }
            rank--;
            if (rank < 0) {
                if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: too many ranks");
                return POS_ERR_BAD_FEN;
            }
            file = 0;
        } else if (isdigit((unsigned char)ch)) {
            int n = ch - '0';
            if (n < 1 || n > 8) {
                if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: invalid digit '%c'", ch);
                return POS_ERR_BAD_FEN;
            }
            for (int i = 0; i < n; ++i) {
                if (file >= 8) {
                    if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: too many files in rank %d", 8 - rank);
                    return POS_ERR_BAD_FEN;
                }
                pos->board[rank * 8 + file] = PIECE_EMPTY;
                file++;
            }
        } else if (isalpha((unsigned char)ch)) {
            int pt = piece_type_from_letter(ch);
            if (pt == 0) {
                if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: invalid piece letter '%c'", ch);
                return POS_ERR_BAD_FEN;
            }
            if (file >= 8) {
                if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: too many files in rank %d", 8 - rank);
                return POS_ERR_BAD_FEN;
            }
            int8_t val = (isupper((unsigned char)ch) ? (int8_t)pt : (int8_t)-pt);
            pos->board[rank * 8 + file] = val;
            file++;
        } else {
            if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: unexpected character '%c'", ch);
            return POS_ERR_BAD_FEN;
        }

        p++;
    }

    if (rank != 0 || file != 8) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: incomplete placement (rank=%d,file=%d)", rank, file);
        return POS_ERR_BAD_FEN;
    }

    if (out) *out = p;
    return POS_OK;
}

pos_error_t position_validate(const Position *pos, char *errbuf, size_t errbuf_size)
{
    if (pos == NULL) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "null position");
        return POS_ERR_INVALID_ARG;
    }

    int white_kings = 0;
    int black_kings = 0;
    for (int i = 0; i < 64; ++i) {
        int v = pos->board[i];
        int a = piece_abs((int8_t)v);
        if (a < 0 || a > 6) {
            if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "invalid piece value at sq %d: %d", i, v);
            return POS_ERR_INVARIANT;
        }
        if (v == PIECE_KING) white_kings++;
        if (v == -PIECE_KING) black_kings++;
    }

    if (white_kings != 1 || black_kings != 1) {
        if (errbuf && errbuf_size)
            snprintf(errbuf, errbuf_size, "invalid king count: white=%d black=%d", white_kings, black_kings);
        return POS_ERR_INVARIANT;
    }

    if (!(pos->castling <= (CASTLE_WHITE_K | CASTLE_WHITE_Q | CASTLE_BLACK_K | CASTLE_BLACK_Q))) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "invalid castling mask");
        return POS_ERR_INVARIANT;
    }

    if (pos->en_passant != POS_NO_SQUARE) {
        if (pos->en_passant < 0 || pos->en_passant >= 64) {
            if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "invalid en-passant square: %d", pos->en_passant);
            return POS_ERR_INVARIANT;
        }
    }

    if (pos->fullmove_number < 1) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "invalid fullmove number: %u", pos->fullmove_number);
        return POS_ERR_INVARIANT;
    }

    return POS_OK;
}

pos_error_t position_from_fen(Position *pos, const char *fen,
                              char *errbuf, size_t errbuf_size)
{
    if (pos == NULL || fen == NULL) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "null argument");
        return POS_ERR_INVALID_ARG;
    }

    position_init(pos);

    const char *p = fen;
    pos_error_t r = parse_placement_field(pos, p, &p, errbuf, errbuf_size);
    if (r != POS_OK) return r;

    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) {
        return POS_OK;
    }

    if (*p == 'w' || *p == 'W') {
        pos->side_to_move = COLOR_WHITE;
        p++;
    } else if (*p == 'b' || *p == 'B') {
        pos->side_to_move = COLOR_BLACK;
        p++;
    } else {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: expected side-to-move 'w' or 'b'");
        return POS_ERR_BAD_FEN;
    }

    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: missing fields after side-to-move");
        return POS_ERR_BAD_FEN;
    }

    if (*p == '-') {
        pos->castling = 0;
        p++;
    } else {
        uint8_t mask = 0;
        while (*p && !isspace((unsigned char)*p)) {
            char c = *p;
            switch (c) {
            case 'K': mask |= CASTLE_WHITE_K; break;
            case 'Q': mask |= CASTLE_WHITE_Q; break;
            case 'k': mask |= CASTLE_BLACK_K; break;
            case 'q': mask |= CASTLE_BLACK_Q; break;
            default:
                if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: invalid castling char '%c'", c);
                return POS_ERR_BAD_FEN;
            }
            p++;
        }
        pos->castling = mask;
    }

    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: missing fields after castling");
        return POS_ERR_BAD_FEN;
    }

    if (*p == '-') {
        pos->en_passant = POS_NO_SQUARE;
        p++;
    } else {
        if (!isalpha((unsigned char)p[0]) || !isdigit((unsigned char)p[1])) {
            if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: invalid en-passant '%c%c'", p[0], p[1]);
            return POS_ERR_BAD_FEN;
        }
        int sq = position_square_from_coords((char)tolower((unsigned char)p[0]), p[1]);
        if (sq == POS_NO_SQUARE) {
            if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: invalid en-passant square '%c%c'", p[0], p[1]);
            return POS_ERR_BAD_FEN;
        }
        pos->en_passant = (int8_t)sq;
        p += 2;
    }

    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: missing halfmove/fullmove fields");
        return POS_ERR_BAD_FEN;
    }

    {
        char *endptr = NULL;
        long v = strtol(p, &endptr, 10);
        if (endptr == p || v < 0) {
            if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: invalid halfmove clock");
            return POS_ERR_BAD_FEN;
        }
        pos->halfmove_clock = (uint16_t)v;
        p = endptr;
    }

    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) {
        if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: missing fullmove number");
        return POS_ERR_BAD_FEN;
    }

    {
        char *endptr = NULL;
        long v = strtol(p, &endptr, 10);
        if (endptr == p || v < 1) {
            if (errbuf && errbuf_size) snprintf(errbuf, errbuf_size, "bad FEN: invalid fullmove number");
            return POS_ERR_BAD_FEN;
        }
        pos->fullmove_number = (uint32_t)v;
        p = endptr;
    }

    r = position_validate(pos, errbuf, errbuf_size);
    if (r != POS_OK) return r;

    return POS_OK;
}

#include "movegen.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

static inline int file_of(int sq) { return SQ_FILE(sq); }
static inline int rank_of(int sq) { return SQ_RANK(sq); }

int is_square_attacked(const Position *pos, int sq, int by)
{
    if (sq < 0 || sq >= 64) return 0;
    int f = file_of(sq);
    int r = rank_of(sq);
    if (by == COLOR_WHITE) {
        if (r < 7) {
            if (f > 0) {
                int s = SQ_INDEX(f - 1, r + 1);
                if (pos->board[s] == PIECE_PAWN) return 1;
            }
            if (f < 7) {
                int s = SQ_INDEX(f + 1, r + 1);
                if (pos->board[s] == PIECE_PAWN) return 1;
            }
        }
    } else {
        if (r > 0) {
            if (f > 0) {
                int s = SQ_INDEX(f - 1, r - 1);
                if (pos->board[s] == -PIECE_PAWN) return 1;
            }
            if (f < 7) {
                int s = SQ_INDEX(f + 1, r - 1);
                if (pos->board[s] == -PIECE_PAWN) return 1;
            }
        }
    }
    const int knight_deltas[8][2] = { {2,1},{1,2},{-1,2},{-2,1},{-2,-1},{-1,-2},{1,-2},{2,-1} };
    for (int i = 0; i < 8; ++i) {
        int ff = f + knight_deltas[i][0];
        int rr = r + knight_deltas[i][1];
        if (ff < 0 || ff > 7 || rr < 0 || rr > 7) continue;
        int s = SQ_INDEX(ff, rr);
        int8_t v = pos->board[s];
        if (v == PIECE_EMPTY) continue;
        if (by == COLOR_WHITE && v == PIECE_KNIGHT) return 1;
        if (by == COLOR_BLACK && v == -PIECE_KNIGHT) return 1;
    }
    for (int df = -1; df <= 1; ++df) {
        for (int dr = -1; dr <= 1; ++dr) {
            if (df == 0 && dr == 0) continue;
            int ff = f + df, rr = r + dr;
            if (ff < 0 || ff > 7 || rr < 0 || rr > 7) continue;
            int s = SQ_INDEX(ff, rr);
            int8_t v = pos->board[s];
            if (v == PIECE_EMPTY) continue;
            if (by == COLOR_WHITE && v == PIECE_KING) return 1;
            if (by == COLOR_BLACK && v == -PIECE_KING) return 1;
        }
    }
    const int dirs[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };
    for (int d = 0; d < 8; ++d) {
        int df = dirs[d][0], dr = dirs[d][1];
        int ff = f + df, rr = r + dr;
        for (; ff >= 0 && ff < 8 && rr >= 0 && rr < 8; ff += df, rr += dr) {
            int s = SQ_INDEX(ff, rr);
            int8_t v = pos->board[s];
            if (v == PIECE_EMPTY) continue;
            int a = piece_abs(v);
            if (by == COLOR_WHITE && v > 0) {
                if (d < 4) {
                    if (a == PIECE_ROOK || a == PIECE_QUEEN) return 1;
                } else {
                    if (a == PIECE_BISHOP || a == PIECE_QUEEN) return 1;
                }
            } else if (by == COLOR_BLACK && v < 0) {
                if (d < 4) {
                    if (a == PIECE_ROOK || a == PIECE_QUEEN) return 1;
                } else {
                    if (a == PIECE_BISHOP || a == PIECE_QUEEN) return 1;
                }
            }
            break;
        }
    }
    return 0;
}

static int find_king_sq(const Position *pos, int color)
{
    for (int i = 0; i < 64; ++i) {
        int8_t v = pos->board[i];
        if (color == COLOR_WHITE && v == PIECE_KING) return i;
        if (color == COLOR_BLACK && v == -PIECE_KING) return i;
    }
    return POS_NO_SQUARE;
}

typedef struct {
    int from, to;
    int8_t moved_piece;
    int8_t captured_piece;
    uint8_t prev_castling;
    int8_t prev_en_passant;
    uint16_t prev_halfmove;
    uint32_t prev_fullmove;
    int ep_capture_sq;
} Undo;

static void make_move_raw(Position *pos, int from, int to, int promotion, Undo *undo)
{
    assert(from >= 0 && from < 64 && to >= 0 && to < 64);
    undo->from = from;
    undo->to = to;
    undo->moved_piece = pos->board[from];
    undo->captured_piece = pos->board[to];
    undo->prev_castling = pos->castling;
    undo->prev_en_passant = pos->en_passant;
    undo->prev_halfmove = pos->halfmove_clock;
    undo->prev_fullmove = pos->fullmove_number;
    undo->ep_capture_sq = POS_NO_SQUARE;
    pos->board[to] = pos->board[from];
    pos->board[from] = PIECE_EMPTY;
    if (promotion != 0) {
        int8_t sign = (undo->moved_piece > 0) ? 1 : -1;
        pos->board[to] = (int8_t)(sign * promotion);
    }
    if (piece_abs(undo->moved_piece) == PIECE_PAWN || undo->captured_piece != PIECE_EMPTY) {
        pos->halfmove_clock = 0;
    } else {
        pos->halfmove_clock++;
    }
    pos->en_passant = POS_NO_SQUARE;
    if (piece_abs(undo->moved_piece) == PIECE_PAWN) {
        int fr = rank_of(from), tr = rank_of(to);
        if (abs(tr - fr) == 2) {
            int ep_rank = (fr + tr) / 2;
            int ep_file = file_of(from);
            pos->en_passant = SQ_INDEX(ep_file, ep_rank);
        }
        if (undo->captured_piece == PIECE_EMPTY && file_of(from) != file_of(to)) {
            if (undo->prev_en_passant != POS_NO_SQUARE && undo->prev_en_passant == to) {
                int dir = (undo->moved_piece > 0) ? -1 : 1;
                int cap_sq = SQ_INDEX(file_of(to), rank_of(to) + dir);
                if (cap_sq >= 0 && cap_sq < 64) {
                    undo->captured_piece = pos->board[cap_sq];
                    undo->ep_capture_sq = cap_sq;
                    pos->board[cap_sq] = PIECE_EMPTY;
                }
            }
        }
    }
    if (piece_abs(undo->moved_piece) == PIECE_KING) {
        int ffile = file_of(from), tfile = file_of(to), frank = rank_of(from);
        if (abs(tfile - ffile) == 2) {
            if (tfile > ffile) {
                int rook_from = SQ_INDEX(7, frank);
                int rook_to   = SQ_INDEX(5, frank);
                pos->board[rook_to] = pos->board[rook_from];
                pos->board[rook_from] = PIECE_EMPTY;
            } else {
                int rook_from = SQ_INDEX(0, frank);
                int rook_to   = SQ_INDEX(3, frank);
                pos->board[rook_to] = pos->board[rook_from];
                pos->board[rook_from] = PIECE_EMPTY;
            }
        }
        if (undo->moved_piece > 0) {
            pos->castling &= ~(CASTLE_WHITE_K | CASTLE_WHITE_Q);
        } else {
            pos->castling &= ~(CASTLE_BLACK_K | CASTLE_BLACK_Q);
        }
    }
    if (piece_abs(undo->moved_piece) == PIECE_ROOK) {
        if (undo->moved_piece > 0) {
            if (from == SQ_INDEX(0,0)) pos->castling &= ~CASTLE_WHITE_Q;
            if (from == SQ_INDEX(7,0)) pos->castling &= ~CASTLE_WHITE_K;
        } else {
            if (from == SQ_INDEX(0,7)) pos->castling &= ~CASTLE_BLACK_Q;
            if (from == SQ_INDEX(7,7)) pos->castling &= ~CASTLE_BLACK_K;
        }
    }
    if (undo->captured_piece != PIECE_EMPTY && piece_abs(undo->captured_piece) == PIECE_ROOK) {
        if (undo->captured_piece > 0) {
            if (to == SQ_INDEX(0,0)) pos->castling &= ~CASTLE_WHITE_Q;
            if (to == SQ_INDEX(7,0)) pos->castling &= ~CASTLE_WHITE_K;
        } else {
            if (to == SQ_INDEX(0,7)) pos->castling &= ~CASTLE_BLACK_Q;
            if (to == SQ_INDEX(7,7)) pos->castling &= ~CASTLE_BLACK_K;
        }
    }
    pos->side_to_move = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    if (pos->side_to_move == COLOR_WHITE) pos->fullmove_number++;
}

static void unmake_move_raw(Position *pos, const Undo *undo)
{
    pos->side_to_move = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    pos->fullmove_number = undo->prev_fullmove;
    pos->halfmove_clock = undo->prev_halfmove;
    pos->castling = undo->prev_castling;
    pos->en_passant = undo->prev_en_passant;
    pos->board[undo->from] = undo->moved_piece;
    if (undo->ep_capture_sq != POS_NO_SQUARE) {
        pos->board[undo->to] = PIECE_EMPTY;
        pos->board[undo->ep_capture_sq] = undo->captured_piece;
    } else {
        pos->board[undo->to]   = undo->captured_piece;
    }
    if (piece_abs(undo->moved_piece) == PIECE_KING) {
        int ffile = file_of(undo->from), tfile = file_of(undo->to), frank = rank_of(undo->from);
        if (abs(tfile - ffile) == 2) {
            if (tfile > ffile) {
                int rook_from = SQ_INDEX(7, frank);
                int rook_to   = SQ_INDEX(5, frank);
                pos->board[rook_from] = pos->board[rook_to];
                pos->board[rook_to] = PIECE_EMPTY;
            } else {
                int rook_from = SQ_INDEX(0, frank);
                int rook_to   = SQ_INDEX(3, frank);
                pos->board[rook_from] = pos->board[rook_to];
                pos->board[rook_to] = PIECE_EMPTY;
            }
        }
    }
}

static int generate_pseudo_moves(Position *pos, int *from_out, int *to_out, int *promo_out, int capacity)
{
    int n = 0;
    int stm = pos->side_to_move;
    for (int sq = 0; sq < 64; ++sq) {
        int8_t v = pos->board[sq];
        if (v == PIECE_EMPTY) continue;
        int color = v > 0 ? COLOR_WHITE : COLOR_BLACK;
        if (color != stm) continue;
        int abs_v = piece_abs(v);
        int f = file_of(sq), r = rank_of(sq);
        if (abs_v == PIECE_PAWN) {
            int dir = (color == COLOR_WHITE) ? 1 : -1;
            int tr = r + dir;
            if (tr >= 0 && tr <= 7) {
                int tsq = SQ_INDEX(f, tr);
                if (pos->board[tsq] == PIECE_EMPTY) {
                    if ((color == COLOR_WHITE && tr == 7) || (color == COLOR_BLACK && tr == 0)) {
                        int promos[4] = { PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT };
                        for (int pi = 0; pi < 4; ++pi) {
                            if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=promos[pi]; n++; }
                        }
                    } else {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                        if ((color == COLOR_WHITE && r == 1) || (color == COLOR_BLACK && r == 6)) {
                            int tr2 = r + 2*dir;
                            int tsq2 = SQ_INDEX(f, tr2);
                            if (pos->board[tsq2] == PIECE_EMPTY) {
                                if (n < capacity) { from_out[n]=sq; to_out[n]=tsq2; promo_out[n]=0; n++; }
                            }
                        }
                    }
                }
            }
            for (int df = -1; df <= 1; df += 2) {
                int ff = f + df;
                int rr = r + dir;
                if (ff < 0 || ff > 7 || rr < 0 || rr > 7) continue;
                int tsq = SQ_INDEX(ff, rr);
                int8_t target = pos->board[tsq];
                if (target != PIECE_EMPTY && ((target > 0) != (v > 0))) {
                    if ((color == COLOR_WHITE && rr == 7) || (color == COLOR_BLACK && rr == 0)) {
                        int promos[4] = { PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT };
                        for (int pi = 0; pi < 4; ++pi) {
                            if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=promos[pi]; n++; }
                        }
                    } else {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                    }
                }
                if (pos->en_passant != POS_NO_SQUARE && tsq == pos->en_passant) {
                    if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                }
            }
        } else if (abs_v == PIECE_KNIGHT) {
            const int kd[8][2] = {{2,1},{1,2},{-1,2},{-2,1},{-2,-1},{-1,-2},{1,-2},{2,-1}};
            for (int i=0;i<8;++i) {
                int ff=f+kd[i][0], rr=r+kd[i][1];
                if (ff<0||ff>7||rr<0||rr>7) continue;
                int tsq = SQ_INDEX(ff, rr);
                int8_t t = pos->board[tsq];
                if (t == PIECE_EMPTY || ((t>0)!=(v>0))) {
                    if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                }
            }
        } else if (abs_v == PIECE_BISHOP || abs_v == PIECE_ROOK || abs_v == PIECE_QUEEN) {
            int dirs_local[8][2];
            int dir_count = 0;
            if (abs_v == PIECE_BISHOP) { int d[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}}; memcpy(dirs_local,d,sizeof d); dir_count=4; }
            else if (abs_v == PIECE_ROOK) { int d[4][2]={{1,0},{-1,0},{0,1},{0,-1}}; memcpy(dirs_local,d,sizeof d); dir_count=4; }
            else { int d[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}}; memcpy(dirs_local,d,sizeof d); dir_count=8; }
            for (int di=0; di<dir_count; ++di) {
                int df = dirs_local[di][0], dr = dirs_local[di][1];
                int ff = f+df, rr = r+dr;
                while (ff>=0 && ff<=7 && rr>=0 && rr<=7) {
                    int tsq = SQ_INDEX(ff, rr);
                    int8_t t = pos->board[tsq];
                    if (t == PIECE_EMPTY) {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                    } else {
                        if ((t>0)!=(v>0)) {
                            if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                        }
                        break;
                    }
                    ff += df; rr += dr;
                }
            }
        } else if (abs_v == PIECE_KING) {
            for (int df=-1; df<=1; ++df) for (int dr=-1; dr<=1; ++dr) {
                if (df==0 && dr==0) continue;
                int ff=f+df, rr=r+dr;
                if (ff<0||ff>7||rr<0||rr>7) continue;
                int tsq = SQ_INDEX(ff, rr);
                int8_t t = pos->board[tsq];
                if (t == PIECE_EMPTY || ((t>0)!=(v>0))) {
                    if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                }
            }
            if (v > 0) {
                int e1 = SQ_INDEX(4,0), f1 = SQ_INDEX(5,0), g1 = SQ_INDEX(6,0), d1 = SQ_INDEX(3,0), c1 = SQ_INDEX(2,0), b1 = SQ_INDEX(1,0);
                if (pos->castling & CASTLE_WHITE_K) {
                    int rook_from = SQ_INDEX(7,0);
                    if (pos->board[rook_from] == PIECE_ROOK &&
                        pos->board[f1]==PIECE_EMPTY && pos->board[g1]==PIECE_EMPTY) {
                        if (!is_square_attacked(pos, e1, COLOR_BLACK) &&
                            !is_square_attacked(pos, f1, COLOR_BLACK) &&
                            !is_square_attacked(pos, g1, COLOR_BLACK)) {
                            if (n < capacity) { from_out[n]=sq; to_out[n]=g1; promo_out[n]=0; n++; }
                        }
                    }
                }
                if (pos->castling & CASTLE_WHITE_Q) {
                    int rook_from = SQ_INDEX(0,0);
                    if (pos->board[rook_from] == PIECE_ROOK &&
                        pos->board[b1]==PIECE_EMPTY && pos->board[c1]==PIECE_EMPTY && pos->board[d1]==PIECE_EMPTY) {
                        if (!is_square_attacked(pos, e1, COLOR_BLACK) &&
                            !is_square_attacked(pos, d1, COLOR_BLACK) &&
                            !is_square_attacked(pos, c1, COLOR_BLACK)) {
                            if (n < capacity) { from_out[n]=sq; to_out[n]=c1; promo_out[n]=0; n++; }
                        }
                    }
                }
            } else {
                int e8 = SQ_INDEX(4,7), f8 = SQ_INDEX(5,7), g8 = SQ_INDEX(6,7), d8 = SQ_INDEX(3,7), c8 = SQ_INDEX(2,7), b8 = SQ_INDEX(1,7);
                if (pos->castling & CASTLE_BLACK_K) {
                    int rook_from = SQ_INDEX(7,7);
                    if (pos->board[rook_from] == -PIECE_ROOK &&
                        pos->board[f8]==PIECE_EMPTY && pos->board[g8]==PIECE_EMPTY) {
                        if (!is_square_attacked(pos, e8, COLOR_WHITE) &&
                            !is_square_attacked(pos, f8, COLOR_WHITE) &&
                            !is_square_attacked(pos, g8, COLOR_WHITE)) {
                            if (n < capacity) { from_out[n]=sq; to_out[n]=g8; promo_out[n]=0; n++; }
                        }
                    }
                }
                if (pos->castling & CASTLE_BLACK_Q) {
                    int rook_from = SQ_INDEX(0,7);
                    if (pos->board[rook_from] == -PIECE_ROOK &&
                        pos->board[b8]==PIECE_EMPTY && pos->board[c8]==PIECE_EMPTY && pos->board[d8]==PIECE_EMPTY) {
                        if (!is_square_attacked(pos, e8, COLOR_WHITE) &&
                            !is_square_attacked(pos, d8, COLOR_WHITE) &&
                            !is_square_attacked(pos, c8, COLOR_WHITE)) {
                            if (n < capacity) { from_out[n]=sq; to_out[n]=c8; promo_out[n]=0; n++; }
                        }
                    }
                }
            }
        }
    }
    return n;
}

int generate_legal_moves(Position *pos, int *moves_from, int *moves_to, int *promotions, int capacity)
{
    int tmp_cap = 512;
    int *from = (int*)malloc(sizeof(int)*tmp_cap);
    int *to   = (int*)malloc(sizeof(int)*tmp_cap);
    int *prom = (int*)malloc(sizeof(int)*tmp_cap);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return 0; }
    int cnt = generate_pseudo_moves(pos, from, to, prom, tmp_cap);
    int out = 0;
    Undo undo;
    for (int i = 0; i < cnt; ++i) {
        make_move_raw(pos, from[i], to[i], prom[i], &undo);
        int moved_color = (undo.moved_piece > 0) ? COLOR_WHITE : COLOR_BLACK;
        int king_sq = find_king_sq(pos, moved_color);
        int illegal = 0;
        if (king_sq == POS_NO_SQUARE) illegal = 1;
        else {
            int attacker = (moved_color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
            if (is_square_attacked(pos, king_sq, attacker)) illegal = 1;
        }
        unmake_move_raw(pos, &undo);
        if (!illegal) {
            if (out < capacity) {
                moves_from[out] = from[i];
                moves_to[out] = to[i];
                promotions[out] = prom[i];
            }
            out++;
        }
    }
    free(from); free(to); free(prom);
    return out;
}

uint64_t perft(Position *pos, int depth)
{
    if (depth == 0) return 1ULL;
    int max_moves = 512;
    int *from = (int*)malloc(sizeof(int)*max_moves);
    int *to   = (int*)malloc(sizeof(int)*max_moves);
    int *prom = (int*)malloc(sizeof(int)*max_moves);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return 0; }
    int n = generate_legal_moves(pos, from, to, prom, max_moves);
    if (depth == 1) {
        uint64_t nodes = (uint64_t)n;
        free(from); free(to); free(prom);
        return nodes;
    }
    uint64_t nodes = 0;
    Undo undo;
    for (int i = 0; i < n; ++i) {
        make_move_raw(pos, from[i], to[i], prom[i], &undo);
        nodes += perft(pos, depth - 1);
        unmake_move_raw(pos, &undo);
    }
    free(from); free(to); free(prom);
    return nodes;
}

void make_move(Position *pos, int from, int to, int promotion, MoveUndo *undo)
{
    make_move_raw(pos, from, to, promotion, (Undo *)undo);
}

void unmake_move(Position *pos, const MoveUndo *undo)
{
    unmake_move_raw(pos, (const Undo *)undo);
}

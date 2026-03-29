#include "movegen.h"
#include "hash.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

static inline int file_of(int sq) { return SQ_FILE(sq); }
static inline int rank_of(int sq) { return SQ_RANK(sq); }
void sq_to_coord(int sq, char *buf);

void sq_to_coord(int sq, char *buf) {
    buf[0] = 'a' + (sq % 8);
    buf[1] = '1' + (sq / 8);
    buf[2] = '\0';
}

int is_square_attacked(const Position *pos, int sq, int by) {
    int f = sq % 8, r = sq / 8;

    if (by == COLOR_WHITE && r > 0) {
        if (f > 0 && pos->board[sq - 9] == PIECE_PAWN)  return 1;
        if (f < 7 && pos->board[sq - 7] == PIECE_PAWN)  return 1;
    }
    if (by == COLOR_BLACK && r < 7) {
        if (f > 0 && pos->board[sq + 7] == -PIECE_PAWN) return 1;
        if (f < 7 && pos->board[sq + 9] == -PIECE_PAWN) return 1;
    }

    static const int kd[8][2] = {
        { 2, 1}, { 1, 2}, {-1, 2}, {-2, 1},
        {-2,-1}, {-1,-2}, { 1,-2}, { 2,-1}
    };
    for (int i = 0; i < 8; ++i) {
        int tf = f + kd[i][0], tr = r + kd[i][1];
        if (tf < 0 || tf > 7 || tr < 0 || tr > 7) continue;
        int to = tr * 8 + tf;
        int v  = pos->board[to];
        if (by == COLOR_WHITE && v ==  PIECE_KNIGHT) return 1;
        if (by == COLOR_BLACK && v == -PIECE_KNIGHT) return 1;
    }

    static const int kd2[8][2] = {
        { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1},
        { 1, 1}, {-1, 1}, { 1,-1}, {-1,-1}
    };
    for (int i = 0; i < 8; ++i) {
        int tf = f + kd2[i][0], tr = r + kd2[i][1];
        if (tf < 0 || tf > 7 || tr < 0 || tr > 7) continue;
        int to = tr * 8 + tf;
        int v  = pos->board[to];
        if (by == COLOR_WHITE && v ==  PIECE_KING) return 1;
        if (by == COLOR_BLACK && v == -PIECE_KING) return 1;
    }

    static const int dd_file[8] = { 1, -1,  0,  0,  1, -1,  1, -1 };
    static const int dd_rank[8] = { 0,  0,  1, -1,  1, -1, -1,  1 };

    for (int d = 0; d < 8; ++d) {
        int tf = f, tr = r;
        for (;;) {
            tf += dd_file[d];
            tr += dd_rank[d];
            if (tf < 0 || tf > 7 || tr < 0 || tr > 7) break;
            int to    = tr * 8 + tf;
            int v     = pos->board[to];
            if (v == PIECE_EMPTY) continue;
            int abs_v = abs(v);
            if (by == COLOR_WHITE && v > 0) {
                if (d < 4 && abs_v == PIECE_ROOK)   return 1;
                if (d >= 4 && abs_v == PIECE_BISHOP) return 1;
                if (abs_v == PIECE_QUEEN)            return 1;
            }
            if (by == COLOR_BLACK && v < 0) {
                if (d < 4 && abs_v == PIECE_ROOK)   return 1;
                if (d >= 4 && abs_v == PIECE_BISHOP) return 1;
                if (abs_v == PIECE_QUEEN)            return 1;
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
        if (color == COLOR_WHITE && v ==  PIECE_KING) return i;
        if (color == COLOR_BLACK && v == -PIECE_KING) return i;
    }
    return POS_NO_SQUARE;
}

typedef struct {
    int from, to;
    int8_t  moved_piece;
    int8_t  captured_piece;
    uint8_t prev_castling;
    int8_t  prev_en_passant;
    uint16_t prev_halfmove;
    uint32_t prev_fullmove;
    int ep_capture_sq;
    uint64_t prev_hash;
} Undo;

static void make_move_raw(Position *pos, int from, int to, int promotion, Undo *undo)
{
    assert(from >= 0 && from < 64 && to >= 0 && to < 64);

    undo->from            = from;
    undo->to              = to;
    undo->moved_piece     = pos->board[from];
    undo->captured_piece  = pos->board[to];
    undo->prev_castling   = pos->castling;
    undo->prev_en_passant = pos->en_passant;
    undo->prev_halfmove   = pos->halfmove_clock;
    undo->prev_fullmove   = pos->fullmove_number;
    undo->ep_capture_sq   = POS_NO_SQUARE;
    undo->prev_hash       = pos->hash;

    pos->board[to]   = pos->board[from];
    pos->board[from] = PIECE_EMPTY;

    if (promotion != 0) {
        int8_t sign    = (undo->moved_piece > 0) ? 1 : -1;
        pos->board[to] = (int8_t)(sign * promotion);
    }

    if (piece_abs(undo->moved_piece) == PIECE_PAWN || undo->captured_piece != PIECE_EMPTY)
        pos->halfmove_clock = 0;
    else
        pos->halfmove_clock++;

    pos->en_passant = POS_NO_SQUARE;

    if (piece_abs(undo->moved_piece) == PIECE_PAWN) {
        int fr = rank_of(from), tr2 = rank_of(to);
        if (abs(tr2 - fr) == 2) {
            int ep_rank = (fr + tr2) / 2;
            int ep_file = file_of(from);
            pos->en_passant = SQ_INDEX(ep_file, ep_rank);
        }
        if (undo->captured_piece == PIECE_EMPTY && file_of(from) != file_of(to)) {
            if (undo->prev_en_passant != POS_NO_SQUARE && undo->prev_en_passant == to) {
                int dir    = (undo->moved_piece > 0) ? -1 : 1;
                int cap_sq = SQ_INDEX(file_of(to), rank_of(to) + dir);
                if (cap_sq >= 0 && cap_sq < 64) {
                    undo->captured_piece  = pos->board[cap_sq];
                    undo->ep_capture_sq   = cap_sq;
                    pos->board[cap_sq]    = PIECE_EMPTY;
                }
            }
        }
    }

    if (piece_abs(undo->moved_piece) == PIECE_KING) {
        int ffile = file_of(from), tfile = file_of(to), frank = rank_of(from);
        if (abs(tfile - ffile) == 2) {
            if (tfile > ffile) {
                int rf = SQ_INDEX(7, frank), rt = SQ_INDEX(5, frank);
                pos->board[rt] = pos->board[rf];
                pos->board[rf] = PIECE_EMPTY;
            } else {
                int rf = SQ_INDEX(0, frank), rt = SQ_INDEX(3, frank);
                pos->board[rt] = pos->board[rf];
                pos->board[rf] = PIECE_EMPTY;
            }
        }
        if (undo->moved_piece > 0) pos->castling &= ~(CASTLE_WHITE_K | CASTLE_WHITE_Q);
        else                       pos->castling &= ~(CASTLE_BLACK_K | CASTLE_BLACK_Q);
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

    {
        uint64_t h = undo->prev_hash;

        zobrist_xor_piece(&h, undo->from, undo->moved_piece);

        if (undo->ep_capture_sq != POS_NO_SQUARE) {
            if (undo->captured_piece != PIECE_EMPTY)
                zobrist_xor_piece(&h, undo->ep_capture_sq, undo->captured_piece);
        } else if (undo->captured_piece != PIECE_EMPTY) {
            zobrist_xor_piece(&h, undo->to, undo->captured_piece);
        }

        if (promotion != 0) {
            int8_t sign    = (undo->moved_piece > 0) ? 1 : -1;
            int8_t prom_v  = (int8_t)(sign * promotion);
            zobrist_xor_piece(&h, undo->to, prom_v);
        } else {
            if (undo->moved_piece != PIECE_EMPTY)
                zobrist_xor_piece(&h, undo->to, undo->moved_piece);
        }

        if (piece_abs(undo->moved_piece) == PIECE_KING) {
            int ffile = file_of(undo->from), tfile = file_of(undo->to), frank = rank_of(undo->from);
            if (abs(tfile - ffile) == 2) {
                int rook_from, rook_to;
                if (tfile > ffile) { rook_from = SQ_INDEX(7, frank); rook_to = SQ_INDEX(5, frank); }
                else               { rook_from = SQ_INDEX(0, frank); rook_to = SQ_INDEX(3, frank); }
                int8_t rook_val = (undo->moved_piece > 0) ? PIECE_ROOK : -PIECE_ROOK;
                zobrist_xor_piece(&h, rook_from, rook_val);
                zobrist_xor_piece(&h, rook_to,   rook_val);
            }
        }

        if (undo->prev_en_passant != POS_NO_SQUARE) zobrist_xor_ep(&h, undo->prev_en_passant);
        if (pos->en_passant       != POS_NO_SQUARE) zobrist_xor_ep(&h, pos->en_passant);

        zobrist_xor_castle(&h, (int)undo->prev_castling);
        zobrist_xor_castle(&h, (int)pos->castling);
        zobrist_xor_side(&h);

        pos->hash = h;
    }

#ifdef DEBUG
    {
        uint64_t recomputed = position_hash(pos);
        if (recomputed != pos->hash) {
            fprintf(stderr, "hash mismatch after make_move_raw: recomputed=0x%016llx stored=0x%016llx\n",
                    (unsigned long long)recomputed, (unsigned long long)pos->hash);
            abort();
        }
    }
#endif
}

static void unmake_move_raw(Position *pos, const Undo *undo)
{
    pos->side_to_move    = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    pos->fullmove_number = undo->prev_fullmove;
    pos->halfmove_clock  = undo->prev_halfmove;
    pos->castling        = undo->prev_castling;
    pos->en_passant      = undo->prev_en_passant;
    pos->board[undo->from] = undo->moved_piece;

    if (undo->ep_capture_sq != POS_NO_SQUARE) {
        pos->board[undo->to]            = PIECE_EMPTY;
        pos->board[undo->ep_capture_sq] = undo->captured_piece;
    } else {
        pos->board[undo->to] = undo->captured_piece;
    }

    if (piece_abs(undo->moved_piece) == PIECE_KING) {
        int ffile = file_of(undo->from), tfile = file_of(undo->to), frank = rank_of(undo->from);
        if (abs(tfile - ffile) == 2) {
            if (tfile > ffile) {
                int rf = SQ_INDEX(7, frank), rt = SQ_INDEX(5, frank);
                pos->board[rf] = pos->board[rt];
                pos->board[rt] = PIECE_EMPTY;
            } else {
                int rf = SQ_INDEX(0, frank), rt = SQ_INDEX(3, frank);
                pos->board[rf] = pos->board[rt];
                pos->board[rt] = PIECE_EMPTY;
            }
        }
    }

    pos->hash = undo->prev_hash;

#ifdef DEBUG
    {
        uint64_t recomputed = position_hash(pos);
        if (recomputed != pos->hash) {
            fprintf(stderr, "hash mismatch after unmake_move_raw: recomputed=0x%016llx stored=0x%016llx\n",
                    (unsigned long long)recomputed, (unsigned long long)pos->hash);
            abort();
        }
    }
#endif
}


int generate_pseudo_moves(Position *pos, int *from_out, int *to_out, int *promo_out, int capacity)
{
    int n = 0;
    int stm = pos->side_to_move;

    // Directions for sliding pieces: 8 directions (E, W, N, S, NE, NW, SE, SW)
    static const int slide_file[8] = { 1, -1, 0, 0, 1, -1, 1, -1 };
    static const int slide_rank[8] = { 0, 0,  1,-1, 1, -1, -1, 1 };

    for (int sq = 0; sq < 64; ++sq) {
        int8_t v = pos->board[sq];
        if (v == PIECE_EMPTY) continue;
        int color = v > 0 ? COLOR_WHITE : COLOR_BLACK;
        if (color != stm) continue;
        int abs_v = piece_abs(v);
        int f = file_of(sq), r = rank_of(sq);

        // --- Debug: show pieces and squares being processed ---
//        printf("Piece %d at sq=%d (%c%c)\n", v, sq, 'a'+f, '1'+r);

        if (abs_v == PIECE_PAWN) {
            int dir = (color == COLOR_WHITE) ? 1 : -1;
            int tr  = r + dir;

            if (tr >= 0 && tr <= 7) {
                int tsq = SQ_INDEX(f, tr);
                if (pos->board[tsq] == PIECE_EMPTY) {
                    if ((color == COLOR_WHITE && tr == 7) || (color == COLOR_BLACK && tr == 0)) {
                        int promos[4] = { PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT };
                        for (int pi = 0; pi < 4; ++pi)
                            if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=promos[pi]; n++; }
                    } else {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                        if ((color == COLOR_WHITE && r == 1) || (color == COLOR_BLACK && r == 6)) {
                            int tsq2 = SQ_INDEX(f, r + 2*dir);
                            if (pos->board[tsq2] == PIECE_EMPTY)
                                if (n < capacity) { from_out[n]=sq; to_out[n]=tsq2; promo_out[n]=0; n++; }
                        }
                    }
                }
            }
            for (int df = -1; df <= 1; df += 2) {
                int ff = f + df, rr = r + dir;
                if (ff < 0 || ff > 7 || rr < 0 || rr > 7) continue;
                int tsq = SQ_INDEX(ff, rr);
                int8_t target = pos->board[tsq];
                if (target != PIECE_EMPTY && ((target > 0) != (v > 0))) {
                    if ((color == COLOR_WHITE && rr == 7) || (color == COLOR_BLACK && rr == 0)) {
                        int promos[4] = { PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT };
                        for (int pi = 0; pi < 4; ++pi)
                            if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=promos[pi]; n++; }
                    } else {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                    }
                }
                if (pos->en_passant != POS_NO_SQUARE && tsq == pos->en_passant)
                    if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
            }

        } else if (abs_v == PIECE_KNIGHT) {
            static const int knightd[8][2] = {
                { 2, 1}, { 1, 2}, {-1, 2}, {-2, 1},
                {-2,-1}, {-1,-2}, { 1,-2}, { 2,-1}
            };
            for (int i = 0; i < 8; ++i) {
                int ff = f + knightd[i][0], rr = r + knightd[i][1];
                if (ff < 0 || ff > 7 || rr < 0 || rr > 7) continue;
                int tsq = SQ_INDEX(ff, rr);
                int8_t t = pos->board[tsq];
                if (t == PIECE_EMPTY || ((t > 0) != (v > 0)))
                    if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
            }

        } else if (abs_v == PIECE_BISHOP || abs_v == PIECE_ROOK || abs_v == PIECE_QUEEN) {
            int start_dir, end_dir;
            if (abs_v == PIECE_BISHOP) {
                start_dir = 4; end_dir = 8; // diagonals
            } else if (abs_v == PIECE_ROOK) {
                start_dir = 0; end_dir = 4; // straight lines
            } else {
                start_dir = 0; end_dir = 8; // all 8 directions for queen
            }
            for (int d = start_dir; d < end_dir; ++d) {
                int ff = f;
                int rr = r;
                while (1) {
                    ff += slide_file[d];
                    rr += slide_rank[d];
                    if (ff < 0 || ff > 7 || rr < 0 || rr > 7) break;
                    int tsq = SQ_INDEX(ff, rr);
                    int8_t t = pos->board[tsq];
                    if (t == PIECE_EMPTY) {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                    } else {
                        if ((t > 0) != (v > 0))
                            if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
                        break;
                    }
                }
            }
        } else if (abs_v == PIECE_KING) {
            for (int df = -1; df <= 1; ++df) for (int dr = -1; dr <= 1; ++dr) {
                if (df == 0 && dr == 0) continue;
                int ff = f + df, rr = r + dr;
                if (ff < 0 || ff > 7 || rr < 0 || rr > 7) continue;
                int tsq = SQ_INDEX(ff, rr);
                int8_t t = pos->board[tsq];
                if (t == PIECE_EMPTY || ((t > 0) != (v > 0)))
                    if (n < capacity) { from_out[n]=sq; to_out[n]=tsq; promo_out[n]=0; n++; }
            }

            int opp = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;

            if (color == COLOR_WHITE) {
                if (pos->castling & CASTLE_WHITE_K) {
                    if (pos->board[SQ_INDEX(7,0)] == PIECE_ROOK         &&
                        pos->board[SQ_INDEX(5,0)] == PIECE_EMPTY        &&
                        pos->board[SQ_INDEX(6,0)] == PIECE_EMPTY        &&
                        !is_square_attacked(pos, SQ_INDEX(4,0), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(5,0), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(6,0), opp))
                    {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=SQ_INDEX(6,0); promo_out[n]=0; n++; }
                    }
                }
                if (pos->castling & CASTLE_WHITE_Q) {
                    if (pos->board[SQ_INDEX(0,0)] == PIECE_ROOK         &&
                        pos->board[SQ_INDEX(1,0)] == PIECE_EMPTY        &&
                        pos->board[SQ_INDEX(2,0)] == PIECE_EMPTY        &&
                        pos->board[SQ_INDEX(3,0)] == PIECE_EMPTY        &&
                        !is_square_attacked(pos, SQ_INDEX(4,0), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(3,0), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(2,0), opp))
                    {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=SQ_INDEX(2,0); promo_out[n]=0; n++; }
                    }
                }
            } else {
                if (pos->castling & CASTLE_BLACK_K) {
                    if (pos->board[SQ_INDEX(7,7)] == -PIECE_ROOK        &&
                        pos->board[SQ_INDEX(5,7)] == PIECE_EMPTY        &&
                        pos->board[SQ_INDEX(6,7)] == PIECE_EMPTY        &&
                        !is_square_attacked(pos, SQ_INDEX(4,7), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(5,7), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(6,7), opp))
                    {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=SQ_INDEX(6,7); promo_out[n]=0; n++; }
                    }
                }
                if (pos->castling & CASTLE_BLACK_Q) {
                    if (pos->board[SQ_INDEX(0,7)] == -PIECE_ROOK        &&
                        pos->board[SQ_INDEX(1,7)] == PIECE_EMPTY        &&
                        pos->board[SQ_INDEX(2,7)] == PIECE_EMPTY        &&
                        pos->board[SQ_INDEX(3,7)] == PIECE_EMPTY        &&
                        !is_square_attacked(pos, SQ_INDEX(4,7), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(3,7), opp)    &&
                        !is_square_attacked(pos, SQ_INDEX(2,7), opp))
                    {
                        if (n < capacity) { from_out[n]=sq; to_out[n]=SQ_INDEX(2,7); promo_out[n]=0; n++; }
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
    int *from = malloc(sizeof(int) * tmp_cap);
    int *to   = malloc(sizeof(int) * tmp_cap);
    int *prom = malloc(sizeof(int) * tmp_cap);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return 0; }

    int cnt = generate_pseudo_moves(pos, from, to, prom, tmp_cap);
    int nlegal = 0;
    Undo undo;

    for (int i = 0; i < cnt; ++i) {
        make_move_raw(pos, from[i], to[i], prom[i], &undo);

        int just_moved = pos->side_to_move ^ 1;
        int king_sq    = find_king_sq(pos, just_moved);
        int attacker   = pos->side_to_move;

        int is_illegal = 0;
        if (king_sq == POS_NO_SQUARE) {
            is_illegal = 1;
            //printf("[DEBUG] king not found for side %d after move %d->%d\n",
              //     just_moved, from[i], to[i]);
        } else if (is_square_attacked(pos, king_sq, attacker)) {
            is_illegal = 1;
            char fs[4], ts[4];
            sq_to_coord(from[i], fs);
            sq_to_coord(to[i],   ts);
            //printf("[DEBUG] move %s%s leaves king in check (side: %d king_sq: %d)\n",
              //     fs, ts, just_moved, king_sq);
        }

        unmake_move_raw(pos, &undo);

        if (!is_illegal && nlegal < capacity) {
            char fs[4], ts[4];
            sq_to_coord(from[i], fs);
            sq_to_coord(to[i],   ts);
            //printf("[LEGAL] %s%s\n", fs, ts);
            moves_from[nlegal] = from[i];
            moves_to[nlegal]   = to[i];
            promotions[nlegal] = prom[i];
            nlegal++;
        }
    }

    free(from); free(to); free(prom);
    //printf("[LEGAL-MOVE TOTAL] %d\n", nlegal);
    return nlegal;
}

uint64_t perft(Position *pos, int depth)
{
    if (depth == 0) return 1ULL;
    int max_moves = 512;
    int *from = malloc(sizeof(int) * max_moves);
    int *to   = malloc(sizeof(int) * max_moves);
    int *prom = malloc(sizeof(int) * max_moves);
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

void make_null_move(Position *pos, MoveUndo *undo)
{
    undo->side_to_move = pos->side_to_move;
    undo->en_passant   = pos->en_passant;
    pos->side_to_move  = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    pos->en_passant    = POS_NO_SQUARE;
}

void unmake_null_move(Position *pos, const MoveUndo *undo)
{
    pos->side_to_move  = undo->side_to_move;
    pos->en_passant    = undo->en_passant;
}

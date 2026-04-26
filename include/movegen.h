#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "position.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int from, to;
    int8_t moved_piece;
    int8_t captured_piece;
    uint8_t prev_castling;
    int8_t prev_en_passant;
    uint16_t prev_halfmove;
    uint32_t prev_fullmove;
    int ep_capture_sq;
    uint64_t prev_hash;
} MoveUndo;

typedef struct {
    int side_to_move;
    int en_passant;
    uint64_t prev_hash;
} NullMoveUndo;

uint64_t perft(Position *pos, int depth);

int generate_legal_moves(Position *pos, int *moves_from, int *moves_to, int *promotions, int capacity);

int generate_pseudo_moves(Position *pos, int *from_out, int *to_out, int *promo_out, int capacity);

void make_move(Position *pos, int from, int to, int promotion, MoveUndo *undo);
void unmake_move(Position *pos, const MoveUndo *undo);

void make_null_move(Position *pos, NullMoveUndo *undo);
void unmake_null_move(Position *pos, const NullMoveUndo *undo);

bool position_king_in_check(const Position *pos, int color);
bool position_is_square_attacked(const Position *pos, int sq, int attacker_color);

#endif

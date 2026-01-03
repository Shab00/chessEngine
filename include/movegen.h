#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "position.h"
#include <stdint.h>

typedef struct {
    int from, to;
    int8_t moved_piece;
    int8_t captured_piece;
    uint8_t prev_castling;
    int8_t prev_en_passant;
    uint16_t prev_halfmove;
    uint32_t prev_fullmove;
    int ep_capture_sq;
} MoveUndo;

uint64_t perft(Position *pos, int depth);

int generate_legal_moves(Position *pos, int *moves_from, int *moves_to, int *promotions, int capacity);

void make_move(Position *pos, int from, int to, int promotion, MoveUndo *undo);
void unmake_move(Position *pos, const MoveUndo *undo);

#endif 

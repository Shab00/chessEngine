#include <stdio.h>
#include "position.h"
// and/or other includes as needed

void print_board(const Position *pos) {
    printf("Current board:\n");
    for (int rank = 7; rank >= 0; --rank) {
        printf("%d ", rank + 1);
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            int8_t v = pos->board[sq];
            char c = '.';
            if (v == PIECE_EMPTY) c = '.';
            else if (v > 0) {
                switch (v) {
                    case PIECE_PAWN: c='P'; break;
                    case PIECE_KNIGHT: c='N'; break;
                    case PIECE_BISHOP: c='B'; break;
                    case PIECE_ROOK: c='R'; break;
                    case PIECE_QUEEN: c='Q'; break;
                    case PIECE_KING: c='K'; break;
                    default: c='?'; break;
                }
            } else {
                switch (v) {
                    case -PIECE_PAWN: c='p'; break;
                    case -PIECE_KNIGHT: c='n'; break;
                    case -PIECE_BISHOP: c='b'; break;
                    case -PIECE_ROOK: c='r'; break;
                    case -PIECE_QUEEN: c='q'; break;
                    case -PIECE_KING: c='k'; break;
                    default: c='?'; break;
                }
            }
            printf("%c ", c);
        }
        printf("\n");
    }
    printf("  a b c d e f g h\n");
}

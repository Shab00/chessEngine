#include <stdio.h>
#include <string.h>
#include "position.h"

static int positions_equal(const Position *a, const Position *b)
{
    if (a->side_to_move != b->side_to_move) return 0;
    if (a->castling != b->castling) return 0;
    if (a->en_passant != b->en_passant) return 0;
    if (a->halfmove_clock != b->halfmove_clock) return 0;
    if (a->fullmove_number != b->fullmove_number) return 0;
    for (int i = 0; i < 64; ++i) if (a->board[i] != b->board[i]) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    const char *fen = argc > 1 ? argv[1] : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Position p1, p2;
    char err[256];
    char buf[256];

    if (position_from_fen(&p1, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "parse failed: %s\n", err);
        return 2;
    }

    if (position_to_fen(&p1, buf, sizeof buf) != POS_OK) {
        fprintf(stderr, "serialize failed: buffer too small\n");
        return 3;
    }

    if (position_from_fen(&p2, buf, err, sizeof err) != POS_OK) {
        fprintf(stderr, "re-parse failed: %s\n", err);
        return 4;
    }

    if (!positions_equal(&p1, &p2)) {
        fprintf(stderr, "round-trip mismatch\noriginal:  %s\nserialized: %s\n", fen, buf);
        fprintf(stderr, "Original board:\n");
        position_print_ascii(&p1, stderr);
        fprintf(stderr, "Serialized board:\n");
        position_print_ascii(&p2, stderr);
        return 5;
    }

    printf("OK: %s\n", buf);
    return 0;
}

#ifndef CHESS_POSITION_H
#define CHESS_POSITION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POS_NO_SQUARE  (-1)

#define PIECE_EMPTY  0
#define PIECE_PAWN   1
#define PIECE_KNIGHT 2
#define PIECE_BISHOP 3
#define PIECE_ROOK   4
#define PIECE_QUEEN 5
#define PIECE_KING   6

#define COLOR_WHITE 0
#define COLOR_BLACK 1
#define COLOR_NONE  -1

#define CASTLE_WHITE_K (1u << 0)
#define CASTLE_WHITE_Q (1u << 1)
#define CASTLE_BLACK_K (1u << 2)
#define CASTLE_BLACK_Q (1u << 3)

typedef enum {
    POS_OK = 0,
    POS_ERR_BAD_FEN,
    POS_ERR_BUF_SMALL,
    POS_ERR_INVALID_ARG,
    POS_ERR_INVARIANT,
    POS_ERR_OTHER
} pos_error_t;

typedef struct {
    int8_t board[64];
    uint8_t side_to_move;
    uint8_t castling;
    int8_t en_passant;
    uint16_t halfmove_clock;
    uint32_t fullmove_number;
} Position;

#define SQ_INDEX(file, rank)  ((rank) * 8 + (file))
#define SQ_FILE(sq)           ((sq) % 8)
#define SQ_RANK(sq)           ((sq) / 8)

static inline int piece_abs(int8_t v) { return v == 0 ? 0 : (v > 0 ? v : -v); }
static inline int piece_color(int8_t v) { return v > 0 ? COLOR_WHITE : (v < 0 ? COLOR_BLACK : -1); }

void position_init(Position *pos);

pos_error_t position_from_fen(Position *pos, const char *fen,
                              char *errbuf, size_t errbuf_size);

pos_error_t position_to_fen(const Position *pos, char *buf, size_t buf_size);

void position_print_ascii(const Position *pos, FILE *out);

pos_error_t position_validate(const Position *pos, char *errbuf, size_t errbuf_size);

int position_square_from_coords(char file_char, char rank_char);
void position_square_to_coords(int sq, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif

#ifndef CARDS_H
#define CARDS_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define CARD_DECK_SIZE 52
#define CARD_PAR1_COUNT 13
#define CARD_PAR2_COUNT 4

typedef struct {
    int parameter_1;
} GameCard_MathModel;

bool random_cards_deck_round(unsigned int* seed_ptr);

bool is_same(const GameCard_MathModel *card1, const GameCard_MathModel *card2);

#endif
#include "../include/card_model.h"

void random_shuffle(GameCard_MathModel *arr, int n, unsigned int* seed_ptr) {
    for (int i = n - 1; i > 0; --i) {
        int random_ind = rand_r(seed_ptr) % (i + 1);
        GameCard_MathModel tmp = arr[random_ind];
        arr[random_ind] = arr[i];
        arr[i] = tmp;
    }
}

bool is_same(const GameCard_MathModel *card1, const GameCard_MathModel *card2) {
    return card1->parameter_1 == card2->parameter_1;
}

bool random_cards_deck_round(unsigned int* seed_ptr) {
    GameCard_MathModel cards_deck[CARD_DECK_SIZE];
    int ind = 0;
    for (int i = 0; i < CARD_PAR1_COUNT; ++i) {
        for (int j = 0; j < CARD_PAR2_COUNT; ++j) {
            cards_deck[ind] = (GameCard_MathModel){
                .parameter_1 = i
            };
            ++ind;
        }
    }
    random_shuffle(cards_deck, CARD_DECK_SIZE, seed_ptr);
    return is_same(&cards_deck[CARD_DECK_SIZE - 1], &cards_deck[CARD_DECK_SIZE - 2]);
}
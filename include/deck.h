#ifndef DECK_H
#define DECK_H

#include <vector>
#include "card.h"

// Deck system: handles card generation, storage, shuffling, and drawing
class Deck {
private:
    // Draw pile: face-down stack, top is back()
    std::vector<Card> drawPile;

public:
    // Constructor: creates a standard UNO deck and shuffles it
    Deck();

    // Rebuild a full standard UNO deck (108 cards)
    void initialize();

    // Shuffle
    void shuffle();

    // Draw one card; throw an exception if the pile is empty
    Card drawCard();

    // Check whether the draw pile is empty
    bool isEmpty() const;

    // Number of cards remaining in the draw pile
    int size() const;

    // Rebuild the draw pile from the discard pile:
    // - Keep the top discard card
    // - Shuffle all other cards back into the draw pile
    void rebuildFromDiscard(std::vector<Card>& discardPile);
};

#endif // DECK_H

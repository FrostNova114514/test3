#include "deck.h"

#include <algorithm>    // std::shuffle
#include <random>       // std::mt19937, std::random_device
#include <stdexcept>    // std::runtime_error

// ====================== Constructor ======================
Deck::Deck() {
    initialize();
    shuffle();
}

// ====================== Initialize the full deck ======================
//
// Standard UNO deck (without special variants): 108 cards:
// - 4 colors: r, y, g, b
// - For each color:
//   · 1 zero card
//   · 2 each of cards 1-9
//   · 2 Skip cards
//   · 2 Reverse cards
//   · 2 Draw Two cards
// - 4 Wild cards (no color)
// - 4 Wild Draw Four cards (no color)
void Deck::initialize() {
    drawPile.clear();

    const char COLORS[4] = {'r', 'y', 'g', 'b'};
    const char WILD_COLOR = 'x'; // Placeholder for no color

    // Cards for the four colors
    for (int i = 0; i < 4; ++i) {
        char c = COLORS[i];

        // One zero card
        drawPile.emplace_back(c, CardType::Number, 0);

        // Two copies of each card from 1 to 9
        for (int number = 1; number <= 9; ++number) {
            drawPile.emplace_back(c, CardType::Number, number);
            drawPile.emplace_back(c, CardType::Number, number);
        }

        // Two Skip cards
        drawPile.emplace_back(c, CardType::Skip, -1);
        drawPile.emplace_back(c, CardType::Skip, -1);

        // Two Reverse cards
        drawPile.emplace_back(c, CardType::Reverse, -1);
        drawPile.emplace_back(c, CardType::Reverse, -1);

        // Two Draw Two cards
        drawPile.emplace_back(c, CardType::DrawTwo, -1);
        drawPile.emplace_back(c, CardType::DrawTwo, -1);
    }

    // Four Wild cards
    for (int i = 0; i < 4; ++i) {
        drawPile.emplace_back(WILD_COLOR, CardType::Wild, -1);
    }

    // Four Wild Draw Four cards
    for (int i = 0; i < 4; ++i) {
        drawPile.emplace_back(WILD_COLOR, CardType::WildDrawFour, -1);
    }

    // Optional: verify that drawPile.size() == 108
}

// ====================== Shuffle ======================
void Deck::shuffle() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(drawPile.begin(), drawPile.end(), gen);
}

// ====================== Draw a card ======================
Card Deck::drawCard() {
    if (drawPile.empty()) {
        throw std::runtime_error("Deck::drawCard: draw pile is empty.");
    }

    Card card = drawPile.back();
    drawPile.pop_back();
    return card;
}

// ====================== Check whether the pile is empty ======================
bool Deck::isEmpty() const {
    return drawPile.empty();
}

// ====================== Remaining cards in the pile ======================
int Deck::size() const {
    return static_cast<int>(drawPile.size());
}

// ====================== Rebuild the draw pile from the discard pile ======================
//
// Logic:
// 1. The discard pile must contain at least 2 cards, otherwise it cannot be rebuilt.
// 2. Keep the top discard card as topCard.
// 3. Copy the remaining discard cards back into drawPile.
// 4. Clear the discard pile and leave only topCard.
// 5. Shuffle drawPile.
void Deck::rebuildFromDiscard(std::vector<Card>& discardPile) {
    if (discardPile.size() <= 1) {
        throw std::runtime_error("Deck::rebuildFromDiscard: not enough cards in discard pile.");
    }

    // Keep the top card
    Card topCard = discardPile.back();
    discardPile.pop_back();

    // Use the remaining cards as the new draw pile
    drawPile = discardPile;

    // Clear the discard pile, leaving only topCard
    discardPile.clear();
    discardPile.push_back(topCard);

    // Shuffle
    shuffle();
}

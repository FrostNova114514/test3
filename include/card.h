#ifndef CARD_H
#define CARD_H

#include <string>

// UNO card types
enum class CardType {
    Number,         // Number card 0-9
    Skip,           // Skip
    Reverse,        // Reverse
    DrawTwo,        // +2
    Wild,           // Wild
    WildDrawFour    // +4
};

// UNO card class
class Card {
private:
    // Color encoding:
    // 'r' = red, 'y' = yellow, 'g' = green, 'b' = blue, 'x' = no color (Wild)
    char color;

    // Card type
    CardType type;

    // Value:
    // - Number cards: 0-9
    // - Action cards: always use -1 (value not used)
    int value;

public:
    // Default constructor (needed for containers)
    Card();

    // Standard constructor
    Card(char color, CardType type, int value);

    // Accessors
    char getColor() const;
    CardType getType() const;
    int getValue() const;

    void setColor(char c);  // Used after a Wild card is assigned a color

    // Check whether this card can be played in the current state
    // Parameters:
    //  - topCard: current discard-pile top card
    //  - currentColor: active color maintained by Game
    bool isPlayable(const Card& topCard, char currentColor) const;

    // Debug / UI output
    std::string toString() const;
};

#endif // CARD_H

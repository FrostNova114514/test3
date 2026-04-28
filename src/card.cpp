#include "card.h"

#include <sstream>

// ========== Constructors ==========

Card::Card()
    : color('x'), type(CardType::Number), value(-1) {}

Card::Card(char color, CardType type, int value)
    : color(color), type(type), value(value) {}

// ========== Getters / Setters ==========

char Card::getColor() const {
    return color;
}

CardType Card::getType() const {
    return type;
}

int Card::getValue() const {
    return value;
}

void Card::setColor(char c) {
    color = c;
}

// ========== Playability Check ==========
//
// Simplified UNO rules:
// 1. Wild / WildDrawFour can always be played.
// 2. A card can be played if its color matches currentColor.
// 3. A card can be played if its type matches the top card.
// 4. Two number cards can be played if their values match.
// (The strict Wild Draw Four restriction is not implemented in Phase 1.)
bool Card::isPlayable(const Card& topCard, char currentColor) const {
    // Wild / Wild Draw Four can always be played (Phase 1 simplification)
    if (type == CardType::Wild || type == CardType::WildDrawFour) {
        return true;
    }

    // Match the current active color
    if (color == currentColor) {
        return true;
    }

    // Match the top card type
    if (type == topCard.getType()) {
        return true;
    }

    // Match the top card value
    if (type == CardType::Number &&
        topCard.getType() == CardType::Number &&
        value == topCard.getValue()) {
        return true;
    }

    return false;
}

// ========== toString: Debug / UI Display ==========

std::string Card::toString() const {
    std::ostringstream oss;

    // Color
    switch (color) {
        case 'r': oss << "R"; break;
        case 'y': oss << "Y"; break;
        case 'g': oss << "G"; break;
        case 'b': oss << "B"; break;
        case 'x': oss << "X"; break; // Wild / no color
        default:  oss << "?"; break;
    }

    oss << "-";

    // Type + value
    switch (type) {
        case CardType::Number:
            oss << value;
            break;
        case CardType::Skip:
            oss << "Skip";
            break;
        case CardType::Reverse:
            oss << "Rev";
            break;
        case CardType::DrawTwo:
            oss << "+2";
            break;
        case CardType::Wild:
            oss << "Wild";
            break;
        case CardType::WildDrawFour:
            oss << "Wild+4";
            break;
    }

    return oss.str();
}

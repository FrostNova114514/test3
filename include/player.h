#ifndef PLAYER_H // include guard start
#define PLAYER_H // include guard define

#include <vector> // use std::vector for hands
#include "card.h" // use Card type

struct GameState; // forward declaration for turn decisions

class Player { // base player class
protected: // protected section for derived classes
    std::vector<Card> hand; // stored cards in hand

public: // public interface
    Player(); // default constructor
    virtual ~Player(); // virtual destructor for polymorphism

    void addCard(const Card& card); // add a card to hand
    bool removeCard(const Card& card); // remove a matching card from hand
    int getCardCount() const; // return number of cards in hand
    const std::vector<Card>& getHand() const; // read-only access to hand
    bool hasPlayableCard(const Card& topCard, char currentColor) const; // check for any playable card

    virtual Card takeTurn(const GameState& state) = 0; // choose a card to play or draw
    virtual char chooseColor() = 0; // choose a color after wild cards
}; // end base player class

class HumanPlayer : public Player { // human-controlled player
public: // public interface
    HumanPlayer(); // default constructor
    ~HumanPlayer() override; // destructor override

    Card takeTurn(const GameState& state) override; // ask user for a move
    char chooseColor() override; // ask user for a color
}; // end human player class

#endif // PLAYER_H // include guard end

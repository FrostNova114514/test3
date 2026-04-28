#ifndef UI_H
#define UI_H

#include <string>
#include <vector>

#include "card.h"
#include "game.h"

class UI {
public:
    // showWelcome
    // Purpose: Display the welcome message and basic control instructions of the game.
    // Input: None.
    // Output: None. The function prints text to the terminal.
    void showWelcome() const;
    void showProjectIntro() const;

    // showGameState
    // Purpose: Display the current game state, including player turn, top card, color, direction, and card counts.
    // Input: state - the current snapshot of the game state.
    // Output: None. The function prints formatted game information to the terminal.
    void showGameState(const GameState& state) const;

    // showHand
    // Purpose: Display the human player's hand and mark cards that are playable in the current turn.
    // Input: hand - the list of cards in the player's hand; state - the current snapshot of the game state.
    // Output: None. The function prints the player's cards to the terminal.
    void showHand(const std::vector<Card>& hand, const GameState& state) const;
    void showHandSelectable(const std::vector<Card>& hand, const GameState& state, int selectedIndex, bool includeDrawOption) const;

    // getPlayerInput
    // Purpose: Ask the human player to choose a card index to play or choose to draw a card.
    // Input: handSize - the number of cards currently in the player's hand.
    // Output: Returns an integer representing the selected card index, or -1 if the player chooses to draw.
    int getPlayerInput(int handSize) const;

    // getColorChoice
    // Purpose: Ask the human player to choose a color after playing a Wild or Wild Draw Four card.
    // Input: None.
    // Output: Returns a lowercase character ('r', 'y', 'g', or 'b') representing the selected color.
    char getColorChoice() const;

    // showDrawMessage
    // Purpose: Display the card just drawn by the player.
    // Input: card - the card drawn from the deck.
    // Output: None. The function prints a message to the terminal.
    void showDrawMessage(const Card& card) const;

    // showInvalidMove
    // Purpose: Inform the player that the selected card cannot be played under the current game state.
    // Input: card - the card selected by the player; state - the current snapshot of the game state.
    // Output: None. The function prints an error message to the terminal.
    void showInvalidMove(const Card& card, const GameState& state) const;

    // showActionMessage
    // Purpose: Display a general action or status message.
    // Input: message - the message to be displayed.
    // Output: None. The function prints the message to the terminal.
    void showActionMessage(const std::string& message) const;

    // showWinner
    // Purpose: Display the winner of the game.
    // Input: playerIndex - the index of the winning player.
    // Output: None. The function prints the winner information to the terminal.
    void showWinner(int playerIndex) const;
    void runRuleWalkthrough() const;


private:
    // colorName
    // Purpose: Convert the internal color code of a card into a readable string.
    // Input: color - a character representing a card color.
    // Output: Returns the corresponding color name as a string.
    std::string colorName(char color) const;

    // directionName
    // Purpose: Convert the turn direction integer into a readable description.
    // Input: direction - the current direction of play.
    // Output: Returns a string describing the direction of play.
    std::string directionName(int direction) const;

    // weatherName
    // Purpose: Convert the internal weather state code into a readable string.
    // Input: weather - an integer representing the weather state.
    // Output: Returns the corresponding weather description as a string.
    std::string weatherName(int weather) const;
};

#endif

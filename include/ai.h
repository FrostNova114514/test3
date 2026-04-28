#ifndef AI_H
#define AI_H

#include "card.h"
#include "player.h"
#include "game.h"
#include <vector>
#include <string>

// ============================================================
// AI Difficulty Levels
// ============================================================
// Easy   - Random valid card selection
// Medium - Prioritizes playing action cards; minimizes hand size
// Hard   - Strategic: color control, tracks opponents, weather-aware
// ============================================================
enum class AIDifficulty {
    Easy,
    Medium,
    Hard
};

// ============================================================
// AIPlayer - Lv1 Standard Uno AI
// Inherits from Player (defined in player.h)
// Handles all decision-making for AI-controlled players
// ============================================================
class AIPlayer : public Player {
protected:
    AIDifficulty difficulty; // Difficulty level of this AI instance
    std::string name;        // Display name (e.g., "AI-1")

    // ---- Internal Helper Methods ----

    // Returns all playable cards from hand given the top card and current color
    // Input:  topCard - top of discard pile, currentColor - active color
    // Output: vector of playable Card objects
    std::vector<Card> getPlayableCards(const Card& topCard, char currentColor) const;

    // Selects the best color to call after playing a Wild card
    // Input:  none (analyzes own hand)
    // Output: char color ('r','y','g','b')
    char pickBestColor() const;

    // Easy AI: Randomly picks a card from playable options
    // Input:  playable - list of valid cards to play
    // Output: chosen Card
    Card easyPick(const std::vector<Card>& playable) const;

    // Medium AI: Prefers action cards; picks color-matching cards second
    // Input:  playable - list of valid cards, currentColor - active color
    // Output: chosen Card
    Card mediumPick(const std::vector<Card>& playable, char currentColor) const;

    // Hard AI: Strategic pick considering hand size, color dominance, and opponents
    // Input:  playable - list of valid cards, state - full game snapshot
    // Output: chosen Card
    Card hardPick(const std::vector<Card>& playable, const GameState& state) const;

public:
    // Constructor: creates AI with given difficulty and name
    // Input:  diff - AIDifficulty level, playerName - display name
    AIPlayer(AIDifficulty diff = AIDifficulty::Easy, const std::string& playerName = "AI");

    // Core turn function: decides which card to play based on difficulty
    // Input:  state - current GameState snapshot from Game::getSnapshot()
    // Output: Card to play; returns dummy card (value=-1) to signal "draw a card"
    Card takeTurn(const GameState& state) override;

    // Color selection after playing a Wild card
    // Input:  none
    // Output: char representing chosen color ('r','y','g','b')
    char chooseColor() override;

    // Returns display name of this AI player
    std::string getName() const;
};


// ============================================================
// AIPlayerFlip - Lv2 Uno Flip AI
// Extends AIPlayer with dark-side card awareness
// NOTE: Dark side CardTypes should be added to card.h by teammate
//       Placeholder hooks are provided below for integration
// ============================================================
class AIPlayerFlip : public AIPlayer {
protected:
    // Dark-side aware card picker (Medium/Hard only)
    // Input:  playable - valid cards, state - game snapshot
    // Output: chosen Card
    Card flipPick(const std::vector<Card>& playable, const GameState& state) const;

    // Returns true if the game is currently on the dark side
    // Input:  state - current GameState
    // Output: bool
    bool isDarkSide(const GameState& state) const;

public:
    // Constructor
    // Input:  diff - AIDifficulty, playerName - display name
    AIPlayerFlip(AIDifficulty diff = AIDifficulty::Easy, const std::string& playerName = "AI-Flip");

    // Overrides takeTurn to handle Flip-specific card logic
    // Input:  state - current GameState snapshot
    // Output: Card to play; dummy card to draw
    Card takeTurn(const GameState& state) override;

    // Color selection - on dark side, only dark colors are valid
    // Input:  none (uses isDarkSide state internally via last known state)
    // Output: char color
    char chooseColor() override;
};


// ============================================================
// AIPlayerWeather - Lv3 Weather + Special Conditions AI
// Extends AIPlayerFlip with weather-adaptive strategy
//
// Weather States (from GameState::currentWeather):
//   0 = Sunny   → Normal play
//   1 = Rainy   → All draws +1 → AI should avoid drawing (play aggressively)
//   2 = Typhoon → Random player gets +1 card each round → AI tracks hand sizes
// ============================================================
class AIPlayerWeather : public AIPlayerFlip {
private:
    // Adjusts strategy based on current weather state
    // Input:  playable - valid cards, state - full game snapshot
    // Output: chosen Card (weather-optimized)
    Card weatherPick(const std::vector<Card>& playable, const GameState& state) const;

    // Returns true if AI should play aggressively (avoid drawing)
    // Used during Rainy weather
    // Input:  state - current GameState
    // Output: bool
    bool shouldAvoidDrawing(const GameState& state) const;

    // Finds the opponent index with the fewest cards (threat detection)
    // Used during Typhoon to target dangerous opponents
    // Input:  state - current GameState
    // Output: int index of most dangerous opponent (0-3)
    int findMostDangerousOpponent(const GameState& state) const;

public:
    // Constructor
    // Input:  diff - AIDifficulty, playerName - display name
    AIPlayerWeather(AIDifficulty diff = AIDifficulty::Easy, const std::string& playerName = "AI-Weather");

    // Overrides takeTurn with full weather-aware decision logic
    // Input:  state - current GameState snapshot
    // Output: Card to play; dummy card to draw
    Card takeTurn(const GameState& state) override;

    // Color selection remains weather-aware
    // Input:  none
    // Output: char color
    char chooseColor() override;
};

#endif // AI_H

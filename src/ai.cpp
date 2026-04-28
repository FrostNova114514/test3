#include "ai.h"
#include "card.h"
#include "game.h"
#include <vector>
#include <algorithm>
#include <cstdlib>   // rand()
#include <climits>   // INT_MAX
#include <map>

// ============================================================
// AIPlayer — Lv1 Standard Uno AI
// ============================================================

// Constructor: sets difficulty and display name
// Input:  diff - AIDifficulty, playerName - display name string
// Output: initialized AIPlayer object
AIPlayer::AIPlayer(AIDifficulty diff, const std::string& playerName)
    : difficulty(diff), name(playerName) {}

// Returns display name
// Input:  none
// Output: name string
std::string AIPlayer::getName() const {
    return name;
}

// Scans hand and returns all cards legally playable right now
// Input:  topCard - top of discard pile, currentColor - active color char
// Output: vector of playable Card objects (may be empty)
std::vector<Card> AIPlayer::getPlayableCards(const Card& topCard, char currentColor) const {
    std::vector<Card> playable;
    for (const Card& card : hand) {  // 'hand' inherited from Player
        if (card.isPlayable(topCard, currentColor)) {
            playable.push_back(card);
        }
    }
    return playable;
}

// Determines the best color to declare after playing a Wild card
// Strategy: pick the color that appears most in hand
// Input:  none (reads own hand)
// Output: char ('r','y','g','b') — defaults to 'r' if hand is empty
char AIPlayer::pickBestColor() const {
    std::map<char, int> colorCount;
    colorCount['r'] = 0;
    colorCount['y'] = 0;
    colorCount['g'] = 0;
    colorCount['b'] = 0;

    for (const Card& card : hand) {
        char c = card.getColor();
        if (c == 'r' || c == 'y' || c == 'g' || c == 'b') {
            colorCount[c]++;
        }
    }

    // Find color with highest count
    char best = 'r';
    int maxCount = -1;
    for (auto& pair : colorCount) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            best = pair.first;
        }
    }
    return best;
}

// Easy AI: Randomly selects a card from the playable list
// Input:  playable - non-empty list of valid cards
// Output: randomly chosen Card
Card AIPlayer::easyPick(const std::vector<Card>& playable) const {
    int idx = rand() % playable.size();
    return playable[idx];
}

// Medium AI: Prefers action/Wild cards over number cards to disrupt opponents
// Priority order: WildDrawFour > Wild > DrawTwo > Skip > Reverse > Number
// Input:  playable - non-empty list of valid cards, currentColor - active color
// Output: highest-priority Card
Card AIPlayer::mediumPick(const std::vector<Card>& playable, char currentColor) const {
    // Score each card type (higher = preferred)
    auto score = [](CardType t) -> int {
        switch (t) {
            case CardType::WildDrawFour: return 6;
            case CardType::Wild:         return 5;
            case CardType::DrawTwo:      return 4;
            case CardType::Skip:         return 3;
            case CardType::Reverse:      return 2;
            case CardType::Number:       return 1;
            default:                     return 0;
        }
    };

    Card best = playable[0];
    int bestScore = score(best.getType());

    for (const Card& card : playable) {
        int s = score(card.getType());
        // Among same score, prefer cards matching current color (smooth flow)
        if (s > bestScore || (s == bestScore && card.getColor() == currentColor)) {
            best = card;
            bestScore = s;
        }
    }
    return best;
}

// Hard AI: Full strategic decision-making
// Strategy:
//  1. If an opponent has 1 card (UNO), prioritize Skip/Reverse/DrawTwo to block them
//  2. Prefer WildDrawFour when opponents are close to winning
//  3. Otherwise, preserve Wilds and play color-matching action cards
//  4. Pick color based on dominant color in hand
// Input:  playable - non-empty list of valid cards, state - full GameState
// Output: strategically chosen Card
Card AIPlayer::hardPick(const std::vector<Card>& playable, const GameState& state) const {
    int myIndex = state.currentPlayerIndex;

    // Check if any opponent is close to winning (1-2 cards left)
    bool opponentNearWin = false;
    for (int i = 0; i < 4; ++i) {
        if (i != myIndex && state.playerCardCounts[i] <= 2) {
            opponentNearWin = true;
            break;
        }
    }

    // Priority 1: Block opponent near win with disruptive cards
    if (opponentNearWin) {
        for (const Card& card : playable) {
            CardType t = card.getType();
            if (t == CardType::WildDrawFour || t == CardType::DrawTwo ||
                t == CardType::Skip || t == CardType::Reverse) {
                return card;
            }
        }
    }

    // Priority 2: Play non-Wild action cards to shrink hand
    for (const Card& card : playable) {
        CardType t = card.getType();
        if (t == CardType::DrawTwo || t == CardType::Skip || t == CardType::Reverse) {
            return card;
        }
    }

    // Priority 3: Play color-matching number cards to preserve Wilds
    for (const Card& card : playable) {
        if (card.getType() == CardType::Number && card.getColor() == state.currentColor) {
            return card;
        }
    }

    // Priority 4: Play any number card
    for (const Card& card : playable) {
        if (card.getType() == CardType::Number) {
            return card;
        }
    }

    // Priority 5: Use Wild as last resort
    for (const Card& card : playable) {
        if (card.getType() == CardType::Wild || card.getType() == CardType::WildDrawFour) {
            return card;
        }
    }

    // Fallback: play first available card
    return playable[0];
}

// Main turn function: dispatches to correct difficulty picker
// Input:  state - GameState snapshot from Game::getSnapshot()
// Output: Card to play; dummy Card (value=-1) signals "draw a card"
Card AIPlayer::takeTurn(const GameState& state) {
    std::vector<Card> playable = getPlayableCards(state.topCard, state.currentColor);

    // No playable cards → signal draw by returning dummy card
    if (playable.empty()) {
        return Card(); // Default constructor: color='x', value=-1
    }

    switch (difficulty) {
        case AIDifficulty::Easy:
            return easyPick(playable);
        case AIDifficulty::Medium:
            return mediumPick(playable, state.currentColor);
        case AIDifficulty::Hard:
            return hardPick(playable, state);
        default:
            return easyPick(playable);
    }
}

// Selects color after playing a Wild card
// Input:  none
// Output: char color based on hand analysis
char AIPlayer::chooseColor() {
    return pickBestColor();
}


// ============================================================
// AIPlayerFlip — Lv2 Uno Flip AI
// ============================================================
// NOTE TO TEAMMATE: When you add dark-side CardTypes to card.h
// (e.g., DrawFive, SkipEveryone, FlipCard), add handling in
// flipPick() below where marked with TODO comments.

// Constructor
// Input:  diff - AIDifficulty, playerName - display name
AIPlayerFlip::AIPlayerFlip(AIDifficulty diff, const std::string& playerName)
    : AIPlayer(diff, playerName) {}

// Returns true if currently on the dark side of the deck
// Input:  state - current GameState (uses isDarkSide field)
// Output: bool
bool AIPlayerFlip::isDarkSide(const GameState& state) const {
    return state.isDarkSide;
}

// Flip-aware card picker: adjusts strategy based on light vs dark side
// Input:  playable - valid cards to play, state - full game snapshot
// Output: best Card for current side
Card AIPlayerFlip::flipPick(const std::vector<Card>& playable, const GameState& state) const {
    if (!isDarkSide(state)) {
        // Light side: use standard medium/hard logic
        if (difficulty == AIDifficulty::Hard) {
            return hardPick(playable, state);
        }
        return mediumPick(playable, state.currentColor);
    }

    // ---- Dark Side Strategy ----
    // TODO (teammate): When dark-side CardTypes are added to card.h, insert
    // priority logic here. Example structure:
    //
    // Priority 1: Dark DrawFive to punish opponents (very powerful)
    // for (const Card& card : playable) {
    //     if (card.getType() == CardType::DrawFive) return card;
    // }
    //
    // Priority 2: SkipEveryone to take consecutive turns
    // for (const Card& card : playable) {
    //     if (card.getType() == CardType::SkipEveryone) return card;
    // }
    //
    // Priority 3: Flip card to switch back to light side if hand is weak
    // for (const Card& card : playable) {
    //     if (card.getType() == CardType::Flip) return card;
    // }

    // Fallback: use base medium logic until dark cards are defined
    return mediumPick(playable, state.currentColor);
}

// Main turn for Flip AI: routes to flipPick
// Input:  state - GameState snapshot
// Output: Card to play; dummy Card to draw
Card AIPlayerFlip::takeTurn(const GameState& state) {
    std::vector<Card> playable = getPlayableCards(state.topCard, state.currentColor);

    if (playable.empty()) {
        return Card(); // Dummy card → signal draw
    }

    if (difficulty == AIDifficulty::Easy) {
        return easyPick(playable);
    }

    return flipPick(playable, state);
}

// Color selection for Flip AI
// Light side: standard color picking
// Dark side: TODO — restrict to dark colors when teammate defines them
// Input:  none
// Output: char color
char AIPlayerFlip::chooseColor() {
    // TODO (teammate): When dark side colors are defined (e.g., 'p','o','t','m'),
    // add a check here: if (lastKnownState.isDarkSide) return pickBestDarkColor();
    return pickBestColor(); // Falls back to light-side color logic
}


// ============================================================
// AIPlayerWeather — Lv3 Weather + Special Conditions AI
// ============================================================

// Constructor
// Input:  diff - AIDifficulty, playerName - display name
AIPlayerWeather::AIPlayerWeather(AIDifficulty diff, const std::string& playerName)
    : AIPlayerFlip(diff, playerName) {}

// Returns true when Rainy weather makes drawing extra costly (+1 card)
// AI should be more aggressive and avoid drawing if possible
// Input:  state - GameState (currentWeather == 1 means Rainy)
// Output: bool
bool AIPlayerWeather::shouldAvoidDrawing(const GameState& state) const {
    return state.currentWeather == 1; // 1 = Rainy
}

// Finds the opponent with the fewest cards (most dangerous — closest to winning)
// Used in Typhoon to identify who might be targeted / who is a threat
// Input:  state - GameState with playerCardCounts[]
// Output: int index of the most dangerous opponent (not self)
int AIPlayerWeather::findMostDangerousOpponent(const GameState& state) const {
    int myIndex = state.currentPlayerIndex;
    int minCards = INT_MAX;
    int dangerousIdx = -1;

    for (int i = 0; i < 4; ++i) {
        if (i == myIndex) continue;
        if (state.playerCardCounts[i] < minCards) {
            minCards = state.playerCardCounts[i];
            dangerousIdx = i;
        }
    }
    return dangerousIdx;
}

// Weather-adaptive card picker: adjusts aggression based on weather
// Input:  playable - valid cards, state - full GameState with weather info
// Output: weather-optimized Card choice
Card AIPlayerWeather::weatherPick(const std::vector<Card>& playable, const GameState& state) const {
    int weather = state.currentWeather;

    // ---- Sunny (0): Normal play — use standard Flip logic ----
    if (weather == 0) {
        if (difficulty == AIDifficulty::Easy) return easyPick(playable);
        return flipPick(playable, state);
    }

    // ---- Rainy (1): Drawing costs +1 extra card → play aggressively ----
    // Force a play at all costs; prefer any card over drawing
    if (weather == 1) {
        // Try to play something — anything — to avoid the costly draw
        // Hard/Medium: still prefer action cards
        if (difficulty == AIDifficulty::Hard || difficulty == AIDifficulty::Medium) {
            // Prioritize action cards first
            for (const Card& card : playable) {
                CardType t = card.getType();
                if (t == CardType::DrawTwo || t == CardType::Skip || t == CardType::Reverse) {
                    return card;
                }
            }
            // Then any color match
            for (const Card& card : playable) {
                if (card.getColor() == state.currentColor) return card;
            }
        }
        // Easy: just pick something — anything to avoid drawing
        return playable[0];
    }

    // ---- Typhoon (2): Random player gets +1 card (uncontrollable) ----
    // Strategy: identify the most dangerous opponent; try to target them
    // with Skip/Reverse to disrupt their flow
    if (weather == 2) {
        int dangerous = findMostDangerousOpponent(state);
        int myIndex = state.currentPlayerIndex;

        // If the most dangerous player is next in line, try to skip/block them
        int nextPlayer = (myIndex + state.direction + 4) % 4;
        if (dangerous == nextPlayer) {
            for (const Card& card : playable) {
                CardType t = card.getType();
                if (t == CardType::Skip || t == CardType::DrawTwo || t == CardType::WildDrawFour) {
                    return card;
                }
            }
        }

        // Otherwise play normally
        if (difficulty == AIDifficulty::Easy) return easyPick(playable);
        return flipPick(playable, state);
    }

    // Fallback for any undefined weather state
    return flipPick(playable, state);
}

// Main turn for Weather AI: routes to weatherPick
// Input:  state - GameState snapshot with currentWeather
// Output: Card to play; dummy Card to draw
Card AIPlayerWeather::takeTurn(const GameState& state) {
    std::vector<Card> playable = getPlayableCards(state.topCard, state.currentColor);

    // No playable cards → must draw
    // In Rainy weather, drawing costs +1 but it's unavoidable
    if (playable.empty()) {
        return Card(); // Dummy card → signal draw to game engine
    }

    return weatherPick(playable, state);
}

// Color selection for Weather AI
// Rainy: pick most dominant color to maintain advantage
// Others: standard color logic
// Input:  none
// Output: char color
char AIPlayerWeather::chooseColor() {
    // In all weather conditions, dominant-color strategy is best
    return pickBestColor();
}

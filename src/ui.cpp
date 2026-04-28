#include "ui.h"
#include "ansi.h"
  
#include <iostream>
#include <cctype>
#include <limits>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>

#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
  
using namespace std;

namespace {
struct TermSize {
    int rows = 24;
    int cols = 80;
};

TermSize getTermSize() {
    TermSize ts;
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_row > 0) ts.rows = ws.ws_row;
        if (ws.ws_col > 0) ts.cols = ws.ws_col;
    }
    return ts;
}

void clearScreen() {
    cout << "\033[2J\033[H";
}

void hideCursor(bool hide) {
    cout << (hide ? "\033[?25l" : "\033[?25h");
}

string repeat(char ch, int count) {
    if (count <= 0) return "";
    return string(static_cast<size_t>(count), ch);
}

string padCenter(const string& s, int width) {
    if (width <= 0) return "";
    if (static_cast<int>(s.size()) >= width) return s.substr(0, static_cast<size_t>(width));
    int left = (width - static_cast<int>(s.size())) / 2;
    int right = width - static_cast<int>(s.size()) - left;
    return repeat(' ', left) + s + repeat(' ', right);
}

constexpr const char* ANSI_RESET = "\033[0m";
constexpr const char* ANSI_BOLD = "\033[1m";
constexpr const char* ANSI_DIM = "\033[2m";

// Raw key input
class RawMode {
public:
    RawMode() : ok(false) {
        if (tcgetattr(STDIN_FILENO, &old) != 0) return;
        termios raw = old;
        raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return;
        ok = true;
    }
    ~RawMode() {
        if (ok) {
            tcsetattr(STDIN_FILENO, TCSANOW, &old);
        }
    }
    RawMode(const RawMode&) = delete;
    RawMode& operator=(const RawMode&) = delete;
private:
    termios old{};
    bool ok;
};

enum class WalkKey { Left, Right, Skip, Other };

WalkKey readWalkKey() {
    unsigned char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) != 1) return WalkKey::Other;
    if (ch == 'q' || ch == 'Q') return WalkKey::Skip;
    if (ch == 27) { // ESC
        unsigned char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return WalkKey::Other;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return WalkKey::Other;
        // Arrow keys: ESC [ C (right), ESC [ D (left)
        if (seq[0] == '[' && (seq[1] == 'C')) return WalkKey::Right;
        if (seq[0] == '[' && (seq[1] == 'D')) return WalkKey::Left;
        return WalkKey::Other;
    }
    return WalkKey::Other;
}

void renderBoxedPage(const string& title, const vector<string>& lines, const string& hint) {
    TermSize ts = getTermSize();
    int maxContentWidth = std::max(40, ts.cols - 6);
    int contentWidth = std::min(92, maxContentWidth);

    vector<string> clipped;
    clipped.reserve(lines.size());
    for (const auto& ln : lines) {
        if (static_cast<int>(ln.size()) <= contentWidth - 4) clipped.push_back(ln);
        else clipped.push_back(ln.substr(0, static_cast<size_t>(contentWidth - 7)) + "...");
    }

    int innerWidth = contentWidth;
    string top = "+" + repeat('-', innerWidth) + "+";
    string mid = "+" + repeat('-', innerWidth) + "+";
    string bot = "+" + repeat('-', innerWidth) + "+";

    int boxHeight = static_cast<int>(clipped.size()) + 6;
    int startRow = std::max(1, (ts.rows - boxHeight) / 2);

    clearScreen();
    for (int i = 0; i < startRow; ++i) cout << "\n";

    int leftPad = std::max(0, (ts.cols - (innerWidth + 2)) / 2);
    auto printLine = [&](const string& s) {
        cout << repeat(' ', leftPad) << s << "\n";
    };

    printLine(top);
    printLine("|" + padCenter(string(ANSI_BOLD) + title + ANSI_RESET, innerWidth) + "|");
    printLine(mid);
    for (const auto& ln : clipped) {
        string padded = ln;
        if (static_cast<int>(padded.size()) < innerWidth) padded += repeat(' ', innerWidth - static_cast<int>(padded.size()));
        printLine("|" + padded + "|");
    }
    // spacer
    printLine("|" + repeat(' ', innerWidth) + "|");
    string hintLine = string(ANSI_DIM) + hint + ANSI_RESET;
    if (static_cast<int>(hintLine.size()) > innerWidth) hintLine = hintLine.substr(0, static_cast<size_t>(innerWidth));
    if (static_cast<int>(hintLine.size()) < innerWidth) hintLine += repeat(' ', innerWidth - static_cast<int>(hintLine.size()));
    printLine("|" + hintLine + "|");
    printLine(bot);
    cout.flush();
}

void renderFullScreenBox(const string& title, const vector<string>& lines, const string& hint) {
    TermSize ts = getTermSize();
    int innerWidth = std::max(40, ts.cols - 4);
    int boxWidth = innerWidth + 2;
    int innerHeight = std::max(8, ts.rows - 4);
    int boxHeight = innerHeight + 2;

    vector<string> clipped;
    clipped.reserve(lines.size());
    for (const auto& ln : lines) {
        if (static_cast<int>(ln.size()) <= innerWidth) clipped.push_back(ln);
        else clipped.push_back(ln.substr(0, static_cast<size_t>(innerWidth - 3)) + "...");
    }

    clearScreen();
    int leftPad = std::max(0, (ts.cols - boxWidth) / 2);
    int topPad = std::max(0, (ts.rows - boxHeight) / 2);
    cout << repeat('\n', topPad);

    auto printLine = [&](const string& s) { cout << repeat(' ', leftPad) << s << "\n"; };

    string top = "+" + repeat('-', innerWidth) + "+";
    printLine(top);
    printLine("|" + padCenter(string(ANSI_BOLD) + title + ANSI_RESET, innerWidth) + "|");
    printLine("+" + repeat('-', innerWidth) + "+");

    int usable = innerHeight - 3; // leave room for hint line and one spacer
    int written = 0;
    for (const auto& ln : clipped) {
        if (written >= usable) break;
        string padded = ln;
        if (static_cast<int>(padded.size()) < innerWidth) padded += repeat(' ', innerWidth - static_cast<int>(padded.size()));
        printLine("|" + padded + "|");
        written++;
    }
    while (written < usable) {
        printLine("|" + repeat(' ', innerWidth) + "|");
        written++;
    }

    string hintLine = string(ANSI_DIM) + hint + ANSI_RESET;
    if (static_cast<int>(hintLine.size()) > innerWidth) hintLine = hintLine.substr(0, static_cast<size_t>(innerWidth));
    if (static_cast<int>(hintLine.size()) < innerWidth) hintLine += repeat(' ', innerWidth - static_cast<int>(hintLine.size()));
    printLine("|" + hintLine + "|");
    printLine(top);

    // bottom padding to truly fill screen
    int usedRows = topPad + boxHeight;
    if (ts.rows > usedRows) cout << repeat('\n', ts.rows - usedRows);
    cout.flush();
}
} // namespace

// showWelcome
// Purpose: Display the welcome message and basic control instructions of the game.
// Input: None.
// Output: None. The function prints text to the terminal.
void UI::showWelcome() const {
    hideCursor(true);
    TermSize ts = getTermSize();
    auto sleepMs = [](int ms) { this_thread::sleep_for(chrono::milliseconds(ms)); };

    // 明确拼出 “UNO”
    vector<string> art = {
        " _   _  _   _   ___ ",
        "| | | || \\ | | / _ \\",
        "| | | ||  \\| || | | |",
        "| |_| || |\\  || |_| |",
        " \\___/ |_| \\_| \\___/ "
    };

    // “搞怪一点”：闪烁+渐隐（用 DIM / NORMAL 交替 + 清屏）
    // 用 basic ANSI color，避免部分终端不显示 truecolor 导致“看起来不彩色”
    for (int frame = 0; frame < 10; ++frame) {
        clearScreen();
        ts = getTermSize();
        int startRow = std::max(1, (ts.rows - static_cast<int>(art.size()) - 6) / 2);
        int leftPad = std::max(0, (ts.cols - 28) / 2);
        cout << repeat('\n', startRow);
        string style = (frame < 6) ? string(ANSI_BOLD) : string(ANSI_DIM);

        const char* colors[4] = {ansi::fgRed(), ansi::fgYellow(), ansi::fgGreen(), ansi::fgBlue()};
        for (size_t i = 0; i < art.size(); ++i) {
            cout << repeat(' ', leftPad)
                 << style << colors[(frame + static_cast<int>(i)) % 4]
                 << art[i] << ansi::reset() << "\n";
        }
        cout << "\n" << repeat(' ', leftPad)
             << string(ANSI_DIM) << "Press any key to continue" << ansi::reset() << "\n";
        cout.flush();
        sleepMs(120);
    }

    // 停住等待任意键，保证用户“看得到”欢迎页
    {
        RawMode raw;
        unsigned char dummy = 0;
        (void)read(STDIN_FILENO, &dummy, 1);
    }

    clearScreen();
    hideCursor(false);
}

void UI::showProjectIntro() const {
    RawMode raw;
    hideCursor(true);

    vector<string> lines = {
        string(ansi::bold()) + "COMP2113 Group Project - UNO+" + ansi::reset(),
        "",
        "What we built:",
        "- Base UNO (4 players: 1 human + 3 AI)",
        "- Tutorial walkthrough (keyboard navigation)",
        "- Extension-ready hooks (UNO Flip / Weather / skills like 三国杀-style cards)",
        "",
        "Team members:",
        "- Alexander (Game engine / turns)",
        "- Thomas (Card + Deck)",
        "- Hugo (AI + utils)",
        "- Joe (UI)",
        "- Frost (Player)",
        "- Rebecca (Integration / testing / docs)",
        "",
        string(ansi::dim()) + "Press → to continue." + ansi::reset()
    };

    while (true) {
        renderFullScreenBox("Project Intro", lines, "← back | → continue | q skip");
        WalkKey k = readWalkKey();
        if (k == WalkKey::Skip || k == WalkKey::Right) break;
        // no previous page; left does nothing here
    }

    clearScreen();
    hideCursor(false);
}


// showGameState
// Purpose: Display the current game state, including player turn, top card, color, direction, and card counts.
// Input: state - the current snapshot of the game state.
// Output: None. The function prints formatted game information to the terminal.
void UI::showGameState(const GameState& state) const {
    vector<string> lines;
    lines.push_back("Current Player: P" + to_string(state.currentPlayerIndex) + (state.currentPlayerIndex == 0 ? " (You)" : ""));
    lines.push_back("Direction     : " + directionName(state.direction));
    lines.push_back("Top Card      : " + ansi::cardText(state.topCard));
    lines.push_back("Current Color : " + ansi::colorName(state.currentColor, colorName(state.currentColor)));
    lines.push_back("");
    string counts = "Cards Left    : ";
    for (int i = 0; i < 4; ++i) {
        if (i) counts += "   ";
        counts += "P" + to_string(i) + "=" + to_string(state.playerCardCounts[i]);
    }
    lines.push_back(counts);
    lines.push_back("");
    lines.push_back(string(ansi::dim()) + "Tip: use Esc from your hand menu to quit." + ansi::reset());

    renderFullScreenBox("UNO - Round Status", lines, "Up/Down move | Space confirm | Esc quit");
}


// showHand
// Purpose: Display the human player's hand and mark cards that are playable in the current turn.
// Input: hand - the list of cards in the player's hand; state - the current snapshot of the game state.
// Output: None. The function prints the player's cards to the terminal.
void UI::showHand(const vector<Card>& hand, const GameState& state) const {
    cout << string(ansi::bold()) << "Your Hand" << ansi::reset() << "\n";
    if (hand.empty()) {
        cout << "  (empty)\n\n";
        return;
    }

    for (size_t i = 0; i < hand.size(); ++i) {
        cout << "  [" << string(ansi::bold()) << i << ansi::reset() << "] " << ansi::cardText(hand[i]);
        if (hand[i].isPlayable(state.topCard, state.currentColor)) {
            cout << "   " << string(ansi::bold()) << ansi::fgGreen() << "<playable>" << ansi::reset();
        }
        cout << "\n";
    }
    cout << "\n";
}

void UI::showHandSelectable(const vector<Card>& hand, const GameState& state, int selectedIndex, bool includeDrawOption) const {
    cout << string(ansi::bold()) << "Your Hand (use \u2191/\u2193, Space to select)" << ansi::reset() << "\n";
    if (hand.empty() && !includeDrawOption) {
        cout << "  (empty)\n\n";
        return;
    }

    auto printRow = [&](int idx, const string& label, bool playable, bool selected) {
        if (selected) {
            // 不用反白（会让卡牌颜色失真）。只用箭头高亮。
            cout << string(ansi::bold()) << "\033[38;2;120;220;255m" << "▶ " << ansi::reset();
        } else {
            cout << "  ";
        }
        cout << "[" << string(ansi::bold()) << idx << ansi::reset() << "] " << label;
        if (playable) {
            cout << "   " << string(ansi::bold()) << ansi::fgGreen() << "<playable>" << ansi::reset();
        }
        cout << "\n";
    };

    for (size_t i = 0; i < hand.size(); ++i) {
        bool playable = hand[i].isPlayable(state.topCard, state.currentColor);
        bool selected = (static_cast<int>(i) == selectedIndex);
        printRow(static_cast<int>(i), ansi::cardText(hand[i]), playable, selected);
    }

    if (includeDrawOption) {
        int drawIdx = static_cast<int>(hand.size());
        bool selected = (drawIdx == selectedIndex);
        printRow(drawIdx, string(ansi::bold()) + ansi::fgWhite() + "Draw a card (-1)" + ansi::reset(), true, selected);
    }

    cout << "\n";
}

// getPlayerInput
// Purpose: Ask the human player to choose a card index to play or choose to draw a card.
// Input: handSize - the number of cards currently in the player's hand.
// Output: Returns an integer representing the selected card index, or -1 if the player chooses to draw.
int UI::getPlayerInput(int handSize) const {
    int choice;
    while (true) {
        cout << "Choose a card index (0-" << handSize - 1 << ") or -1 to draw: ";
        if (cin >> choice) {
            if (choice >= -1 && choice < handSize) {
                return choice;
            }
        }
        cout << "Invalid input. Please enter a valid card index or -1.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// getColorChoice
// Purpose: Ask the human player to choose a color after playing a Wild or Wild Draw Four card.
// Input: None.
// Output: Returns a lowercase character ('r', 'y', 'g', or 'b') representing the selected color.
char UI::getColorChoice() const {
    char choice;
    while (true) {
        cout << "Choose a color [R/Y/G/B]: ";
        if (cin >> choice) {
            choice = static_cast<char>(tolower(static_cast<unsigned char>(choice)));
            if (choice == 'r' || choice == 'y' || choice == 'g' || choice == 'b') {
                return choice;
            }
        }
        cout << "Invalid color. Please enter R, Y, G, or B.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// showDrawMessage
// Purpose: Display the card just drawn by the player.
// Input: card - the card drawn from the deck.
// Output: None. The function prints a message to the terminal.
void UI::showDrawMessage(const Card& card) const {
    cout << "You drew: " << ansi::cardText(card) << "\n";
}

// showInvalidMove
// Purpose: Inform the player that the selected card cannot be played under the current game state.
// Input: card - the card selected by the player; state - the current snapshot of the game state.
// Output: None. The function prints an error message to the terminal.
void UI::showInvalidMove(const Card& card, const GameState& state) const {
    cout << "Invalid move: " << ansi::cardText(card)
         << " cannot be played on " << ansi::cardText(state.topCard)
         << " with current color " << colorName(state.currentColor) << ".\n";
}

// showActionMessage
// Purpose: Display a general action or status message.
// Input: message - the message to be displayed.
// Output: None. The function prints the message to the terminal.
void UI::showActionMessage(const string& message) const {
    cout << message << "\n";
}

// showWinner
// Purpose: Display the winner of the game.
// Input: playerIndex - the index of the winning player.
// Output: None. The function prints the winner information to the terminal.
void UI::showWinner(int playerIndex) const {
    cout << "\n========================================\n";
    cout << "Winner: P" << playerIndex;
    if (playerIndex == 0) {
        cout << " (You)";
    }
    cout << "\n========================================\n";
}

void UI::runRuleWalkthrough() const {
    struct Page { string title; vector<string> lines; };
    vector<Page> pages = {
        {"Walkthrough 1/6 - Goal", {
            "Win by being the first player to empty your hand.",
            "Players: P0 (you) + P1/P2/P3 (AI).",
            "",
            "Tip: You can skip this tutorial anytime."
        }},
        {"Walkthrough 2/6 - Play a Card", {
            "On your turn, play ONE valid card from your hand.",
            "A card is valid if it matches:",
            "- the current color, OR",
            "- the top card's number/type, OR",
            "- it is a Wild card."
        }},
        {"Walkthrough 3/6 - Draw Rule", {
            "If you cannot (or choose not to) play a card:",
            "Draw 1 card from the deck.",
            "If the drawn card is playable, you may play it immediately.",
            "Otherwise, your turn ends."
        }},
        {"Walkthrough 4/6 - Special Cards", {
            "Skip: next player loses a turn.",
            "Reverse: direction changes.",
            "Draw Two: next player draws 2 and is skipped.",
            "Wild: choose a new current color.",
            "Wild Draw Four: choose color; next draws 4 and is skipped (only if you have no current-color card)."
        }},
        {"Walkthrough 5/6 - UNO Call", {
            "When you have exactly 1 card left after playing, call UNO.",
            "If you forget and the game catches you before the next turn,",
            "you draw 2 penalty cards."
        }},
        {"Walkthrough 6/6 - Example Flow", {
            "P0 plays Red 5 -> P1 plays Red Skip -> P2 is skipped",
            "-> P3 plays Wild (choose Blue) -> P0 must follow Blue -> ...",
            "",
            "Now you're ready."
        }},
    };

    const string hint = "← back   → next   q skip";
    RawMode raw;
    hideCursor(true);
    int i = 0;
    while (i >= 0 && i < static_cast<int>(pages.size())) {
        renderBoxedPage(pages[static_cast<size_t>(i)].title, pages[static_cast<size_t>(i)].lines, hint);
        WalkKey k = readWalkKey();
        if (k == WalkKey::Skip) {
            clearScreen();
            hideCursor(false);
            cout << "Tutorial skipped. Starting game...\n";
            return;
        }
        if (k == WalkKey::Left) {
            i = std::max(0, i - 1);
        } else if (k == WalkKey::Right) {
            i++;
        }
    }
    clearScreen();
    hideCursor(false);
}

// colorName
// Purpose: Convert the internal color code of a card into a readable string.
// Input: color - a character representing a card color.
// Output: Returns the corresponding color name as a string.
string UI::colorName(char color) const {
    switch (tolower(static_cast<unsigned char>(color))) {
        case 'r': return "Red";
        case 'y': return "Yellow";
        case 'g': return "Green";
        case 'b': return "Blue";
        case 'x': return "Wild / Unset";
        case 'n': return "None";
        default: return "Unknown";
    }
}

// directionName
// Purpose: Convert the turn direction integer into a readable description.
// Input: direction - the current direction of play.
// Output: Returns a string describing the direction of play.
string UI::directionName(int direction) const {
    return direction >= 0 ? "Clockwise" : "Counter-clockwise";
}

// weatherName
// Purpose: Convert the internal weather state code into a readable string.
// Input: weather - an integer representing the weather state.
// Output: Returns the corresponding weather description as a string.
string UI::weatherName(int weather) const {
    switch (weather) {
        case 0: return "Normal / Sunny";
        case 1: return "Rainy";
        case 2: return "Typhoon";
        case 3: return "Roulette";
        default: return "Unknown";
    }
}

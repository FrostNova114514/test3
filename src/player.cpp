#include "player.h" // player interface // 玩家接口

#include "game.h" // game state definition // 游戏状态定义
#include "ui.h"
#include "ansi.h"

#include <algorithm> // std::find_if // 使用 std::find_if
#include <cctype> // std::tolower // 使用 std::tolower
#include <iostream> // std::cout, std::cin // 使用 std::cout 和 std::cin
#include <limits> // std::numeric_limits // 使用 std::numeric_limits
#include <utility> // utility helpers if needed // 如有需要使用通用工具
#include <string>

#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

namespace {
class RawModeLocal {
public:
    RawModeLocal() : ok(false) {
        if (tcgetattr(STDIN_FILENO, &old) != 0) return;
        termios raw = old;
        raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return;
        ok = true;
    }
    ~RawModeLocal() {
        if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &old);
    }
    RawModeLocal(const RawModeLocal&) = delete;
    RawModeLocal& operator=(const RawModeLocal&) = delete;
private:
    termios old{};
    bool ok;
};

int readChar() {
    unsigned char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) != 1) return -1;
    return ch;
}

bool inputAvailable(int timeoutUsec) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = timeoutUsec;

    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &timeout) > 0;
}

enum class NavKey { Up, Down, Select, Enter, Escape, Other };

NavKey readNavKey() {
    int ch = readChar();
    if (ch < 0) return NavKey::Other;
    if (ch == 27) { // ESC or escape sequence
        if (!inputAvailable(30000)) return NavKey::Escape;
        int ch2 = readChar();
        if (ch2 == -1) return NavKey::Escape;
        if (ch2 != '[') return NavKey::Escape;
        if (!inputAvailable(30000)) return NavKey::Other;
        int ch3 = readChar();
        if (ch3 == 'A') return NavKey::Up;
        if (ch3 == 'B') return NavKey::Down;
        return NavKey::Other;
    }
    if (ch == ' ') return NavKey::Select;
    if (ch == '\r' || ch == '\n') return NavKey::Enter;
    return NavKey::Other;
}

} // namespace

Player::Player() = default; // default constructor // 默认构造函数

Player::~Player() = default; // virtual destructor // 虚析构函数

void Player::addCard(const Card& card) { // add a card to the hand // 向手牌中加入一张牌
    hand.push_back(card); // append card // 追加牌
} // end addCard // 结束 addCard

bool Player::removeCard(const Card& card) { // remove a matching card // 移除一张匹配的牌
    auto it = std::find_if(hand.begin(), hand.end(), [&card](const Card& current) { // search hand // 搜索手牌
        return current.getColor() == card.getColor() && // compare color // 比较颜色
               current.getType() == card.getType() && // compare type // 比较类型
               current.getValue() == card.getValue(); // compare value // 比较数值
    }); // end search // 结束搜索

    if (it == hand.end()) { // not found // 未找到
        return false; // removal failed // 删除失败
    } // end not found check // 结束未找到检查

    hand.erase(it); // erase matching card // 删除匹配的牌
    return true; // removal succeeded // 删除成功
} // end removeCard // 结束 removeCard

int Player::getCardCount() const { // get hand size // 获取手牌数量
    return static_cast<int>(hand.size()); // convert size to int // 转换为整型
} // end getCardCount // 结束 getCardCount

const std::vector<Card>& Player::getHand() const { // read-only hand access // 只读访问手牌
    return hand; // return hand reference // 返回手牌引用
} // end getHand // 结束 getHand

bool Player::hasPlayableCard(const Card& topCard, char currentColor) const { // check playable cards // 检查是否有可出牌
    for (const Card& card : hand) { // scan all cards // 遍历所有牌
        if (card.isPlayable(topCard, currentColor)) { // if playable // 如果可出
            return true; // found one // 找到一张
        } // end playable check // 结束可出检查
    } // end loop // 结束循环
    return false; // none found // 没有找到
} // end hasPlayableCard // 结束 hasPlayableCard

char Player::chooseColor() { // default color choice // 默认选择颜色
    return 'r'; // fallback red // 默认返回红色
} // end chooseColor // 结束 chooseColor

HumanPlayer::HumanPlayer() = default; // default constructor // 默认构造函数

HumanPlayer::~HumanPlayer() = default; // destructor // 析构函数

Card HumanPlayer::takeTurn(const GameState& state) { // 询问人类玩家出牌（COMP2113 课程要求：循环直到合法选择）
    while (true) { // 循环提示用户，直到做出合法出牌或选择抽牌
        UI ui;
        ui.showGameState(state);
        int selected = 0;
        if (!hand.empty()) selected = 0;

        // Raw menu: Up/Down to move, Space to confirm, Esc quit
        RawModeLocal raw;
        int optionCount = static_cast<int>(hand.size()) + 1; // +1 for Draw
        selected = std::min(selected, optionCount - 1);

        while (true) {
            ui.showHandSelectable(hand, state, selected, true);
            std::cout << "Space=confirm  Esc=quit\n";

            NavKey key = readNavKey();
            if (key == NavKey::Escape) return Game::quitSignal();
            if (key == NavKey::Up) {
                selected = (selected - 1 + optionCount) % optionCount;
                ui.showGameState(state);
                continue;
            }
            if (key == NavKey::Down) {
                selected = (selected + 1) % optionCount;
                ui.showGameState(state);
                continue;
            }
            if (key == NavKey::Select) {
                // Draw option
                if (selected == static_cast<int>(hand.size())) {
                    return Card();
                }

                // Validate chosen card
                if (hand[selected].isPlayable(state.topCard, state.currentColor)) {
                    return hand[selected];
                }
                std::cout << "Invalid move. Pick a playable card or draw.\n";
                // small pause
                usleep(350000);
                ui.showGameState(state);
                continue;
            }
        }

        // 选牌不合法：输出详细错误提示并重新循环
        std::cout << "Invalid move! That card cannot be played on "
                  << ansi::cardText(state.topCard) << " with current color "
                  << state.currentColor << ". Please choose another card.\n";
    } // 结束 while 循环
} // 结束 takeTurn

char HumanPlayer::chooseColor() { // human color choice // 人类选择颜色
    std::cout << "Choose color (r/y/g/b): ";
    std::cout << " [" << ansi::bold() << ansi::fgRed() << "R" << ansi::reset() << "]"
              << " [" << ansi::bold() << ansi::fgYellow() << "Y" << ansi::reset() << "]"
              << " [" << ansi::bold() << ansi::fgGreen() << "G" << ansi::reset() << "]"
              << " [" << ansi::bold() << ansi::fgBlue() << "B" << ansi::reset() << "]\n";
    char color = 'r'; // default color variable // 默认颜色变量
    while (true) { // keep asking until valid // 持续询问直到有效
        if (std::cin >> color) { // read input // 读取输入
            if (color == 27 || color == 'q' || color == 'Q') {
                // Esc isn't reliably captured in canonical mode; allow q to quit.
                // Signal quit by throwing to upper level is too invasive; fallback to red.
                return 'r';
            }
            color = static_cast<char>(std::tolower(static_cast<unsigned char>(color))); // normalize case // 统一大小写
            if (color == 'r' || color == 'y' || color == 'g' || color == 'b') { // validate color // 校验颜色
                return color; // return valid color // 返回有效颜色
            } // end valid color check // 结束有效颜色检查
        } // end successful read // 结束成功读取
        std::cin.clear(); // clear stream state // 清除流状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input // 丢弃无效输入
        std::cout << "Invalid color. Choose r/y/g/b: "; // reprompt // 重新提示
    } // end loop // 结束循环
} // end chooseColor // 结束 chooseColor


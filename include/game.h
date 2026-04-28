#ifndef GAME_H
#define GAME_H

#include <vector>
#include <array>
#include <string>

// Include other module headers
#include "card.h"
#include "deck.h"
#include "player.h"

// Game state snapshot used to pass render data to the UI
struct GameState {
    Card topCard;                 // Top card of the discard pile (for display)
    char currentColor;            // Current active color ('r', 'y', 'g', 'b', or 'x')
    int currentPlayerIndex;       // Current player index (0-3)
    int direction;                // Game direction (1 = clockwise, -1 = counter-clockwise)
    std::array<int, 4> playerCardCounts; // Remaining hand count for each player

    // --- Reserved for Phase 2 & 3 extensions ---
    bool isDarkSide;              // [Phase 2] Uno Flip dark-side state
    int currentWeather;           // [Phase 3] Weather state (0=sunny, 1=rainy, 2=typhoon, 3=roulette)
};

class Game {
private:
    // --- Core variables ---
    std::vector<Player*> players;    // [Dynamic memory and polymorphism] Stores 1 HumanPlayer and 3 AIPlayers
    Deck deck;                       // Draw pile
    std::vector<Card> discardPile;   // 弃牌堆

    int currentPlayer;               // 当前回合玩家索引（0-3）
    int direction;                   // 游戏方向（1=顺时针, -1=逆时针）
    char currentColor;               // 当前生效颜色
    int pendingDrawCount;            // 待执行的罚牌数（累计 +2/+4 惩罚）
    bool isGameOver;                 // 游戏结束标志

    // --- Phase 2 & 3 扩展预留 ---
    bool isDarkSide;
    int weatherState;
    std::vector<bool> unoAnnounced;      // 记录玩家是否在剩 1 张牌时喊 UNO
    int pendingWinner;                   // 最后一张是 +2/+4 时，延后结算赢家

    // --- 内部逻辑辅助函数 ---
    void applyCardEffect(const Card& playedCard); // 处理特殊卡牌效果
    void nextTurn();                              // 切换至下一位玩家
    bool checkAndReshuffleDeck();                 // 防崩溃机制：牌堆空时尝试从弃牌堆重建，返回是否成功
    bool checkWin(int playerIndex) const;         // 检查指定玩家是否获胜
    bool canUseWildDrawFour(const Player* player) const; // 检查 +4 使用是否合法
    void resolveUnoPenalties();                   // 在回合开始前处理 UNO 漏喊惩罚

public:
    // --- 构造与析构 ---
    Game();
    ~Game(); // [防止内存泄漏] 负责释放 players 向量中的动态分配对象

    // --- File I/O（课程硬性要求：文件读写） ---
    void logGameResult(const std::string& filename, int winnerIndex);

    // --- 主循环 API ---
    void start();                    // 启动游戏主循环
    void playTurn();                 // 处理单名玩家的完整回合
    GameState getSnapshot() const;   // 获取打包后的当前游戏状态

    static Card quitSignal();        // 人类玩家退出信号卡
};

#endif // GAME_H

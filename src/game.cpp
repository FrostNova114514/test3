#include "game.h"
#include "ai.h"
#include "player.h"
#include "ui.h" // UI module header (course OOP requirement: activate terminal rendering in Game::start())
#include "ansi.h"
#include <iostream>
#include <fstream>
#include <stdexcept> // Used to catch std::runtime_error exceptions thrown by Deck
#include <limits>

// Constructor: initialize memory, shuffle, and deal cards
Game::Game() {
    // 1. Add 1 HumanPlayer and 3 AIPlayers to the players vector (polymorphism)
    players.push_back(new HumanPlayer());
    for (int i = 0; i < 3; ++i) {
        players.push_back(new AIPlayer());
    }

    // 2. Initialize core variables
    currentPlayer = 0;
    direction = 1;
    pendingDrawCount = 0;
    isGameOver = false;
    isDarkSide = false; // Reserved for Phase 2 flip functionality
    weatherState = 0;   // 0 = Sunny
    unoAnnounced.assign(4, false);
    pendingWinner = -1;

    // 3. Shuffle
    deck.shuffle();

    // 4. Deal cards (7 each)
    bool dealFailed = false; // 发牌失败标志，用于中断双层循环
    for (int i = 0; i < 7; ++i) {
        for (Player* player : players) {
            // Check whether the pile is empty before each draw to avoid crashes
            if (!checkAndReshuffleDeck()) {
                dealFailed = true;
                break; // 牌堆重建失败，停止发牌
            }
            player->addCard(deck.drawCard());
        }
        if (dealFailed) {
            break; // 跳出外层发牌循环
        }
    }

    // 5. Flip the first card of the discard pile
    // Standard rule: the initial card cannot be a Wild Draw Four; if drawn, put it back and redraw
    Card firstCard = deck.drawCard();
    while (firstCard.getType() == CardType::WildDrawFour) {
        discardPile.push_back(firstCard); // Wild card is placed at the bottom of the discard pile and will not become the top card
        if (!checkAndReshuffleDeck()) {
            break; // Stop the redraw loop if rebuilding the pile fails, to prevent an infinite loop
        }
        firstCard = deck.drawCard();
    }
    discardPile.push_back(firstCard);
    if (firstCard.getType() == CardType::Wild) {
        currentColor = players[currentPlayer]->chooseColor();
    } else {
        currentColor = firstCard.getColor();
    }
}

// 析构函数：释放动态分配的玩家对象，防止内存泄漏
Game::~Game() {
    for (Player* player : players) {
        delete player;
    }
    players.clear();
}

// 以追加模式将游戏结果写入文件（课程硬性要求：File I/O）
void Game::logGameResult(const std::string& filename, int winnerIndex) {
    // 使用 std::ios::app 追加模式，每次游戏结束都在文件末尾追加记录
    std::ofstream outFile(filename, std::ios::app);
    if (!outFile.is_open()) {
        // 文件打开失败时输出错误提示
        std::cerr << "错误：无法打开文件 " << filename << " 进行写入！" << std::endl;
        return;
    }

    // 写入获胜者编号、游戏方向、最终颜色等关键信息
    outFile << "=== 游戏结果 ===\n";
    outFile << "获胜者编号: " << winnerIndex << "\n";
    outFile << "游戏方向: " << direction << " (1=顺时针, -1=逆时针)\n";
    outFile << "最终颜色: " << currentColor << "\n";
    outFile << "================\n\n";

    outFile.close();
}

// 核心游戏主循环（COMP2113 课程要求：接入 UI 模块进行终端渲染）
void Game::start() {
    UI ui; // 实例化 UI 对象，激活闲置的 UI 模块
    ui.showWelcome(); // 显示游戏欢迎界面与控制说明
    ui.showProjectIntro(); // 项目介绍 + 成员名单
    ui.runRuleWalkthrough(); // 开局新手引导：分步演示基础规则流程

    while (!isGameOver) {
        resolveUnoPenalties();

        // 0. 触发天气判定（天气/轮盘随机事件），Phase 1 暂不需要

        // 1. 获取游戏状态快照并渲染 UI 面板
        GameState state = getSnapshot();
        ui.showGameState(state); // 在终端渲染当前游戏状态面板（顶牌、颜色、方向、手牌数）

        // 2. 执行当前玩家的回合
        int actor = currentPlayer;
        playTurn();

        // 3. 若是 +2/+4 的收尾牌，需等待罚抽结算后再判胜
        if (pendingWinner != -1 && pendingDrawCount == 0) {
            isGameOver = true;
            ui.showWinner(pendingWinner);
            logGameResult("game_results.txt", pendingWinner);
            break;
        }

        // 4. 检查当前回合玩家是否获胜
        if (checkWin(actor)) {
            isGameOver = true;
            ui.showWinner(actor); // 在终端显示获胜玩家信息
            // 将胜利记录以追加模式写入文件（课程 File I/O 要求）
            logGameResult("game_results.txt", actor);
            break;
        }

        // 5. 切换到下一位玩家
        nextTurn();
    }
}

// 处理单名玩家的完整回合逻辑
void Game::playTurn() {
    Player* currentP = players[currentPlayer];

    // 1. 环境与惩罚结算
    if (pendingDrawCount > 0) {
        // 强制执行罚抽牌
        for (int i = 0; i < pendingDrawCount; ++i) {
            // 每次抽牌前检查牌堆是否为空，若重建失败则中断罚抽循环
            if (!checkAndReshuffleDeck()) {
                break; // 牌堆重建失败，立即停止罚抽，防止崩溃
            }
            currentP->addCard(deck.drawCard());
        }
        pendingDrawCount = 0; // 清除累计罚牌数
        if (pendingWinner != -1) {
            // +2/+4 收尾时，需先让受罚者完成抽牌，再允许结算赢家
        }
        return; // 罚牌后跳过本回合出牌阶段
    }

    // 2. 获取玩家的出牌决策
    // takeTurn 返回玩家决定打出的牌；若选择摸牌，则返回一张虚拟牌（color='x', type=Number, value=-1）
    Card playedCard = currentP->takeTurn(getSnapshot());

    if (playedCard.getValue() == -2 && playedCard.getType() == CardType::Number) {
        isGameOver = true;
        std::cout << ansi::dim() << "Quit requested. Bye.\n" << ansi::reset();
        return;
    }

    // 2.5 显式拦截抽牌信号（COMP2113 课程要求：防止虚拟牌被误判导致游戏逻辑混乱）
    // 虚拟牌特征：getValue()==-1 且 getType()==CardType::Number（即 Card 的默认构造函数产生的牌）
    // 必须在 isPlayable 判断之前拦截，否则虚拟牌可能意外通过合法性校验
    if (playedCard.getValue() == -1 && playedCard.getType() == CardType::Number) {
        // 玩家主动选择摸牌，执行“摸 1 张后可立即打出该张”的标准规则
        if (!checkAndReshuffleDeck()) {
            return; // 牌堆重建失败，直接结束当前回合
        }
        Card drawnCard = deck.drawCard();
        currentP->addCard(drawnCard);

        if (drawnCard.isPlayable(discardPile.back(), currentColor)) {
            bool playDrawnCard = true;
            if (currentPlayer == 0) {
                std::cout << "You drew " << ansi::cardText(drawnCard)
                          << ". Play it now? (y/n): ";
                char choice = 'y';
                if (!(std::cin >> choice)) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    choice = 'n';
                }
                playDrawnCard = (choice == 'y' || choice == 'Y');
            }

            if (playDrawnCard) {
                currentP->removeCard(drawnCard);
                discardPile.push_back(drawnCard);
                applyCardEffect(drawnCard);

                if (currentP->getCardCount() == 1) {
                    if (currentPlayer == 0) {
                        std::cout << "Call UNO now? (y/n): ";
                        char unoChoice = 'y';
                        if (!(std::cin >> unoChoice)) {
                            std::cin.clear();
                            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            unoChoice = 'n';
                        }
                        unoAnnounced[currentPlayer] = (unoChoice == 'y' || unoChoice == 'Y');
                    } else {
                        unoAnnounced[currentPlayer] = true;
                    }
                } else {
                    unoAnnounced[currentPlayer] = false;
                }

                if (currentP->getCardCount() == 0 &&
                    (drawnCard.getType() == CardType::DrawTwo || drawnCard.getType() == CardType::WildDrawFour)) {
                    pendingWinner = currentPlayer;
                }
            }
        }
        return; // 摸牌分支到此结束
    }

    if (playedCard.getType() == CardType::WildDrawFour && !canUseWildDrawFour(currentP)) {
        std::cout << "Invalid move: " << ansi::cardText(playedCard)
                  << " can only be used when no card matches current color.\n";
        if (!checkAndReshuffleDeck()) {
            return;
        }
        currentP->addCard(deck.drawCard());
        return;
    }

    // 3. 合法性校验（仅对真实出牌进行）
    bool isPlayable = playedCard.isPlayable(discardPile.back(), currentColor);

    // 4. 执行决策
    if (isPlayable) {
        // 玩家成功打出一张合法牌
        currentP->removeCard(playedCard); // 从手牌中移除
        discardPile.push_back(playedCard); // 放入弃牌堆
        applyCardEffect(playedCard);       // 应用特殊卡牌效果

        if (currentP->getCardCount() == 1) {
            if (currentPlayer == 0) {
                std::cout << "Call UNO now? (y/n): ";
                char unoChoice = 'y';
                if (!(std::cin >> unoChoice)) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    unoChoice = 'n';
                }
                unoAnnounced[currentPlayer] = (unoChoice == 'y' || unoChoice == 'Y');
            } else {
                unoAnnounced[currentPlayer] = true;
            }
        } else {
            unoAnnounced[currentPlayer] = false;
        }

        if (currentP->getCardCount() == 0 &&
            (playedCard.getType() == CardType::DrawTwo || playedCard.getType() == CardType::WildDrawFour)) {
            pendingWinner = currentPlayer;
        }
    } else {
        // 无效出牌，或玩家主动选择摸牌
        // 摸牌前检查牌堆是否为空，若重建失败则跳过摸牌，防止崩溃
        if (!checkAndReshuffleDeck()) {
            return; // 牌堆重建失败，直接结束本回合，不再调用 drawCard()
        }
        currentP->addCard(deck.drawCard());
        // Phase 1 简化处理：摸牌后立即结束回合
    }
}

// 处理卡牌效果与颜色变更（使用 Getter 方法访问私有成员）
void Game::applyCardEffect(const Card& playedCard) {
    // 若卡牌具有有效颜色（非 Wild 的 'x'），则更新当前生效颜色
    // 补充完整的颜色判断：必须是 r/y/g/b 四种有效颜色之一，而非仅判断 != 'x'
    char cardColor = playedCard.getColor();
    if (cardColor == 'r' || cardColor == 'y' || cardColor == 'g' || cardColor == 'b') {
        currentColor = cardColor;
    }

    // 根据卡牌类型（通过 getType() 获取）执行对应效果
    CardType cardType = playedCard.getType();
    if (cardType == CardType::Skip) {
        nextTurn(); // 跳过下一位玩家（额外一次回合推进）
    }
    else if (cardType == CardType::Reverse) {
        direction *= -1; // 反转游戏方向
    }
    else if (cardType == CardType::DrawTwo) {
        pendingDrawCount += 2; // 累计 +2 罚牌
    }
    else if (cardType == CardType::Wild) {
        // 调用当前玩家的 chooseColor() 选择新颜色
        currentColor = players[currentPlayer]->chooseColor();
    }
    else if (cardType == CardType::WildDrawFour) {
        pendingDrawCount += 4; // 累计 +4 罚牌
        // 调用当前玩家的 chooseColor() 选择新颜色
        currentColor = players[currentPlayer]->chooseColor();
    }
}

// 玩家轮转算法：按方向计算下一位玩家索引
void Game::nextTurn() {
    int n = players.size();
    // 加上 n 可防止 direction 为 -1 时出现负数索引越界
    currentPlayer = (currentPlayer + direction + n) % n;
}

// 防崩溃机制：当抽牌堆为空时，尝试从弃牌堆重建（使用 try-catch 捕获异常）
bool Game::checkAndReshuffleDeck() {
    // 若牌堆非空，无需重建，直接返回成功
    if (deck.size() > 0) {
        return true;
    }

    // 牌堆已空，尝试用弃牌堆重建抽牌堆
    try {
        // Deck::rebuildFromDiscard 内部会保留弃牌堆顶牌，其余洗回抽牌堆
        deck.rebuildFromDiscard(discardPile);
        return true; // 重建成功
    } catch (const std::exception& e) {
        // 重建失败（如弃牌堆牌数不足），输出错误信息并返回 false
        std::cerr << "错误：牌堆重建失败 - " << e.what() << std::endl;
        return false; // 返回 false 通知调用方不要再调用 drawCard()
    }
}

// 检查胜利条件：指定玩家手牌为空
bool Game::checkWin(int playerIndex) const {
    return players[playerIndex]->getCardCount() == 0;
}

bool Game::canUseWildDrawFour(const Player* player) const {
    const std::vector<Card>& hand = player->getHand();
    for (const Card& card : hand) {
        if (card.getType() == CardType::Wild || card.getType() == CardType::WildDrawFour) {
            continue;
        }
        if (card.getColor() == currentColor) {
            return false;
        }
    }
    return true;
}

void Game::resolveUnoPenalties() {
    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        if (players[i]->getCardCount() == 1 && !unoAnnounced[i]) {
            std::cout << "Player " << i << " failed to call UNO and draws 2 cards.\n";
            for (int draw = 0; draw < 2; ++draw) {
                if (!checkAndReshuffleDeck()) {
                    break;
                }
                players[i]->addCard(deck.drawCard());
            }
        }
        if (players[i]->getCardCount() != 1) {
            unoAnnounced[i] = false;
        }
    }
}

// 获取游戏状态快照，供 UI 和玩家决策使用
GameState Game::getSnapshot() const {
    GameState state;

    // 安全检查：弃牌堆正常情况下不应为空
    if (!discardPile.empty()) {
        state.topCard = discardPile.back();
    }

    state.currentColor = currentColor;
    state.currentPlayerIndex = currentPlayer;
    state.direction = direction;
    state.isDarkSide = isDarkSide;
    state.currentWeather = weatherState;

    for (int i = 0; i < 4; ++i) {
        state.playerCardCounts[i] = players[i]->getCardCount();
    }

    return state;
}

Card Game::quitSignal() {
    return Card('x', CardType::Number, -2);
}

#include "deck.h"

#include <algorithm>    // std::shuffle
#include <random>       // std::mt19937, std::random_device
#include <stdexcept>    // std::runtime_error

// ====================== 构造函数 ======================
Deck::Deck() {
    initialize();
    shuffle();
}

// ====================== 初始化整副牌 ======================
//
// 标准 UNO 一副牌（不含特殊玩法）：108 张：
// - 4 种颜色：r, y, g, b
// - 每种颜色：
//   · 1 张数字 0
//   · 2 张数字 1-9
//   · 2 张 Skip
//   · 2 张 Reverse
//   · 2 张 Draw Two
// - 4 张 Wild（无颜色）
// - 4 张 Wild Draw Four（无颜色）
void Deck::initialize() {
    drawPile.clear();

    const char COLORS[4] = {'r', 'y', 'g', 'b'};
    const char WILD_COLOR = 'x'; // 无颜色占位

    // 四种颜色牌
    for (int i = 0; i < 4; ++i) {
        char c = COLORS[i];

        // 1 张 0
        drawPile.emplace_back(c, CardType::Number, 0);

        // 每个颜色两张 1-9
        for (int number = 1; number <= 9; ++number) {
            drawPile.emplace_back(c, CardType::Number, number);
            drawPile.emplace_back(c, CardType::Number, number);
        }

        // 2 张 Skip
        drawPile.emplace_back(c, CardType::Skip, -1);
        drawPile.emplace_back(c, CardType::Skip, -1);

        // 2 张 Reverse
        drawPile.emplace_back(c, CardType::Reverse, -1);
        drawPile.emplace_back(c, CardType::Reverse, -1);

        // 2 张 Draw Two
        drawPile.emplace_back(c, CardType::DrawTwo, -1);
        drawPile.emplace_back(c, CardType::DrawTwo, -1);
    }

    // 4 张 Wild
    for (int i = 0; i < 4; ++i) {
        drawPile.emplace_back(WILD_COLOR, CardType::Wild, -1);
    }

    // 4 张 Wild Draw Four
    for (int i = 0; i < 4; ++i) {
        drawPile.emplace_back(WILD_COLOR, CardType::WildDrawFour, -1);
    }

    // 可选：这里可以检查 drawPile.size() == 108
}

// ====================== 洗牌 ======================
void Deck::shuffle() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(drawPile.begin(), drawPile.end(), gen);
}

// ====================== 抽一张牌 ======================
Card Deck::drawCard() {
    if (drawPile.empty()) {
        throw std::runtime_error("Deck::drawCard: draw pile is empty.");
    }

    Card card = drawPile.back();
    drawPile.pop_back();
    return card;
}

// ====================== 是否空牌堆 ======================
bool Deck::isEmpty() const {
    return drawPile.empty();
}

// ====================== 牌堆剩余张数 ======================
int Deck::size() const {
    return static_cast<int>(drawPile.size());
}

// ====================== 用弃牌堆重建抽牌堆 ======================
//
// 逻辑：
// 1. 弃牌堆至少要有 2 张牌，否则无法重建（只剩顶牌没法洗）
// 2. 保留弃牌堆顶牌 topCard
// 3. 其余所有弃牌复制回 drawPile
// 4. 弃牌堆清空，只放回 topCard
// 5. 对 drawPile 洗牌
void Deck::rebuildFromDiscard(std::vector<Card>& discardPile) {
    if (discardPile.size() <= 1) {
        throw std::runtime_error("Deck::rebuildFromDiscard: not enough cards in discard pile.");
    }

    // 保留顶部牌
    Card topCard = discardPile.back();
    discardPile.pop_back();

    // 其余作为新抽牌堆
    drawPile = discardPile;

    // 清空弃牌堆，仅保留 topCard
    discardPile.clear();
    discardPile.push_back(topCard);

    // 洗牌
    shuffle();
}

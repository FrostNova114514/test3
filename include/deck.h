#ifndef DECK_H
#define DECK_H

#include <vector>
#include "card.h"

// 牌堆系统：负责“牌”的生成、存储、洗牌、抽牌
class Deck {
private:
    // 抽牌堆：面朝下的牌堆，顶端是 back()
    std::vector<Card> drawPile;

public:
    // 构造函数：生成一副标准 UNO 牌并洗牌
    Deck();

    // 重新生成整副标准 UNO 牌（108 张）
    void initialize();

    // 洗牌
    void shuffle();

    // 抽一张牌；若牌堆为空则抛出异常
    Card drawCard();

    // 抽牌堆是否为空
    bool isEmpty() const;

    // 当前抽牌堆剩余张数
    int size() const;

    // 用弃牌堆重建抽牌堆：
    // - 保留弃牌堆顶牌
    // - 其余全部洗回抽牌堆
    void rebuildFromDiscard(std::vector<Card>& discardPile);
};

#endif // DECK_H

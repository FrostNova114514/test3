#ifndef CARD_H
#define CARD_H

#include <string>

// UNO 牌的类型
enum class CardType {
    Number,         // 数字牌 0-9
    Skip,           // 跳过
    Reverse,        // 反转
    DrawTwo,        // +2
    Wild,           // 变色
    WildDrawFour    // +4
};

// UNO 卡牌类
class Card {
private:
    // 颜色编码：
    // 'r' = red, 'y' = yellow, 'g' = green, 'b' = blue, 'x' = 无颜色（Wild）
    char color;

    // 牌类型
    CardType type;

    // 数值：
    // - 数字牌：0-9
    // - 功能牌：统一用 -1（不用数值）
    int value;

public:
    // 默认构造（容器需要）
    Card();

    // 常规构造函数
    Card(char color, CardType type, int value);

    // 访问接口
    char getColor() const;
    CardType getType() const;
    int getValue() const;

    void setColor(char c);  // Wild 被指定颜色后修改用

    // 判断该牌在当前局面是否可以打出
    // 参数：
    //  - topCard: 当前弃牌堆顶牌
    //  - currentColor: 当前生效颜色（Game 维护）
    bool isPlayable(const Card& topCard, char currentColor) const;

    // 调试 / UI 输出
    std::string toString() const;
};

#endif // CARD_H

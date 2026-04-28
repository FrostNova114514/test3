#ifndef PLAYER_H // include guard start // 头文件保护开始
#define PLAYER_H // include guard define // 定义头文件保护宏

#include <vector> // use std::vector for hands // 使用 std::vector 保存手牌
#include "card.h" // use Card type // 使用 Card 类型

struct GameState; // forward declaration for turn decisions // 用于回合决策的前向声明

class Player { // base player class // 玩家基类
protected: // protected section for derived classes // 供派生类访问的受保护区域
    std::vector<Card> hand; // stored cards in hand // 保存玩家手牌

public: // public interface // 公共接口
    Player(); // default constructor // 默认构造函数
    virtual ~Player(); // virtual destructor for polymorphism // 用于多态的虚析构函数

    void addCard(const Card& card); // add a card to hand // 向手牌中加入一张牌
    bool removeCard(const Card& card); // remove a matching card from hand // 从手牌中移除一张匹配的牌
    int getCardCount() const; // return number of cards in hand // 返回手牌数量
    const std::vector<Card>& getHand() const; // read-only access to hand // 只读访问手牌
    bool hasPlayableCard(const Card& topCard, char currentColor) const; // check for any playable card // 检查是否有可出的牌

    virtual Card takeTurn(const GameState& state) = 0; // choose a card to play or draw // 选择出牌或抽牌
    virtual char chooseColor() = 0; // choose a color after wild cards // 在万能牌后选择颜色
}; // end base player class // 结束玩家基类

class HumanPlayer : public Player { // human-controlled player // 人类玩家
public: // public interface // 公共接口
    HumanPlayer(); // default constructor // 默认构造函数
    ~HumanPlayer() override; // destructor override // 重写析构函数

    Card takeTurn(const GameState& state) override; // ask user for a move // 询问用户操作
    char chooseColor() override; // ask user for a color // 询问用户选择颜色
}; // end human player class // 结束人类玩家类

#endif // PLAYER_H // include guard end // 头文件保护结束

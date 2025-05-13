#include "gameservice.hpp"
#include <random>

GameService& GameService::getInstance()
{
    static GameService instance;
    return instance;
}
// 初始化全部牌
void GameService::initCards()
{
    cards.clear();
    for (int card = Card_RedHeart_A; card < Card_End; card++)
    {
        cards.push_back(static_cast<Card>(card));
    }
}
// 随机发牌三张
std::vector<Card> GameService::getRandomCards()
{
    std::vector<Card> result;
    for (int i = 0; i < 3; ++i)
    {
        result.push_back(cards[rand() % cards.size()]);
    }
    return result;
}
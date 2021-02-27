#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;

class CrackShotCardAction : public CardAction {
private:
  sf::Sprite overlay;
  SpriteProxyNode* attachment{ nullptr };
  Animation attachmentAnim;
  int damage;
public:
  CrackShotCardAction(Character& owner, int damage);
  ~CrackShotCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnEndAction() override;
  void OnExecute() override;
};

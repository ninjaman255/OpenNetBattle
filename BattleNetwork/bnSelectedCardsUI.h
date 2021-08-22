/*! \brief Optionally displays the cards over the character and publishes on behalf of the character
 */
#pragma once
#include <SFML/Graphics.hpp>
#include <sstream>
#include <vector>
#include <optional>
#include "bnUIComponent.h"
#include "bnCardUsePublisher.h"
#include "bnSpriteProxyNode.h"
#include "bnText.h"

using std::ostringstream;
using std::vector;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;

class Card;
class Character;
class CardPackageManager;
class BattleSceneBase;

class SelectedCardsUI : public CardActionUsePublisher, public UIComponent {
public:
  /**
   * \param character Character to attach to
   */
  SelectedCardsUI(Character* owner, CardPackageManager* packageManager);
  
  /**
   * @brief destructor
   */
  virtual ~SelectedCardsUI();

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /**
   * @brief Hold START to spread the cards out
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Set the cards array and size. Updates card cursor to 0.
   * @param incoming List of Card pointers
   * @param size Size of List
   */
  void LoadCards(Battle::Card** incoming, int size);
  
  /**
   * @brief Broadcasts the card at the cursor curr. Increases curr.
   * @return True if there was a card to use
   */
  virtual bool UseNextCard();

  /**
 * @brief Broadcasts the card information to all listeners
 * @param card being used
 * @param user using the card
 */
  virtual void Broadcast(std::shared_ptr<CardAction> action);

  /**
   * @brief Does nothing at this time
   */
  void Inject(BattleSceneBase&) override;

  /**
  * @brief Increases the next card's damage property
  */
  void SetMultiplier(unsigned mult);

  /**
  * @brief Return a const reference to the next card, if valid
  * @preconditions Assumes the card can be used and currCard < cardCount!
  */
  std::optional<std::reference_wrapper<const Battle::Card>> Peek();

  void HandlePeekEvent(Character* from);

  /**
  * @brief Returns the uuids of all the cards
  */
  std::vector<std::string> GetUUIDList();

protected:

  const int GetCardCount() const;
  const int GetCurrentCardIndex() const;
  const unsigned GetMultiplier() const;
  Battle::Card** SelectedCardsPtrArray() const;
  SpriteProxyNode& IconNode() const;
  SpriteProxyNode& FrameNode() const;
  CardPackageManager* packageManager{ nullptr };

private:
  double elapsed{}; /*!< Used by draw function, delta time since last update frame */
  Battle::Card** selectedCards{ nullptr }; /*!< Current list of cards. */
  int cardCount{}; /*!< Size of list */
  int curr{}; /*!< Card cursor index */
  unsigned multiplierValue{ 1 };
  mutable bool firstFrame{ true }; /*!< If true, this UI graphic is being drawn for the first time*/
  mutable SpriteProxyNode icon;
  mutable SpriteProxyNode frame; /*!< Sprite for the card icon and the black border */
};

#pragma once

#include <list>

#include "bnComponent.h"
#include "bnCard.h"
#include "bnCardUseListener.h"

class Character;

/**
 * @class CardUsePublisher
 * @author mav
 * @date 05/05/19
 * @brief Emits card use information to all subscribers
 * @see CounterHitPublisher
 */
class CardUsePublisher {
private:
  friend class CardUseListener;

  std::list<CardUseListener*> listeners; /*!< All subscribers */

  void AddListener(CardUseListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~CardUsePublisher();
  
  /**
   * @brief Must implement
   */
  virtual void UseNextCard() = 0;

 /**
  * @brief Broadcasts the card information to all listeners
  * @param card being used
  * @param user using the card
  */
  void Broadcast(Card& card, Character& user) {
    std::list<CardUseListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnCardUse(card, user);
      iter++;
    }
  }
};
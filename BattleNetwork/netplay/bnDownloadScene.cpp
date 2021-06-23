#include "bnDownloadScene.h"
#include "../bnWebClientMananger.h"
#include <Segues/PixelateBlackWashFade.h>

DownloadScene::DownloadScene(swoosh::ActivityController& ac, const DownloadSceneProps& props) : 
  downloadSuccess(props.downloadSuccess),
  label(Font::Style::tiny),
  Scene(ac)
{
  ourCardList = props.cardUUIDs;
  downloadSuccess = false; 

  packetProcessor = props.packetProcessor;
  packetProcessor->SetKickCallback([this] {
    Logger::Logf("Kicked for silence!");
    this->Abort(retryCardList);
  });

  packetProcessor->EnableKickForSilence(true);

  packetProcessor->SetPacketBodyCallback([this](NetPlaySignals header, const Poco::Buffer<char>& body) {
    this->ProcessPacketBody(header, body);
  });

  Net().AddHandler(props.remoteAddress, packetProcessor);

  lastScreen = props.lastScreen;

  blur.setPower(40);
  blur.setTexture(&lastScreen);

  bg = sf::Sprite(lastScreen);
  bg.setColor(sf::Color(255, 255, 255, 200));

  setView(sf::Vector2u(480, 320));
}

DownloadScene::~DownloadScene()
{

}

void DownloadScene::TradeCardList(const std::vector<std::string>& uuids)
{
  // Upload card list to remote. Track as the handshake.
  ourCardList = uuids;
  packetProcessor->UpdateHandshakeID(packetProcessor->SendPacket(Reliability::Reliable, SerializeUUIDs(NetPlaySignals::trade_card_list, uuids)).second);
}

void DownloadScene::RequestCardList(const std::vector<std::string>& uuids)
{
  packetProcessor->SendPacket(Reliability::Reliable, SerializeUUIDs(NetPlaySignals::card_list_request, uuids));
}

void DownloadScene::SendCustomPlayerData()
{
  // TODO:
}

void DownloadScene::SendDownloadComplete(bool success)
{
  downloadSuccess = success;

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::downloads_complete };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&downloadSuccess, sizeof(bool));

  packetProcessor->SendPacket(Reliability::Reliable, buffer);
}

void DownloadScene::SendPing()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::ping };
  buffer.append((char*)&type, sizeof(NetPlaySignals));

  packetProcessor->SendPacket(Reliability::Unreliable, buffer);
}

void DownloadScene::RecieveCustomPlayerData(const Poco::Buffer<char>& buffer)
{
  // TODO:
}

void DownloadScene::ProcessPacketBody(NetPlaySignals header, const Poco::Buffer<char>& body)
{
  switch (header) {
  case NetPlaySignals::trade_card_list: 
    Logger::Logf("Recieved trade list download signal");
    if (currState == state::trade) {
      Logger::Logf("Processing trade list");
      this->RecieveTradeCardList(body);
    }
    break;
  case NetPlaySignals::card_list_request:
    this->RecieveRequestCardList(body);
    break;
  case NetPlaySignals::card_list_download:
    Logger::Logf("Recieved card list download signal");
    if (currState == state::download) {
      Logger::Logf("Downloading card list...");
      this->DownloadCardList(body);
    }
    break;
  case NetPlaySignals::custom_character_download:
    this->RecieveCustomPlayerData(body);
    break;
  case NetPlaySignals::downloads_complete:
    this->RecieveDownloadComplete(body);
    break;
  }
}

void DownloadScene::RecieveTradeCardList(const Poco::Buffer<char>& buffer)
{
  retryCardList.clear();
  auto remote = DeserializeUUIDs(buffer);

  for (auto& remoteUUID : remote) {
    if (auto ptr = WEBCLIENT.GetWebCard(remoteUUID); !ptr) {
      retryCardList.push_back(remoteUUID);
      cardsToDownload[remoteUUID] = "Downloading";
    }
  }

  Logger::Logf("Recieved remote's list size: %d", remote.size());

  // move to the next state
  if (retryCardList.empty()) {
    Logger::Logf("Nothing to download.");
    currState = state::complete;
    SendDownloadComplete(true);
  }
  else {
    Logger::Logf("Need to download %d cards", retryCardList.size());
    currState = state::download;
    RequestCardList(retryCardList);
  }
}

void DownloadScene::RecieveRequestCardList(const Poco::Buffer<char>& buffer)
{
  auto uuids = DeserializeUUIDs(buffer);

  Logger::Logf("Recieved download request for %d items", uuids.size());
  packetProcessor->SendPacket(Reliability::BigData, SerializeCards(uuids));
}

void DownloadScene::RecieveDownloadComplete(const Poco::Buffer<char>& buffer)
{
  bool result{};
  std::memcpy(&result, buffer.begin(), sizeof(bool));

  if (result) {
    remoteSuccess = true;
  }
  else {
    // downloadSuccess ref will tell matchmaking scene we failed
    Abort(retryCardList);
  }

  Logger::Logf("Remote says download complete. Result: %s", result ? "Success" : "Fail");
}

void DownloadScene::DownloadCardList(const Poco::Buffer<char>& buffer)
{
  // Tell web client to fetch and download these cards
  std::vector<std::string> cardList;
  size_t cardLen{};
  size_t read{};

  std::memcpy(&cardLen, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  while (cardLen > 0) {
    // name
    size_t id_len = 0;
    std::memcpy((char*)&id_len, buffer.begin() + read, sizeof(size_t));
    std::string id = std::string(buffer.begin() + read, id_len);
    read += sizeof(size_t);

    // image data
    size_t image_len = 0;
    std::memcpy((char*)&image_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);

    sf::Uint8* imageData = new  sf::Uint8[image_len]{ 0 };
    std::memcpy(imageData, buffer.begin() + read, image_len);
    read += sizeof(size_t);

    sf::Image image;
    image.create(56, 48, imageData);
    auto textureObj = std::make_shared<sf::Texture>();
    textureObj->loadFromImage(image);
    delete[] imageData;

    // icon data
    size_t icon_len = 0;
    std::memcpy((char*)&icon_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);

    sf::Uint8* iconData = new  sf::Uint8[icon_len]{ 0 };
    std::memcpy(iconData, buffer.begin() + read, icon_len);
    read += sizeof(size_t);

    sf::Image icon;
    icon.create(14, 14, iconData);
    auto iconObj = std::make_shared<sf::Texture>();
    iconObj->loadFromImage(icon);
    delete[] iconData;

    ////////////////////
    //   Card Model   //
    ////////////////////
    size_t model_id_len = 0;
    std::string model_id;

    std::memcpy((char*)&model_id_len, buffer.begin() + read, sizeof(size_t));
    model_id = std::string(buffer.begin() + read, model_id_len);
    read += sizeof(size_t);

    size_t name_len = 0;
    std::memcpy((char*)&name_len, buffer.begin() + read, sizeof(size_t));
    std::string name = std::string(buffer.begin() + read, name_len);
    read += sizeof(size_t);

    size_t action_len = 0;
    std::memcpy((char*)&action_len, buffer.begin() + read, sizeof(size_t));
    std::string action = std::string(buffer.begin() + read, action_len);
    read += sizeof(size_t);

    size_t element_len = 0;
    std::memcpy((char*)&element_len, buffer.begin() + read, sizeof(size_t));
    std::string element = std::string(buffer.begin() + read, element_len);
    read += sizeof(size_t);

    size_t secondary_element_len = 0;
    std::memcpy((char*)&secondary_element_len, buffer.begin() + read, sizeof(size_t));
    std::string secondary_element = std::string(buffer.begin() + read, secondary_element_len);
    read += sizeof(size_t);

    auto props = std::make_shared<WebAccounts::CardProperties>();

    std::memcpy((char*)&props->classType, buffer.begin() + read, sizeof(decltype(props->classType)));
    read += sizeof(decltype(props->classType));

    std::memcpy((char*)&props->damage, buffer.begin() + read, sizeof(decltype(props->damage)));
    read += sizeof(decltype(props->damage));

    std::memcpy((char*)&props->timeFreeze, buffer.begin() + read, sizeof(decltype(props->timeFreeze)));
    read += sizeof(decltype(props->timeFreeze));

    std::memcpy((char*)&props->limit, buffer.begin() + read, sizeof(decltype(props->limit)));
    read += sizeof(decltype(props->limit));

    props->id = id;
    props->name = name;
    props->action = action;
    props->canBoost = props->damage > 0;
    props->element = element;
    props->secondaryElement = secondary_element;

    // codes count
    std::vector<char> codes;
    size_t codes_len = 0;
    std::memcpy((char*)&codes_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);

    while (codes_len-- > 0) {
      char code = 0;
      std::memcpy((char*)&code, buffer.begin() + read, 1);
      read += 1;
      codes.push_back(code);
    }

    props->codes = codes;

    // meta classes count
    std::vector<std::string> metaClasses;
    size_t meta_len = 0;
    std::memcpy((char*)&meta_len, buffer.begin() + read, sizeof(size_t));

    while (meta_len-- > 0) {
      size_t meta_i_len = 0;
      std::memcpy((char*)&meta_i_len, buffer.begin() + read, sizeof(size_t));
      read += sizeof(size_t);
      std::string meta = std::string(buffer.begin() + read, meta_i_len);
      metaClasses.push_back(meta);
    }

    props->metaClasses = metaClasses;

    size_t desc_len = 0;
    std::memcpy((char*)&desc_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);
    std::string description = std::string(buffer.begin() + read, desc_len);

    size_t verbose_len = 0;
    std::memcpy((char*)&verbose_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);
    std::string verboseDesc = std::string(buffer.begin() + read, verbose_len);

    props->description = description;
    props->verboseDescription = verboseDesc;

    ////////////////////
    //  Card Data     //
    ////////////////////
    char code = 0;

    std::memcpy((char*)&code, buffer.begin() + read, 1);
    read += 1;

    std::memcpy((char*)&id_len, buffer.begin() + read, sizeof(size_t));
    std::string card_id = std::string(buffer.begin() + read, id_len);
    read += sizeof(size_t);

    std::memcpy((char*)&model_id_len, buffer.begin() + read, sizeof(size_t));
    std::string modelId = std::string(buffer.begin() + read, model_id_len);
    read += sizeof(size_t);

    auto cardData = std::make_shared<WebAccounts::Card>();
    cardData->id = card_id;
    cardData->code = code;
    cardData->modelId = modelId;

    // Upload
    WEBCLIENT.UploadCardData(id, iconObj, textureObj, cardData, props);
    cardsToDownload[id] = "Complete";
  }

  SendDownloadComplete(true);
  // move to the next state
  currState = state::complete;
}

std::vector<std::string> DownloadScene::DeserializeUUIDs(const Poco::Buffer<char>& buffer)
{
  size_t len{};
  size_t read{};
  std::vector<std::string> uuids;

  // list length
  std::memcpy(&len, buffer.begin() + read, sizeof(size_t));
  read += sizeof(size_t);

  while (len > 0) {
    size_t id_len{};
    std::memcpy(&id_len, buffer.begin() + read, sizeof(size_t));
    read += sizeof(size_t);

    uuids.emplace_back(buffer.begin() + read, id_len);
    read += id_len;

    len--;
  }

  return uuids;
}

Poco::Buffer<char> DownloadScene::SerializeUUIDs(NetPlaySignals header, const std::vector<std::string>& uuids)
{
  Poco::Buffer<char> data{ 0 };

  // header
  data.append((char*)&header, sizeof(NetPlaySignals));

  // list length
  size_t len = uuids.size();
  data.append((char*)&len, sizeof(size_t));

  for(auto & id : uuids) {
    // name
    size_t sz = id.length();
    data.append((char*)&sz, sizeof(size_t));
    data.append(id.c_str(), id.length());
  }

  return data;
}

Poco::Buffer<char> DownloadScene::SerializeCards(const std::vector<std::string>& cardList)
{
  Poco::Buffer<char> data{ 0 };
  size_t len = cardList.size();

  // header
  NetPlaySignals type{ NetPlaySignals::card_list_download };
  data.append((char*)&type, sizeof(NetPlaySignals));

  // number of cards
  data.append((char*)&len, sizeof(len));

  for (auto& card : cardList) {
    // name
    size_t sz = card.length();
    data.append((char*)&sz, sizeof(size_t));
    data.append(card.c_str(), card.length());

    // image data
    auto texture = WEBCLIENT.GetIconForCard(card);
    auto image = texture->copyToImage();
    size_t image_len = static_cast<size_t>(image.getSize().x * image.getSize().y * 4u);
    data.append((char*)&image_len, sizeof(size_t));
    data.append((char*)image.getPixelsPtr(), image_len);

    // icon data
    auto icon = WEBCLIENT.GetImageForCard(card);
    image = icon->copyToImage();
    image_len = static_cast<size_t>(image.getSize().x * image.getSize().y * 4u);
    data.append((char*)&image_len, sizeof(size_t));
    data.append((char*)image.getPixelsPtr(), image_len);

    ////////////////////
    // card meta data //
    ////////////////////

    auto cardData = WEBCLIENT.GetWebCard(card);
    auto model = WEBCLIENT.GetWebCardModel(cardData->modelId);

    std::string id = model->id;
    size_t id_len = id.size();
    data.append((char*)&id_len, sizeof(size_t));
    data.append(id.c_str(), id_len);

    std::string name = model->name;
    size_t name_len = name.size();
    data.append((char*)&name_len, sizeof(size_t));
    data.append(name.c_str(), name_len);

    std::string action = model->action;
    size_t action_len = action.size();
    data.append((char*)&action_len, sizeof(size_t));
    data.append(action.c_str(), action_len);

    std::string element = model->element;
    size_t element_len = element.size();
    data.append((char*)&element_len, sizeof(size_t));
    data.append(element.c_str(), element_len);

    std::string secondary = model->secondaryElement;
    size_t secondary_len = secondary.size();
    data.append((char*)&secondary_len, sizeof(size_t));
    data.append(secondary.c_str(), secondary_len);

    data.append((char*)&model->classType, sizeof(decltype(model->classType)));
    data.append((char*)&model->damage, sizeof(decltype(model->damage)));
    data.append((char*)&model->timeFreeze, sizeof(decltype(model->timeFreeze)));
    data.append((char*)&model->limit, sizeof(decltype(model->limit)));

    // codes count
    std::vector<char> codes = model->codes;
    size_t codes_len = codes.size();
    data.append((char*)&codes_len, sizeof(size_t));

    for (auto&& code : codes) {
      data.append((char*)&code, 1);
    }

    // meta classes count
    auto metaClasses = model->metaClasses;
    size_t meta_len = metaClasses.size();
    data.append((char*)&meta_len, sizeof(size_t));

    for (auto&& meta : metaClasses) {
      size_t meta_i_len = meta.size();
      data.append((char*)&meta_i_len, sizeof(size_t));
      data.append(meta.c_str(), meta.size());
    }

    std::string description = model->description;
    size_t desc_len = description.size();

    data.append((char*)&desc_len, sizeof(size_t));
    data.append(description.c_str(), description.size());

    std::string verboseDesc = model->verboseDescription;
    size_t verbose_len = verboseDesc.size();

    data.append((char*)&verbose_len, sizeof(size_t));
    data.append(verboseDesc.c_str(), verboseDesc.size());

    ///////////////
    // Card Data //
    ///////////////
    char code = cardData->code;

    id = cardData->id;
    id_len = id.size();

    std::string modelId = cardData->modelId;
    size_t modelId_len = modelId.size();

    data.append((char*)&code, 1);

    data.append((char*)&id_len, sizeof(size_t));
    data.append((char*)id.c_str(), id_len);

    data.append((char*)&modelId_len, sizeof(size_t));
    data.append((char*)modelId.c_str(), modelId_len);
  }

  return data;
}

void DownloadScene::Abort(const std::vector<std::string>& failed)
{
  Logger::Logf("Aborting");

  if (!aborting) {
    for (auto& uuid : failed) {
      cardsToDownload[uuid] = "Failed";
    }

    SendDownloadComplete(false);
    aborting = true;
  }
}

void DownloadScene::onUpdate(double elapsed)
{
  if (!packetProcessor->IsHandshakeAck() && !aborting) return;

  SendPing();

  if (aborting) {
    abortingCountdown -= from_seconds(elapsed);
    if (abortingCountdown <= frames(0)) {
      // abort match
      using effect = swoosh::types::segue<PixelateBlackWashFade>;
      getController().pop<effect>();
    }

    return;
  }
  else if (downloadSuccess && remoteSuccess) {
    getController().pop();
  }
}

void DownloadScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bg);
  blur.apply(surface);

  float w = static_cast<float>(getController().getVirtualWindowSize().x);
  float h = static_cast<float>(getController().getVirtualWindowSize().y);

  // 1. Draw the state status info
  switch (currState) {
  case state::trade:
    label.SetString("Connecting to other player...");
    break;
  case state::download:
    label.SetString("Downloading, please wait...");
    break;
  case state::complete:
    label.SetString("Complete, waiting...");
    break;
  }

  auto bounds = label.GetLocalBounds();
  label.setOrigin(sf::Vector2f(bounds.width * label.getScale().x , 0));
  label.setPosition(w, 0);
  label.SetColor(sf::Color::White);
  surface.draw(label);

  if (currState < state::download) {
    label.SetString("Request download, please wait...");
    auto bounds = label.GetLocalBounds();
    label.SetColor(sf::Color::White);
    label.setOrigin(sf::Vector2f(0, bounds.height));
    label.setPosition(0, h);
    surface.draw(label);
  }
  else {
    sf::Sprite icon;

    if (cardsToDownload.empty()) {
      label.SetString("Nothing to download.");
      auto bounds = label.GetLocalBounds();
      label.SetColor(sf::Color::White);
      label.setOrigin(sf::Vector2f(0, bounds.height));
      label.setPosition(0, h);
      surface.draw(label);
    }

    for (auto& [key, value] : cardsToDownload) {
      label.SetString(key + " - " + value);

      auto bounds = label.GetLocalBounds();
      float ydiff = bounds.height * label.getScale().y;

      if (value == "Downloading") {
        label.SetColor(sf::Color::White);
      }
      else if (value == "Complete") {
        label.SetColor(sf::Color::Green);
      }
      else {
        label.SetColor(sf::Color::Red);
      }

      if (auto iconTexture = WEBCLIENT.GetIconForCard(key)) {
        icon.setTexture(*iconTexture);
        float iconHeight = icon.getLocalBounds().height;
        icon.setOrigin(0, iconHeight);
      }

      icon.setPosition(sf::Vector2f(bounds.width + 5.0f, bounds.height));
      label.setOrigin(sf::Vector2f(0, bounds.height));
      label.setPosition(0, h);

      h -= ydiff + 5.0f;

      surface.draw(label);
    }
  }
}

void DownloadScene::onLeave()
{
}

void DownloadScene::onExit()
{
}

void DownloadScene::onEnter()
{
}

void DownloadScene::onStart()
{
  TradeCardList(ourCardList);
}

void DownloadScene::onResume()
{
}

void DownloadScene::onEnd()
{
  Net().DropProcessor(packetProcessor);
}
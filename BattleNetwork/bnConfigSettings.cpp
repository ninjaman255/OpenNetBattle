#include "bnConfigSettings.h"
#include "bnLogger.h"
#include "bnInputEvent.h"

/**
 * @brief If config file is ok
 * @return true if wellformed, false otherwise
 */
const bool ConfigSettings::IsOK() const { return isOK; }

/**
 * @brief Check if Audio() is on or off based on ini file
 * @return true if on, false otherwise
 */
const bool ConfigSettings::IsAudioEnabled() const { return (musicLevel || sfxLevel); }

const bool ConfigSettings::IsFullscreen() const
{
    return fullscreen;
}

const int ConfigSettings::GetMusicLevel() const { return musicLevel; }

const int ConfigSettings::GetSFXLevel() const { return sfxLevel; }

const bool ConfigSettings::IsKeyboardOK() const { 
    bool hasUp      = GetPairedInput(InputEvents::pressed_ui_up.name) != sf::Keyboard::Unknown;
    bool hasDown    = GetPairedInput(InputEvents::pressed_ui_down.name) != sf::Keyboard::Unknown;
    bool hasLeft    = GetPairedInput(InputEvents::pressed_ui_left.name) != sf::Keyboard::Unknown;
    bool hasRight   = GetPairedInput(InputEvents::pressed_ui_right.name) != sf::Keyboard::Unknown;
    bool hasConfirm = GetPairedInput(InputEvents::pressed_confirm.name) != sf::Keyboard::Unknown;

    return hasUp && hasDown && hasLeft && hasRight && hasConfirm;
}

void ConfigSettings::SetMusicLevel(int level) { musicLevel = level; }

void ConfigSettings::SetSFXLevel(int level) { sfxLevel = level; }

const std::list<std::string> ConfigSettings::GetPairedActions(const sf::Keyboard::Key& event) const {
  std::list<std::string> list;

  for (auto& [k, v] : keyboard) {
    if (k == event) {
      list.push_back(v);
    }
  }

  return list;
}

const sf::Keyboard::Key ConfigSettings::GetPairedInput(std::string action) const
{
  for (auto& [k, v] : keyboard) {
    std::string value = v;
    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    std::transform(action.begin(), action.end(), action.begin(), ::toupper);

    if (value == action) {
      return k;
    }
  }

  return sf::Keyboard::Unknown;
}

const Gamepad ConfigSettings::GetPairedGamepadButton(std::string action) const
{
  for (auto& [k, v] : gamepad) {
    std::string value = v;
    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    std::transform(action.begin(), action.end(), action.begin(), ::toupper);

    if (value == action) {
      return k;
    }
  }

  return Gamepad::BAD_CODE;
}

const std::list<std::string> ConfigSettings::GetPairedActions(const Gamepad& event) const {
  std::list<std::string> list;

  for (auto& [k, v] : gamepad) {
    if (k == event) {
      list.push_back(v);
    }
  }

  return list;
}

ConfigSettings & ConfigSettings::operator=(const ConfigSettings& rhs)
{
  discord = rhs.discord;
  webServer = rhs.webServer;
  gamepad = rhs.gamepad;
  musicLevel = rhs.musicLevel;
  sfxLevel = rhs.sfxLevel;
  isOK = rhs.isOK;
  keyboard = rhs.keyboard;
  fullscreen = rhs.fullscreen;
  return *this;
}

const DiscordInfo& ConfigSettings::GetDiscordInfo() const
{
  return discord;
}

const WebServerInfo& ConfigSettings::GetWebServerInfo() const
{
  return webServer;
}

void ConfigSettings::SetKeyboardHash(const KeyboardHash key)
{
  keyboard = key;
}

void ConfigSettings::SetGamepadHash(const GamepadHash gamepad)
{
  ConfigSettings::gamepad = gamepad;
}

ConfigSettings::ConfigSettings(const ConfigSettings & rhs)
{
  this->operator=(rhs);
}

ConfigSettings::ConfigSettings()
{
  isOK = false;
}

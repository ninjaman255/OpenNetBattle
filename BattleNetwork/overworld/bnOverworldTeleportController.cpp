#define _USE_MATH_DEFINES
#include <cmath>

#include "bnOverworldTeleportController.h"
#include "bnOverworldActor.h"
#include "../bnShaderResourceManager.h"
#include "../bnAudioResourceManager.h"

Overworld::TeleportController::TeleportController()
{
  beam.setTexture(TEXTURES.LoadTextureFromFile("resources/ow/teleport.png"));
  beamAnim = Animation("resources/ow/teleport.animation");
  beamAnim.SetFrame(0, beam.getSprite());
}

Overworld::TeleportController::Command& Overworld::TeleportController::TeleportOut(Actor& actor)
{
  this->actor = &actor;
  this->actor->Hide();

  auto onStart = [=] {
    if (!mute) {
      AUDIO.Play(AudioType::AREA_GRAB);
    }
  };

  auto onFinish = [=] {
    this->animComplete = true;
  };

  this->animComplete = false;
  this->beamAnim << "TELEPORT_OUT" << Animator::On(1, onStart) << onFinish;
  this->beamAnim.Refresh(this->beam.getSprite());
  this->beam.setPosition(actor.getPosition());

  this->sequence.push(Command{ Command::state::teleport_out });
  return this->sequence.back();
}

Overworld::TeleportController::Command& Overworld::TeleportController::TeleportIn(Actor& actor, const sf::Vector2f& start, Direction dir)
{
  auto onStart = [=] {
    if (!mute) {
      AUDIO.Play(AudioType::AREA_GRAB);
    }
  };

  auto onSpin = [=] {
    this->actor->Reveal();
    this->actor->SetShader(SHADERS.GetShader(ShaderType::ADDITIVE));
    this->actor->setColor(sf::Color::Cyan);
    this->spin = true;
    this->spinProgress = 0;
  };

  auto onFinish = [=] {
    this->animComplete = true;
    this->actor->RevokeShader();
    this->actor->setColor(sf::Color::White);
  };

  this->walkFrames = frames(50);
  this->startDir = dir;
  this->startPos = start;
  this->actor = &actor;
  this->animComplete = this->walkoutComplete = this->spin = false;
  this->beamAnim << "TELEPORT_IN" << Animator::On(2, onStart) << Animator::On(4, onSpin) << onFinish;
  this->beamAnim.Refresh(this->beam.getSprite());
  actor.Hide();
  actor.setPosition(start);
  this->beam.setPosition(start);

  this->sequence.push(Command{ Command::state::teleport_in });
  return this->sequence.back();
}

void Overworld::TeleportController::Update(double elapsed)
{
  this->beamAnim.Update(static_cast<float>(elapsed), beam.getSprite());

  if (sequence.empty()) return;

  auto& next = sequence.front();
  if (next.state == Command::state::teleport_in) {
    if (animComplete) {
      if (walkFrames > frames(0) && this->startDir != Direction::none) {
        // walk out for 50 frames
        actor->Walk(this->startDir);
        walkFrames -= frame_time_t::from_seconds(elapsed);
      }
      else {
        this->walkoutComplete = true;
        next.onFinish();
        sequence.pop();
      }
    }
    else if (spin) {
      constexpr float _2pi = 2.0f * M_PI;
      constexpr float spin_frames = _2pi/0.25f;
      float progress = spin_frames * static_cast<float>(elapsed);
      spinProgress += progress;

      auto cyan = sf::Color::Cyan;
      sf::Uint8 alpha = static_cast<sf::Uint8>(255 * (1.0f - (spinProgress / spin_frames)));
      this->actor->setColor({ cyan.r, cyan.g, cyan.b, alpha });

      actor->Face(Actor::MakeDirectionFromVector({ std::cos(spinProgress), std::sin(spinProgress) }, 0.5f));
    }
  }
  else {
    // Command::state::teleport_out
    if (animComplete) {
      next.onFinish();
      sequence.pop();
    }
  }
}

const bool Overworld::TeleportController::IsComplete() const
{
  if (sequence.empty()) return true;

  if (sequence.front().state == Command::state::teleport_in) {
    return walkoutComplete;
  }

  return animComplete;
}

SpriteProxyNode& Overworld::TeleportController::GetBeam()
{
  return beam;
}

void Overworld::TeleportController::EnableSound(bool enable)
{
  mute = !enable;
}
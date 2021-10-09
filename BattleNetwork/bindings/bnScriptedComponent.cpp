#include "bnScriptedComponent.h"
#include "../bnCharacter.h"
#include "../bnSolHelpers.h"
#include "bnWeakWrapper.h"

ScriptedComponent::ScriptedComponent(std::weak_ptr<Character> owner, Component::lifetimes lifetime) :
    Component(owner, lifetime)
{
}

ScriptedComponent::~ScriptedComponent()
{
}

void ScriptedComponent::Init() {
  weakWrap = WeakWrapper(weak_from_base<ScriptedComponent>());
}

void ScriptedComponent::OnUpdate(double dt)
{
  if (entries["update_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "update_func", weakWrap, dt);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedComponent::Inject(BattleSceneBase& scene)
{
  if (entries["scene_inject_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "scene_inject_func", weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }

  // the component is now injected into the scene's update loop
  // because the character's update loop is only called when they are on the field
  // this way the timer can keep ticking

  scene.Inject(shared_from_this());
}

std::shared_ptr<Character> ScriptedComponent::GetOwnerAsCharacter()
{
  return this->GetOwnerAs<Character>();
}

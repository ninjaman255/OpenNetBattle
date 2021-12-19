#pragma once
#ifdef BN_MOD_SUPPORT

#include "bnLogger.h"

#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <atomic>
#include <filesystem>
#include <list>
#include "stx/result.h"

#ifdef __unix__
#define LUA_USE_POSIX 1
#endif

#include <sol/sol.hpp>
#include "bnPackageManager.h"

class CardPackagePartitioner;

class ScriptResourceManager {
public:
  struct LoadScriptResult {
    sol::protected_function_result result;
    sol::state* state{ nullptr };
  };

private:
  unsigned int randSeed{};
  std::vector<sol::state*> states;
  std::map<std::string, LoadScriptResult> scriptTableHash; /*!< Script path to sol table hash */
  std::map<PackageAddress, std::string> cardFQN; /*! character FQN to script path */
  std::map<PackageAddress, std::string> characterFQN; /*! character FQN to script path */
  std::map<PackageAddress, std::string> libraryFQN; /*! library FQN to script path */
  std::map<sol::state*, std::string> state2Namespace; /*! pointer to namespace hash */
  std::map<PackageAddress, std::list< std::string > > scriptDependencies; // [ Package Name, List of packages it depends on ] 
  CardPackagePartitioner* cardPartition{ nullptr };

  void ConfigureEnvironment(sol::state& state);

public:
  ~ScriptResourceManager();

  LoadScriptResult& LoadScript(const std::string& namespaceId, const std::filesystem::path& path);
  LoadScriptResult& InitLibrary(const std::string& namespaceId, const std::string& path);

  void DropPackageData(const PackageAddress& addr);
  void DefineCard(const std::string& namespaceId, const std::string& fqn, const std::string& path) /* throw std::exception */;
  void DefineCharacter(const std::string& namespaceId, const std::string& fqn, const std::string& path) /* throw std::exception */;
  void DefineLibrary(const std::string& namespaceId, const std::string& fqn, const std::string& path) /* throw std::exception */;
  sol::state* FetchCharacter(const std::string& namespaceId, const std::string& fqn);
  sol::state* FetchCard(const std::string& namespaceId, const std::string& fqn);
  const std::string& FetchSharedLibraryPath(const std::string& namespaceId, const std::string& fqn);
  const std::string& CharacterToModpath(const std::string& namespaceId, const std::string& fqn);
  void SeedRand(unsigned int seed);
  void SetCardPackagePartitioner(CardPackagePartitioner& partition);
  CardPackagePartitioner& GetCardPackagePartitioner();
  void AddDependencyNote(sol::state& state, const std::string& dependencyPackageID );
  void RegisterDependencyNotes(sol::state& state);
  std::string GetStateNamespace(sol::state& state);

  static sol::object PrintInvalidAccessMessage(sol::table table, const std::string typeName, const std::string key );
  static sol::object PrintInvalidAssignMessage(sol::table table, const std::string typeName, const std::string key );

  private:
  void SetSystemFunctions(sol::state& state );
  void SetModPathVariable(sol::state& state, const std::filesystem::path& modDirectory);

  static std::string GetCurrentLine( lua_State* L );
  static stx::result_t<std::string> GetCurrentFile(lua_State* L);
  static stx::result_t<std::string> GetCurrentFolder(lua_State* L);
};

#endif
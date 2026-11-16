/*
 * ClientLibraries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class GameLibrary;
class ModManager;
class ModSnapshot;

/// Owns mod manager of the client and the library of the active preset while a session library is current
class ClientLibraries : boost::noncopyable
{
	std::unique_ptr<ModManager> modManager;
	/// Library of the active preset while a session library is current, null while it is current itself
	std::unique_ptr<const GameLibrary> homeLibrary;

	std::atomic<bool> returnRequested = false;

	std::thread worker;
	std::unique_ptr<GameLibrary> loadedLibrary;
	std::string loadingError;

	void loadLibrary(const ModSnapshot & mods);
	void finishReload(const std::function<void(bool success)> & onFinished);
	void switchLibrary(std::unique_ptr<const GameLibrary> library);

public:
	explicit ClientLibraries(std::unique_ptr<ModManager> modManager);
	~ClientLibraries();

	ModManager & mods();

	/// Returns true if current library is not the library of the active preset
	bool isSessionLibrary() const;

	/// Replaces current library with a library of provided mod list, then calls onFinished on main thread. Pregame only
	void reloadLibrary(const ModSnapshot & mods, const std::function<void(bool success)> & onFinished);

	/// Switches back to library of the active preset without loading. Main thread, pregame only
	void returnToHomeLibrary();

	/// Requests return to library of the active preset once the main menu is the top window again, if a session library is current
	void requestReturnToHomeLibrary();
	bool isReturnRequested() const;
};

/*
 * ClientLibraries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ClientLibraries.h"

#include "GameEngine.h"
#include "GameInstance.h"
#include "CServerHandler.h"
#include "gui/WindowHandler.h"
#include "mainmenu/CMainMenu.h"
#include "windows/InfoWindows.h"

#include "../lib/CStopWatch.h"
#include "../lib/CThreadHelper.h"
#include "../lib/ExceptionsCommon.h"
#include "../lib/GameLibrary.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModManager.h"
#include "../lib/modding/ModSnapshot.h"
#include "../lib/texts/MetaString.h"

ClientLibraries::ClientLibraries(std::unique_ptr<ModManager> modManager)
	: modManager(std::move(modManager))
{
}

ClientLibraries::~ClientLibraries()
{
	// loading can not be interrupted
	if(worker.joinable())
		worker.join();
}

ModManager & ClientLibraries::mods()
{
	return *modManager;
}

bool ClientLibraries::isSessionLibrary() const
{
	return homeLibrary != nullptr;
}

void ClientLibraries::loadLibrary(const ModSnapshot & mods)
{
	try
	{
		CStopWatch timer;
		auto library = std::make_unique<GameLibrary>();
		library->initializeFilesystem(mods);
		library->initializeLibrary();
		loadedLibrary = std::move(library);
		logGlobal->info("Game library reloaded: %d ms", timer.getDiff());
	}
	catch(const DataLoadingException & e)
	{
		loadingError = e.what();
		logGlobal->error("Failed to reload game library: %s", loadingError);
	}
}

void ClientLibraries::reloadLibrary(const ModSnapshot & mods, const std::function<void(bool success)> & onFinished)
{
	assert(!worker.joinable());
	assert(!GAME->server().client);

	if(homeLibrary && homeLibrary->modh->getModList().hasSameActiveMods(mods))
	{
		returnToHomeLibrary();
		onFinished(true);
		return;
	}

	loadedLibrary.reset();
	loadingError.clear();

	if(!ENGINE)
	{
		loadLibrary(mods);
		finishReload(onFinished);
		return;
	}

	ENGINE->windows().clear();
	ENGINE->windows().createAndPushWindow<CLoadingScreen>();

	worker = std::thread([this, mods, onFinished]()
	{
		setThreadName("reloadLibrary");
		loadLibrary(mods);
		ENGINE->dispatchMainThread([this, onFinished]()
		{
			worker.join();
			finishReload(onFinished);
		});
	});
}

void ClientLibraries::finishReload(const std::function<void(bool success)> & onFinished)
{
	if(!loadedLibrary)
	{
		if(ENGINE)
			ENGINE->windows().clear();

		// caller reopens its interface, the error is shown on top of it
		onFinished(false);

		if(ENGINE)
		{
			MetaString message;
			message.appendTextID("vcmi.client.errors.modReloadFailure");
			message.replaceRawString(loadingError);
			CInfoWindow::showInfoDialog(message.toString(&GAME->translator()), {});
		}
		return;
	}

	modManager->setValidatedChecksums(loadedLibrary->modh->getValidatedChecksums());
	switchLibrary(std::move(loadedLibrary));
	onFinished(true);
	GAME->checkForModLoadingFailure();
}

void ClientLibraries::switchLibrary(std::unique_ptr<const GameLibrary> library)
{
	const bool isHome = library->modh->getModList().hasSameActiveMods(modManager->createSnapshot());

	GAME->releaseLibraryClientState();
	auto previous = setGameLibrary(std::move(library));
	GAME->createLibraryClientState();

	// previous library is kept only if it is the home library and the new one is not
	if(isHome)
		homeLibrary.reset();
	else if(!homeLibrary)
		homeLibrary = std::move(previous);
}

void ClientLibraries::requestReturnToHomeLibrary()
{
	returnRequested = isSessionLibrary();
}

bool ClientLibraries::isReturnRequested() const
{
	return returnRequested;
}

void ClientLibraries::returnToHomeLibrary()
{
	returnRequested = false;
	if(!homeLibrary)
		return;

	switchLibrary(std::move(homeLibrary));
}

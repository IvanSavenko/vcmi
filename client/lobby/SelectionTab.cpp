/*
 * SelectionTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SelectionTab.h"
#include "CSelectionBase.h"
#include "CLobbyScreen.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../windows/CMapOverview.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

#include "../../CCallback.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/GameSettings.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/TerrainHandler.h"

bool mapSorter::operator()(const ElementInfo & aaa, const ElementInfo & bbb)
{
	if(aaa.isFolder || bbb.isFolder)
	{
		if(aaa.isFolder != bbb.isFolder)
			return (aaa.isFolder > bbb.isFolder);
		else
		{
			if(boost::algorithm::starts_with(aaa.folderName, "..") || boost::algorithm::starts_with(bbb.folderName, ".."))
				return boost::algorithm::starts_with(aaa.folderName, "..");
			return boost::ilexicographical_compare(aaa.folderName, bbb.folderName);
		}
	}

	auto a = aaa.map->mapHeader.get();
	auto b = bbb.map->mapHeader.get();
	if(a && b) //if we are sorting scenarios
	{
		switch(sortBy)
		{
		case _format: //by map format (RoE, WoG, etc)
			return (a->version < b->version);
			break;
		case _loscon: //by loss conditions
			return (a->defeatIconIndex < b->defeatIconIndex);
			break;
		case _playerAm: //by player amount
			int playerAmntB;
			int humenPlayersB;
			int playerAmntA;
			int humenPlayersA;
			playerAmntB = humenPlayersB = playerAmntA = humenPlayersA = 0;
			for(int i = 0; i < 8; i++)
			{
				if(a->players[i].canHumanPlay)
				{
					playerAmntA++;
					humenPlayersA++;
				}
				else if(a->players[i].canComputerPlay)
				{
					playerAmntA++;
				}
				if(b->players[i].canHumanPlay)
				{
					playerAmntB++;
					humenPlayersB++;
				}
				else if(b->players[i].canComputerPlay)
				{
					playerAmntB++;
				}
			}
			if(playerAmntB != playerAmntA)
				return (playerAmntA < playerAmntB);
			else
				return (humenPlayersA < humenPlayersB);
			break;
		case _size: //by size of map
			return (a->width < b->width);
			break;
		case _viccon: //by victory conditions
			return (a->victoryIconIndex < b->victoryIconIndex);
			break;
		case _name: //by name
			return boost::ilexicographical_compare(a->name.toString(), b->name.toString());
		case _fileName: //by filename
			return boost::ilexicographical_compare(aaa.map->fileURI, bbb.map->fileURI);
		case _changeDate: //by changedate
			return aaa.map->lastWrite < bbb.map->lastWrite;
		default:
			return boost::ilexicographical_compare(a->name.toString(), b->name.toString());
		}
	}
	else //if we are sorting campaigns
	{
		switch(sortBy)
		{
		case _numOfMaps: //by number of maps in campaign
			return aaa.map->campaign->scenariosCount() < bbb.map->campaign->scenariosCount();
		case _name: //by name
			return boost::ilexicographical_compare(aaa.map->campaign->getNameTranslated(), bbb.map->campaign->getNameTranslated());
		default:
			return boost::ilexicographical_compare(aaa.map->campaign->getNameTranslated(), bbb.map->campaign->getNameTranslated());
		}
	}
}

// pick sorting order based on selection
static ESortBy getSortBySelectionScreen(ESelectionScreen Type)
{
	switch(Type)
	{
	case ESelectionScreen::newGame:
		return ESortBy::_name;
	case ESelectionScreen::loadGame:
	case ESelectionScreen::saveGame:
		return ESortBy::_fileName;
	case ESelectionScreen::campaignList:
		return ESortBy::_name;
	}
	// Should not reach here. But let's not crash the game.
	return ESortBy::_name;
}

SelectionTab::SelectionTab(ESelectionScreen Type)
	: CIntObject(LCLICK | SHOW_POPUP | KEYBOARD | DOUBLECLICK), callOnSelect(nullptr), tabType(Type), selectionPos(0), sortModeAscending(true), inputNameRect{32, 539, 350, 20}, curFolder(""), currentMapSizeFilter(0), showRandom(false)
{
	OBJ_CONSTRUCTION;
		
	generalSortingBy = getSortBySelectionScreen(tabType);
	sortingBy = _format;

	bool enableUiEnhancements = settings["general"]["enableUiEnhancements"].Bool();

	if(tabType != ESelectionScreen::campaignList)
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("SCSELBCK.bmp"), 0, 6);
		pos = background->pos;
		inputName = std::make_shared<CTextInput>(inputNameRect, Point(-32, -25), ImagePath::builtin("GSSTRIP.bmp"));
		inputName->setFilterFilename();
		labelMapSizes = std::make_shared<CLabel>(87, 62, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[510]);

		// TODO: Global constants?
		int sizes[] = {CMapHeader::MAP_SIZE_SMALL,
						CMapHeader::MAP_SIZE_MIDDLE,
						CMapHeader::MAP_SIZE_LARGE,
						CMapHeader::MAP_SIZE_XLARGE,
						0};
		const char * filterIconNmes[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
		for(int i = 0; i < 5; i++)
			buttonsSortBy.push_back(std::make_shared<CButton>(Point(158 + 47 * i, 46), AnimationPath::builtin(filterIconNmes[i]), CGI->generaltexth->zelp[54 + i], std::bind(&SelectionTab::filter, this, sizes[i], true)));

		int xpos[] = {23, 55, 88, 121, 306, 339};
		const char * sortIconNames[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
		for(int i = 0; i < 6; i++)
		{
			ESortBy criteria = (ESortBy)i;
			if(criteria == _name)
				criteria = generalSortingBy;

			buttonsSortBy.push_back(std::make_shared<CButton>(Point(xpos[i], 86), AnimationPath::builtin(sortIconNames[i]), CGI->generaltexth->zelp[107 + i], std::bind(&SelectionTab::sortBy, this, criteria)));
		}
	}

	int positionsToShow = 18;
	std::string tabTitle;
	switch(tabType)
	{
	case ESelectionScreen::newGame:
		tabTitle = CGI->generaltexth->arraytxt[229];
		break;
	case ESelectionScreen::loadGame:
		tabTitle = CGI->generaltexth->arraytxt[230];
		break;
	case ESelectionScreen::saveGame:
		positionsToShow = 16;
		tabTitle = CGI->generaltexth->arraytxt[231];
		break;
	case ESelectionScreen::campaignList:
		tabTitle = CGI->generaltexth->allTexts[726];
		setRedrawParent(true); // we use parent background so we need to make sure it's will be redrawn too
		pos.w = parent->pos.w;
		pos.h = parent->pos.h;
		pos.x += 3;
		pos.y += 6;

		buttonsSortBy.push_back(std::make_shared<CButton>(Point(23, 86), AnimationPath::builtin("CamCusM.DEF"), CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _numOfMaps)));
		buttonsSortBy.push_back(std::make_shared<CButton>(Point(55, 86), AnimationPath::builtin("CamCusL.DEF"), CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _name)));
		break;
	default:
		assert(0);
		break;
	}

	if(enableUiEnhancements)
	{
		auto sortByDate = std::make_shared<CButton>(Point(371, 85), AnimationPath::builtin("selectionTabSortDate"), CButton::tooltip("", CGI->generaltexth->translate("vcmi.lobby.sortDate")), std::bind(&SelectionTab::sortBy, this, ESortBy::_changeDate));
		sortByDate->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/selectionTabSortDate")));
		buttonsSortBy.push_back(sortByDate);
	}

	iconsMapFormats = GH.renderHandler().loadAnimation(AnimationPath::builtin("SCSELC.DEF"));
	iconsVictoryCondition = GH.renderHandler().loadAnimation(AnimationPath::builtin("SCNRVICT.DEF"));
	iconsLossCondition = GH.renderHandler().loadAnimation(AnimationPath::builtin("SCNRLOSS.DEF"));
	for(int i = 0; i < positionsToShow; i++)
		listItems.push_back(std::make_shared<ListItem>(Point(30, 129 + i * 25), iconsMapFormats, iconsVictoryCondition, iconsLossCondition));

	labelTabTitle = std::make_shared<CLabel>(205, 28, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, tabTitle);
	slider = std::make_shared<CSlider>(Point(372, 86 + (enableUiEnhancements ? 30 : 0)), (tabType != ESelectionScreen::saveGame ? 480 : 430) - (enableUiEnhancements ? 30 : 0), std::bind(&SelectionTab::sliderMove, this, _1), positionsToShow, (int)curItems.size(), 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(24);

	// create scroll bounds that encompass all area in this UI element to the left of slider (including area of slider itself)
	// entire screen can't be used in here since map description might also have slider
	slider->setScrollBounds(Rect(pos.x - slider->pos.x, 0, slider->pos.x + slider->pos.w - pos.x, slider->pos.h ));
	filter(0);
}

void SelectionTab::toggleMode()
{
	allItems.clear();
	curItems.clear();
	if(slider)
		slider->block(true);

	if(CSH->isHost())
	{
		switch(tabType)
		{
		case ESelectionScreen::newGame:
		case ESelectionScreen::loadGame:
			inputName->disable();
			break;

		case ESelectionScreen::saveGame:
			inputName->enable();
			inputName->activate();
			break;

		case ESelectionScreen::campaignList:
			break;

		default:
			assert(0);
			break;
		}
		if(slider)
		{
			slider->block(false);
			filter(0);
		}

		if(CSH->campaignStateToSend)
		{
			CSH->setCampaignState(CSH->campaignStateToSend);
			CSH->campaignStateToSend.reset();
		}
	}
	slider->setAmount((int)curItems.size());
	updateListItems();
	redraw();
}

void SelectionTab::clickReleased(const Point & cursorPosition)
{
	int line = getLine();

	if(line != -1)
	{
		select(line);
	}
#ifdef VCMI_MOBILE
	// focus input field if clicked inside it
	else if(inputName && inputName->isActive() && inputNameRect.isInside(cursorPosition))
		inputName->giveFocus();
#endif

}

void SelectionTab::keyPressed(EShortcut key)
{
	int moveBy = 0;
	switch(key)
	{
	case EShortcut::MOVE_UP:
		moveBy = -1;
		break;
	case EShortcut::MOVE_DOWN:
		moveBy = +1;
		break;
	case EShortcut::MOVE_PAGE_UP:
		moveBy = -(int)listItems.size() + 1;
		break;
	case EShortcut::MOVE_PAGE_DOWN:
		moveBy = +(int)listItems.size() - 1;
		break;
	case EShortcut::MOVE_FIRST:
		select(-slider->getValue());
		return;
	case EShortcut::MOVE_LAST:
		select((int)curItems.size() - slider->getValue());
		return;
	default:
		return;
	}
	select((int)selectionPos - slider->getValue() + moveBy);
}

void SelectionTab::clickDouble(const Point & cursorPosition)
{
	int position = getLine();
	int itemIndex = position + slider->getValue();

	if(itemIndex >= curItems.size())
		return;

	if (itemIndex != selectionPos)
	{
		// double-click BUT player hit different item than he had selected
		// ignore - clickReleased would still trigger and update selection.
		// After which another (3rd) click if it happens would still register as double-click
		return;
	}

	if(itemIndex >= 0 && curItems[itemIndex].isFolder)
	{
		select(position);
		return;
	}

	if(getLine() != -1) //double clicked scenarios list
	{
		(static_cast<CLobbyScreen *>(parent))->buttonStart->clickPressed(cursorPosition);
		(static_cast<CLobbyScreen *>(parent))->buttonStart->clickReleased(cursorPosition);
	}
}

void SelectionTab::showPopupWindow(const Point & cursorPosition)
{
	int position = getLine();
	int py = position + slider->getValue();

	if(py >= curItems.size())
		return;

	if(!curItems[py].isFolder)
		GH.windows().createAndPushWindow<CMapOverview>(curItems[py].map->getNameTranslated(), curItems[py].map->fullFileURI, curItems[py].map->date, ResourcePath(curItems[py].map->fileURI), tabType);
	else
		CRClickPopup::createAndPush(curItems[py].folderName);
}

auto SelectionTab::checkSubfolder(std::string path)
{
	struct Ret
	{
		std::string folderName;
		std::string baseFolder;
		bool parentExists;
		bool fileInFolder;
	} ret;

	ret.parentExists = (curFolder != "");
	ret.fileInFolder = false;

	std::vector<std::string> filetree;
	// delete first element (e.g. 'MAPS')
	boost::split(filetree, path, boost::is_any_of("/"));
	filetree.erase(filetree.begin());
	std::string pathWithoutPrefix = boost::algorithm::join(filetree, "/");

	if(!filetree.empty())
	{
		filetree.pop_back();
		ret.baseFolder = boost::algorithm::join(filetree, "/");
	}
	else
		ret.baseFolder = "";

	if(boost::algorithm::starts_with(ret.baseFolder, curFolder))
	{
		std::string folder = ret.baseFolder.substr(curFolder.size());

		if(folder != "")
		{
			boost::split(filetree, folder, boost::is_any_of("/"));
			ret.folderName = filetree[0];
		}
	}

	if(boost::algorithm::starts_with(pathWithoutPrefix, curFolder))
		if(boost::count(pathWithoutPrefix.substr(curFolder.size()), '/') == 0)
			ret.fileInFolder = true;

	return ret;
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter(int size, bool selectFirst)
{
	if(size == -1)
		size = currentMapSizeFilter;
	currentMapSizeFilter = size;

	curItems.clear();

	for(auto elem : allItems)
	{
		if((elem.map->mapHeader && (!size || elem.map->mapHeader->width == size)) || tabType == ESelectionScreen::campaignList)
		{
			if(showRandom)
				curFolder = "RANDOMMAPS/";

			auto [folderName, baseFolder, parentExists, fileInFolder] = checkSubfolder(elem.map->originalFileURI);

			if((showRandom && baseFolder != "RANDOMMAPS") || (!showRandom && baseFolder == "RANDOMMAPS"))
				continue;

			if(parentExists && !showRandom)
			{
				ElementInfo folder;
				folder.isFolder = true;
				folder.folderName = "..     (" + curFolder + ")";
				auto itemIt = boost::range::find_if(curItems, [](const ElementInfo & e) { return boost::starts_with(e.folderName, ".."); });
				if (itemIt == curItems.end()) {
					curItems.push_back(folder);
				}			
			}

			ElementInfo folder;
			folder.isFolder = true;
			folder.folderName = folderName;
			auto itemIt = boost::range::find_if(curItems, [folder](const ElementInfo & e) { return e.folderName == folder.folderName; });
			if (itemIt == curItems.end() && folderName != "") {
				curItems.push_back(folder);
			}

			if(fileInFolder)
				curItems.push_back(elem);
		}
	}

	if(curItems.size())
	{
		slider->block(false);
		slider->setAmount((int)curItems.size());
		sort();
		if(selectFirst)
		{
			int firstPos = boost::range::find_if(curItems, [](const ElementInfo & e) { return !e.isFolder; }) - curItems.begin();
			if(firstPos < curItems.size())
			{
				slider->scrollTo(firstPos);
				callOnSelect(curItems[firstPos].map);
				selectAbs(firstPos);
			}
		}
	}
	else
	{
		updateListItems();
		redraw();
		slider->block(true);
		if(callOnSelect)
			callOnSelect(nullptr);
	}
}

void SelectionTab::sortBy(int criteria)
{
	if(criteria == sortingBy)
	{
		sortModeAscending = !sortModeAscending;
	}
	else
	{
		sortingBy = (ESortBy)criteria;
		sortModeAscending = true;
	}
	sort();

	selectAbs(-1);
}

void SelectionTab::sort()
{
	if(sortingBy != generalSortingBy)
		std::stable_sort(curItems.begin(), curItems.end(), mapSorter(generalSortingBy));
	std::stable_sort(curItems.begin(), curItems.end(), mapSorter(sortingBy));

	int firstMapIndex = boost::range::find_if(curItems, [](const ElementInfo & e) { return !e.isFolder; }) - curItems.begin();
	if(!sortModeAscending)
		std::reverse(std::next(curItems.begin(), firstMapIndex), curItems.end());

	updateListItems();
	redraw();
}

void SelectionTab::select(int position)
{
	if(!curItems.size())
		return;

	// New selection. py is the index in curItems.
	int py = position + slider->getValue();
	vstd::amax(py, 0);
	vstd::amin(py, curItems.size() - 1);

	selectionPos = py;

	if(position < 0)
		slider->scrollBy(position);
	else if(position >= listItems.size())
		slider->scrollBy(position - (int)listItems.size() + 1);

	if(curItems[py].isFolder) {
		if(boost::starts_with(curItems[py].folderName, ".."))
		{
			std::vector<std::string> filetree;
			boost::split(filetree, curFolder, boost::is_any_of("/"));
			filetree.pop_back();
			filetree.pop_back();
			curFolder = filetree.size() > 0 ? boost::algorithm::join(filetree, "/") + "/" : "";
		}
		else
			curFolder += curItems[py].folderName + "/";
		filter(-1);
		slider->scrollTo(0);

		int firstPos = boost::range::find_if(curItems, [](const ElementInfo & e) { return !e.isFolder; }) - curItems.begin();
		if(firstPos < curItems.size())
		{
			selectAbs(firstPos);
		}

		return;
	}

	rememberCurrentSelection();

	if(inputName && inputName->isActive())
	{
		auto filename = *CResourceHandler::get()->getResourceName(ResourcePath(curItems[py].map->fileURI, EResType::SAVEGAME));
		inputName->setText(filename.stem().string());
	}

	updateListItems();
	redraw();
	if(callOnSelect)
		callOnSelect(curItems[py].map);
}

void SelectionTab::selectAbs(int position)
{
	if(position == -1)
		position = boost::range::find_if(curItems, [](const ElementInfo & e) { return !e.isFolder; }) - curItems.begin();
	select(position - slider->getValue());
}

void SelectionTab::sliderMove(int slidPos)
{
	if(!slider)
		return; // ignore spurious call when slider is being created
	updateListItems();
	redraw();
}

void SelectionTab::updateListItems()
{
	// elemIdx is the index of the maps or saved game to display on line 0
	// slider->capacity contains the number of available screen lines
	// slider->positionsAmnt is the number of elements after filtering
	int elemIdx = slider->getValue();
	for(auto item : listItems)
	{
		if(elemIdx < curItems.size())
		{
			item->updateItem(curItems[elemIdx], elemIdx == selectionPos);
			elemIdx++;
		}
		else
		{
			item->updateItem();
		}
	}
}

bool SelectionTab::receiveEvent(const Point & position, int eventType) const
{
	// FIXME: widget should instead have well-defined pos so events will be filtered using standard routine
	return getLine(position - pos.topLeft()) != -1;
}

int SelectionTab::getLine() const
{
	Point clickPos = GH.getCursorPosition() - pos.topLeft();
	return getLine(clickPos);
}

int SelectionTab::getLine(const Point & clickPos) const
{
	int line = -1;

	// Ignore clicks on save name area
	int maxPosY;
	if(tabType == ESelectionScreen::saveGame)
		maxPosY = 516;
	else
		maxPosY = 564;

	if(clickPos.y > 115 && clickPos.y < maxPosY && clickPos.x > 22 && clickPos.x < 371)
	{
		line = (clickPos.y - 115) / 25; //which line
	}

	return line;
}

void SelectionTab::selectFileName(std::string fname)
{
	boost::to_upper(fname);

	for(int i = (int)allItems.size() - 1; i >= 0; i--)
	{
		if(allItems[i].map->fileURI == fname)
		{
			auto [folderName, baseFolder, parentExists, fileInFolder] = checkSubfolder(allItems[i].map->originalFileURI);
			curFolder = baseFolder != "" ? baseFolder + "/" : "";
		}
	}

	for(int i = (int)curItems.size() - 1; i >= 0; i--)
	{
		if(curItems[i].map->fileURI == fname)
		{
			slider->scrollTo(i);
			selectAbs(i);
			return;
		}
	}

	filter(-1);
	selectAbs(-1);

	if(tabType == ESelectionScreen::saveGame && inputName->getText().empty())
		inputName->setText("NEWGAME");
}

std::shared_ptr<CMapInfo> SelectionTab::getSelectedMapInfo() const
{
	return curItems.empty() || curItems[selectionPos].isFolder ? nullptr : curItems[selectionPos].map;
}

void SelectionTab::rememberCurrentSelection()
{
	if(getSelectedMapInfo() == nullptr)
		return;

	// TODO: this can be more elegant
	if(tabType == ESelectionScreen::newGame)
	{
		Settings lastMap = settings.write["general"]["lastMap"];
		lastMap->String() = getSelectedMapInfo()->fileURI;
	}
	else if(tabType == ESelectionScreen::loadGame)
	{
		Settings lastSave = settings.write["general"]["lastSave"];
		lastSave->String() = getSelectedMapInfo()->fileURI;
	}
	else if(tabType == ESelectionScreen::campaignList)
	{
		Settings lastCampaign = settings.write["general"]["lastCampaign"];
		lastCampaign->String() = getSelectedMapInfo()->fileURI;
	}
}

SelectionTab::ListItem::ListItem(Point position, std::shared_ptr<CAnimation> iconsFormats, std::shared_ptr<CAnimation> iconsVictory, std::shared_ptr<CAnimation> iconsLoss)
	: CIntObject(LCLICK, position)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pictureEmptyLine = std::make_shared<CPicture>(GH.renderHandler().loadImage(ImagePath::builtin("camcust")), Rect(25, 121, 349, 26), -8, -14);
	labelName = std::make_shared<CLabel>(184, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "", 185);
	labelName->setAutoRedraw(false);
	labelAmountOfPlayers = std::make_shared<CLabel>(8, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelAmountOfPlayers->setAutoRedraw(false);
	labelNumberOfCampaignMaps = std::make_shared<CLabel>(8, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelNumberOfCampaignMaps->setAutoRedraw(false);
	labelMapSizeLetter = std::make_shared<CLabel>(41, 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	labelMapSizeLetter->setAutoRedraw(false);
	// FIXME: This -12 should not be needed, but for some reason CAnimImage displaced otherwise
	iconFolder = std::make_shared<CPicture>(ImagePath::builtin("lobby/iconFolder.png"), -8, -12);
	iconFormat = std::make_shared<CAnimImage>(iconsFormats, 0, 0, 59, -12);
	iconVictoryCondition = std::make_shared<CAnimImage>(iconsVictory, 0, 0, 277, -12);
	iconLossCondition = std::make_shared<CAnimImage>(iconsLoss, 0, 0, 310, -12);
}

void SelectionTab::ListItem::updateItem(const ElementInfo & info, bool selected)
{
	auto color = selected ? Colors::YELLOW : Colors::WHITE;
	if(info.isFolder)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->enable();
		pictureEmptyLine->enable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->disable();
		labelName->enable();
		labelName->setMaxWidth(316);
		labelName->setText(info.folderName);
		labelName->setColor(color);
		return;
	}

	if(!info.map)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->disable();
		pictureEmptyLine->disable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->disable();
		labelName->disable();
		return;
	}

	labelName->enable();
	if(info.map->campaign)
	{
		labelAmountOfPlayers->disable();
		labelMapSizeLetter->disable();
		iconFolder->disable();
		pictureEmptyLine->disable();
		iconFormat->disable();
		iconVictoryCondition->disable();
		iconLossCondition->disable();
		labelNumberOfCampaignMaps->enable();
		std::ostringstream ostr(std::ostringstream::out);
		ostr << info.map->campaign->scenariosCount();
		labelNumberOfCampaignMaps->setText(ostr.str());
		labelNumberOfCampaignMaps->setColor(color);
		labelName->setMaxWidth(316);
	}
	else
	{
		labelNumberOfCampaignMaps->disable();
		std::ostringstream ostr(std::ostringstream::out);
		ostr << info.map->amountOfPlayersOnMap << "/" << info.map->amountOfHumanControllablePlayers;
		labelAmountOfPlayers->enable();
		labelAmountOfPlayers->setText(ostr.str());
		labelAmountOfPlayers->setColor(color);
		labelMapSizeLetter->enable();
		labelMapSizeLetter->setText(info.map->getMapSizeName());
		labelMapSizeLetter->setColor(color);
		iconFolder->disable();
		pictureEmptyLine->disable();
		iconFormat->enable();
		iconFormat->setFrame(info.map->getMapSizeFormatIconId());
		iconVictoryCondition->enable();
		iconVictoryCondition->setFrame(info.map->mapHeader->victoryIconIndex, 0);
		iconLossCondition->enable();
		iconLossCondition->setFrame(info.map->mapHeader->defeatIconIndex, 0);
		labelName->setMaxWidth(185);
	}
	labelName->setText(info.map->getNameForList());
	labelName->setColor(color);
}

void SelectionTab::processMapList(std::vector<std::shared_ptr<CMapInfo>> mapList)
{
	for(const auto & item : mapList)
		allItems.emplace_back(item);

	filter(-1, true);
}

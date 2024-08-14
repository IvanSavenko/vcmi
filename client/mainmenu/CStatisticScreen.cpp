/*
 * CStatisticScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CStatisticScreen.h"
#include "../CGameInfo.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../eventsSDL/InputHandler.h"
#include "../gui/Shortcut.h"

#include "../render/Graphics.h"
#include "../render/IImage.h"

#include "../widgets/ComboBox.h"
#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"
#include "../widgets/Slider.h"

#include "../../lib/gameState/GameStatistics.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"

#include <vstd/DateUtils.h>

CStatisticScreen::CStatisticScreen(StatisticDataSet stat)
	: CWindowObject(BORDERED), statistic(stat)
{
	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 800, 600));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));

	contentArea = Rect(10, 40, 780, 510);
	layout.push_back(std::make_shared<CLabel>(400, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->translate("vcmi.statisticWindow.statistic")));
	layout.push_back(std::make_shared<TransparentFilledRectangle>(contentArea, ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 80, 128, 255), 1));
	layout.push_back(std::make_shared<CButton>(Point(725, 558), AnimationPath::builtin("MUBCHCK"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT));

	buttonSelect = std::make_shared<CToggleButton>(Point(10, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){
		std::vector<std::string> texts;
		for(auto & val : contentInfo)
			texts.push_back(CGI->generaltexth->translate(std::get<0>(val.second)));
		GH.windows().createAndPushWindow<StatisticSelector>(texts, [this](int selectedIndex)
		{
			OBJECT_CONSTRUCTION;
			if(!std::get<1>(contentInfo[(Content)selectedIndex]))
				mainContent = getContent((Content)selectedIndex, EGameResID::NONE);
			else
			{
				auto content = (Content)selectedIndex;
				auto possibleRes = std::vector<EGameResID>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS};
				std::vector<std::string> resourceText;
				for(auto & res : possibleRes)
					resourceText.push_back(CGI->generaltexth->translate(TextIdentifier("core.restypes", res.getNum()).get()));
				
				GH.windows().createAndPushWindow<StatisticSelector>(resourceText, [this, content, possibleRes](int selectedIndex)
				{
					OBJECT_CONSTRUCTION;
					mainContent = getContent(content, possibleRes[selectedIndex]);
				});
			}
		});
	});
	buttonSelect->setTextOverlay(CGI->generaltexth->translate("vcmi.statisticWindow.selectView"), EFonts::FONT_SMALL, Colors::YELLOW);

	buttonCsvSave = std::make_shared<CToggleButton>(Point(150, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){ GH.input().copyToClipBoard(statistic.toCsv("\t"));	});
	buttonCsvSave->setTextOverlay(CGI->generaltexth->translate("vcmi.statisticWindow.tsvCopy"), EFonts::FONT_SMALL, Colors::YELLOW);

	mainContent = getContent(OVERVIEW, EGameResID::NONE);
}

TData CStatisticScreen::extractData(StatisticDataSet stat, std::function<float(StatisticDataSetEntry val)> selector)
{
	auto tmpData = stat.data;
	std::sort(tmpData.begin(), tmpData.end(), [](StatisticDataSetEntry v1, StatisticDataSetEntry v2){ return v1.player == v2.player ? v1.day < v2.day : v1.player < v2.player; });

	PlayerColor tmpColor = PlayerColor::NEUTRAL;
	std::vector<float> tmpColorSet;
	TData plotData;
	for(auto & val : tmpData)
	{
		if(tmpColor != val.player)
		{
			if(tmpColorSet.size())
			{
				plotData.emplace(graphics->playerColors[tmpColor.getNum()], std::vector<float>(tmpColorSet));
				tmpColorSet.clear();
			}

			tmpColor = val.player;
		}
		if(val.status == EPlayerStatus::INGAME)
			tmpColorSet.push_back(selector(val));
	}
	if(tmpColorSet.size())
		plotData.emplace(graphics->playerColors[tmpColor.getNum()], std::vector<float>(tmpColorSet));

	return plotData;
}

std::shared_ptr<CIntObject> CStatisticScreen::getContent(Content c, EGameResID res)
{
	TData plotData;
	TIcons icons;

	switch (c)
	{
	case OVERVIEW:
		return std::make_shared<OverviewPanel>(contentArea.resize(-15), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), statistic);
	
	case CHART_RESOURCES:
		plotData = extractData(statistic, [res](StatisticDataSetEntry val) -> float { return val.resources[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", res.getNum()).get()), plotData, icons, 0);
	
	case CHART_INCOME:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.income; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_HEROES:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.numberHeroes; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_TOWNS:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.numberTowns; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_ARTIFACTS:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.numberArtifacts; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_DWELLINGS:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.numberDwellings; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_MINES:
		plotData = extractData(statistic, [res](StatisticDataSetEntry val) -> float { return val.numMines[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", res.getNum()).get()), plotData, icons, 0);
	
	case CHART_ARMY_STRENGTH:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.armyStrength; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_EXPERIENCE:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.totalExperience; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_RESOURCES_SPENT_ARMY:
		plotData = extractData(statistic, [res](StatisticDataSetEntry val) -> float { return val.spentResourcesForArmy[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", res.getNum()).get()), plotData, icons, 0);
	
	case CHART_RESOURCES_SPENT_BUILDINGS:
		plotData = extractData(statistic, [res](StatisticDataSetEntry val) -> float { return val.spentResourcesForBuildings[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", res.getNum()).get()), plotData, icons, 0);
	
	case CHART_MAP_EXPLORED:
		plotData = extractData(statistic, [](StatisticDataSetEntry val) -> float { return val.mapExploredRatio; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 1);
	}

	return nullptr;
}

StatisticSelector::StatisticSelector(std::vector<std::string> texts, std::function<void(int selectedIndex)> cb)
	: CWindowObject(BORDERED | NEEDS_ANIMATED_BACKGROUND), texts(texts), cb(cb)
{
	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 128 + 16, std::min((int)texts.size(), LINES) * 40));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));

	slider = std::make_shared<CSlider>(Point(pos.w - 16, 0), pos.h, [this](int to){ update(to); redraw(); }, LINES, texts.size(), 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(40);
	slider->setScrollBounds(Rect(-pos.w + slider->pos.w, 0, pos.w, pos.h));

	update(0);
}

void StatisticSelector::update(int to)
{
	OBJECT_CONSTRUCTION;
	buttons.clear();
	for(int i = to; i < LINES + to; i++)
	{
		if(i>=texts.size())
			continue;

		auto button = std::make_shared<CToggleButton>(Point(0, 10 + (i - to) * 40), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this, i](bool on){ close(); cb(i); });
		button->setTextOverlay(texts[i], EFonts::FONT_SMALL, Colors::WHITE);
		buttons.push_back(button);
	}
}

OverviewPanel::OverviewPanel(Rect position, std::string title, StatisticDataSet stat)
	: CIntObject(), data(stat)
{
	OBJECT_CONSTRUCTION;

	pos = position + pos.topLeft();

	layout.push_back(std::make_shared<CLabel>(pos.w / 2, 10, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, title));

	canvas = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, Y_OFFS, pos.w - 16, pos.h - Y_OFFS));

	dataExtract = {
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.playerName"), [this](PlayerColor color){
				return playerDataFilter(color).front().playerName;
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.daysSurvived"), [this](PlayerColor color){
				return std::to_string(playerDataFilter(color).size());
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.maxHeroLevel"), [this](PlayerColor color){
				int maxLevel = 0;
				for(auto val : playerDataFilter(color))
					if(maxLevel < val.maxHeroLevel)
						maxLevel = val.maxHeroLevel;
				return std::to_string(maxLevel);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.battleWinRatioHero"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				if(!val.numBattlesPlayer)
					return std::string("");
				float tmp = ((float)val.numWinBattlesPlayer / (float)val.numBattlesPlayer) * 100;
				return std::to_string((int)tmp) + " %";
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.battleWinRatioNeutral"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				if(!val.numWinBattlesNeutral)
					return std::string("");
				float tmp = ((float)val.numWinBattlesNeutral / (float)val.numBattlesNeutral) * 100;
				return std::to_string((int)tmp) + " %";
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.battlesHero"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.numBattlesPlayer);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.battlesNeutral"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.numBattlesNeutral);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.obeliskVisited"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string((int)(val.obeliskVisitedRatio * 100)) + " %";
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.maxArmyStrength"), [this](PlayerColor color){
				int maxArmyStrength = 0;
				for(auto val : playerDataFilter(color))
					if(maxArmyStrength < val.armyStrength)
						maxArmyStrength = val.armyStrength;
				return TextOperations::formatMetric(maxArmyStrength, 6);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", EGameResID::GOLD).get()), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::GOLD]);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", EGameResID::WOOD).get()), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::WOOD]);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", EGameResID::MERCURY).get()), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::MERCURY]);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", EGameResID::ORE).get()), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::ORE]);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", EGameResID::SULFUR).get()), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::SULFUR]);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", EGameResID::CRYSTAL).get()), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::CRYSTAL]);
			}
		},
		{
			CGI->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + CGI->generaltexth->translate(TextIdentifier("core.restypes", EGameResID::GEMS).get()), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::GEMS]);
			}
		},
	};

	int usedLines = dataExtract.size();

	slider = std::make_shared<CSlider>(Point(pos.w - 16, Y_OFFS), pos.h - Y_OFFS, [this](int to){ update(to); setRedrawParent(true); redraw(); }, LINES - 1, usedLines, 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(canvas->pos.h / LINES);
	slider->setScrollBounds(Rect(-pos.w + slider->pos.w, 0, pos.w, canvas->pos.h));

	fieldSize = Point(canvas->pos.w / (graphics->playerColors.size() + 2), canvas->pos.h / LINES);
	for(int x = 0; x < graphics->playerColors.size() + 1; x++)
		for(int y = 0; y < LINES; y++)
		{
			int xStart = (x + (x == 0 ? 0 : 1)) * fieldSize.x;
			int yStart = y * fieldSize.y;
			if(x == 0 || y == 0)
				canvas->addBox(Point(xStart, yStart), Point(x == 0 ? 2 * fieldSize.x : fieldSize.x, fieldSize.y), ColorRGBA(0, 0, 0, 100));
			canvas->addRectangle(Point(xStart, yStart), Point(x == 0 ? 2 * fieldSize.x : fieldSize.x, fieldSize.y), ColorRGBA(127, 127, 127, 255));
		}

	update(0);
}

std::vector<StatisticDataSetEntry> OverviewPanel::playerDataFilter(PlayerColor color)
{
	std::vector<StatisticDataSetEntry> tmpData;
	std::copy_if(data.data.begin(), data.data.end(), std::back_inserter(tmpData), [color](StatisticDataSetEntry e){ return e.player == color; });
	return tmpData;
}

void OverviewPanel::update(int to)
{
	OBJECT_CONSTRUCTION;

	content.clear();
	for(int y = to; y < LINES - 1 + to; y++)
	{
		if(y >= dataExtract.size())
			continue;

		for(int x = 0; x < PlayerColor::PLAYER_LIMIT_I + 1; x++)
		{
			if(y == to && x < PlayerColor::PLAYER_LIMIT_I)
				content.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ITGFLAGS"), x, 0, 180 + x * fieldSize.x, 35));
			int xStart = (x + (x == 0 ? 0 : 1)) * fieldSize.x + (x == 0 ? fieldSize.x : (fieldSize.x / 2));
			int yStart = Y_OFFS + (y + 1 - to) * fieldSize.y + (fieldSize.y / 2);
			PlayerColor tmpColor(x - 1);
			if(playerDataFilter(tmpColor).size() || x == 0)
				content.push_back(std::make_shared<CLabel>(xStart, yStart, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, (x == 0 ? dataExtract[y].first : dataExtract[y].second(tmpColor)), x == 0 ? (fieldSize.x * 2) : fieldSize.x));
		}
	}
}

LineChart::LineChart(Rect position, std::string title, TData data, TIcons icons, float maxY)
	: CIntObject(), maxVal(0), maxDay(0)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | MOVE);

	pos = position + pos.topLeft();

	layout.push_back(std::make_shared<CLabel>(pos.w / 2, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, title));

	chartArea = pos.resize(-50);
	chartArea.moveTo(Point(50, 50));

	canvas = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, 0, pos.w, pos.h));

	statusBar = CGStatusBar::create(0, 0, ImagePath::builtin("radialMenu/statusBar"));
	((std::shared_ptr<CIntObject>)statusBar)->setEnabled(false);

	// additional calculations
	bool skipMaxValCalc = maxY > 0;
	maxVal = maxY;
	for(auto & line : data)
	{
		for(auto & val : line.second)
			if(maxVal < val && !skipMaxValCalc)
				maxVal = val;
		if(maxDay < line.second.size())
			maxDay = line.second.size();
	}

	// draw
	for(auto & line : data)
	{
		Point lastPoint = Point(-1, -1);
		for(int i = 0; i < line.second.size(); i++)
		{
			float x = ((float)chartArea.w / (float)(maxDay-1)) * (float)i;
			float y = (float)chartArea.h - ((float)chartArea.h / maxVal) * line.second[i];
			Point p = Point(x, y) + chartArea.topLeft();

			if(lastPoint.x != -1)
				canvas->addLine(lastPoint, p, line.first);

			lastPoint = p;
		}
	}

	// Axis
	canvas->addLine(chartArea.topLeft() + Point(0, -10), chartArea.topLeft() + Point(0, chartArea.h + 10), Colors::WHITE);
	canvas->addLine(chartArea.topLeft() + Point(-10, chartArea.h), chartArea.topLeft() + Point(chartArea.w + 10, chartArea.h), Colors::WHITE);

	Point p = chartArea.topLeft() + Point(-5, chartArea.h + 10);
	layout.push_back(std::make_shared<CLabel>(p.x, p.y, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::WHITE, "0"));
	p = chartArea.topLeft() + Point(chartArea.w + 10, chartArea.h + 10);
	layout.push_back(std::make_shared<CLabel>(p.x, p.y, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(maxDay)));
	p = chartArea.topLeft() + Point(-5, -10);
	layout.push_back(std::make_shared<CLabel>(p.x, p.y, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::WHITE, std::to_string((int)maxVal)));
}

void LineChart::updateStatusBar(const Point & cursorPosition)
{
	statusBar->moveTo(cursorPosition + Point(-statusBar->pos.w / 2, 20));
	Rect r(pos.x + chartArea.x, pos.y + chartArea.y, chartArea.w, chartArea.h);
	statusBar->setEnabled(r.isInside(cursorPosition));
	if(r.isInside(cursorPosition))
	{
		float x = ((float)maxDay / (float)chartArea.w) * ((float)cursorPosition.x - (float)r.x) + 1.0f;
		float y = maxVal - ((float)maxVal / (float)chartArea.h) * ((float)cursorPosition.y - (float)r.y);
		statusBar->write(CGI->generaltexth->translate("core.genrltxt.64") + ": " + std::to_string((int)x) + "   " + CGI->generaltexth->translate("vcmi.statisticWindow.value") + ": " + ((int)y > 0 ? std::to_string((int)y) : std::to_string(y)));
	}
	GH.windows().totalRedraw();
}

void LineChart::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	updateStatusBar(cursorPosition);
}

void LineChart::clickPressed(const Point & cursorPosition)
{
	updateStatusBar(cursorPosition);
}

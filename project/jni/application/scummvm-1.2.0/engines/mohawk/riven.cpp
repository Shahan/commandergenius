/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-2-0/engines/mohawk/riven.cpp $
 * $Id: riven.cpp 52692 2010-09-12 21:35:49Z criezy $
 *
 */

#include "common/config-manager.h"
#include "common/events.h"
#include "common/EventRecorder.h"
#include "common/keyboard.h"
#include "common/translation.h"

#include "mohawk/graphics.h"
#include "mohawk/resource.h"
#include "mohawk/riven.h"
#include "mohawk/riven_external.h"
#include "mohawk/riven_saveload.h"
#include "mohawk/dialogs.h"
#include "mohawk/video.h"

namespace Mohawk {

Common::Rect *g_atrusJournalRect1;
Common::Rect *g_atrusJournalRect2;
Common::Rect *g_cathJournalRect2;
Common::Rect *g_atrusJournalRect3;
Common::Rect *g_cathJournalRect3;
Common::Rect *g_trapBookRect3;
Common::Rect *g_demoExitRect;

MohawkEngine_Riven::MohawkEngine_Riven(OSystem *syst, const MohawkGameDescription *gamedesc) : MohawkEngine(syst, gamedesc) {
	_showHotspots = false;
	_cardData.hasData = false;
	_gameOver = false;
	_activatedSLST = false;
	_ignoreNextMouseUp = false;
	_extrasFile = NULL;
	_curStack = aspit;
	_hotspots = NULL;

	// NOTE: We can never really support CD swapping. All of the music files
	// (*_Sounds.mhk) are stored on disc 1. They are copied to the hard drive
	// during install and used from there. The same goes for the extras.mhk
	// file. The following directories allow Riven to be played directly
	// from the DVD.

	const Common::FSNode gameDataDir(ConfMan.get("path"));
	SearchMan.addSubDirectoryMatching(gameDataDir, "all");
	SearchMan.addSubDirectoryMatching(gameDataDir, "data");
	SearchMan.addSubDirectoryMatching(gameDataDir, "exe");
	SearchMan.addSubDirectoryMatching(gameDataDir, "assets1");

	g_atrusJournalRect1 = new Common::Rect(295, 402, 313, 426);
	g_atrusJournalRect2 = new Common::Rect(259, 402, 278, 426);
	g_cathJournalRect2 = new Common::Rect(328, 408, 348, 419);
	g_atrusJournalRect3 = new Common::Rect(222, 402, 240, 426);
	g_cathJournalRect3 = new Common::Rect(291, 408, 311, 419);
	g_trapBookRect3 = new Common::Rect(363, 396, 386, 432);
	g_demoExitRect = new Common::Rect(291, 408, 317, 419);
}

MohawkEngine_Riven::~MohawkEngine_Riven() {
	delete _gfx;
	delete _console;
	delete _externalScriptHandler;
	delete _extrasFile;
	delete _saveLoad;
	delete _scriptMan;
	delete[] _vars;
	delete _optionsDialog;
	delete _rnd;
	delete[] _hotspots;
	delete g_atrusJournalRect1;
	delete g_atrusJournalRect2;
	delete g_cathJournalRect2;
	delete g_atrusJournalRect3;
	delete g_cathJournalRect3;
	delete g_trapBookRect3;
	delete g_demoExitRect;
}

GUI::Debugger *MohawkEngine_Riven::getDebugger() {
	return _console;
}

Common::Error MohawkEngine_Riven::run() {
	MohawkEngine::run();

	_gfx = new RivenGraphics(this);
	_console = new RivenConsole(this);
	_saveLoad = new RivenSaveLoad(this, _saveFileMan);
	_externalScriptHandler = new RivenExternal(this);
	_optionsDialog = new RivenOptionsDialog(this);
	_scriptMan = new RivenScriptManager(this);

	_rnd = new Common::RandomSource();
	g_eventRec.registerRandomSource(*_rnd, "riven");

	initVars();

	// Open extras.mhk for common images
	_extrasFile = new MohawkArchive();
	_extrasFile->open("extras.mhk");

	// Start at main cursor
	_gfx->changeCursor(kRivenMainCursor);

	// Let's begin, shall we?
	if (getFeatures() & GF_DEMO) {
		// Start the demo off with the videos
		changeToStack(aspit);
		changeToCard(6);
	} else if (ConfMan.hasKey("save_slot")) {
		// Load game from launcher/command line if requested
		uint32 gameToLoad = ConfMan.getInt("save_slot");
		Common::StringArray savedGamesList = _saveLoad->generateSaveGameList();
		if (gameToLoad > savedGamesList.size())
			error ("Could not find saved game");
		_saveLoad->loadGame(savedGamesList[gameToLoad]);
	} else {
		// Otherwise, start us off at aspit's card 1 (the main menu)
        changeToStack(aspit);
		changeToCard(1);
	}

	
	while (!_gameOver && !shouldQuit())
		handleEvents();

	return Common::kNoError;
}

void MohawkEngine_Riven::handleEvents() {
	Common::Event event;

	// Update background videos and the water effect
	bool needsUpdate = _gfx->runScheduledWaterEffects();
	needsUpdate |= _video->updateBackgroundMovies();

	while (_eventMan->pollEvent(event)) {
		switch (event.type) {
		case Common::EVENT_MOUSEMOVE:
			checkHotspotChange();

			if (!(getFeatures() & GF_DEMO)) {
				// Check to show the inventory, but it is always "showing" in the demo
				if (_eventMan->getMousePos().y >= 392)
					_gfx->showInventory();
				else
					_gfx->hideInventory();
			}

			needsUpdate = true;
			break;
		case Common::EVENT_LBUTTONDOWN:
			if (_curHotspot >= 0)
				runHotspotScript(_curHotspot, kMouseDownScript);
			break;
		case Common::EVENT_LBUTTONUP:
			// See RivenScript::switchCard() for more information on why we sometimes
			// disable the next up event.
			if (!_ignoreNextMouseUp) {
				if (_curHotspot >= 0)
					runHotspotScript(_curHotspot, kMouseUpScript);
				else
					checkInventoryClick();
			}
			_ignoreNextMouseUp = false;
			break;
		case Common::EVENT_KEYDOWN:
			switch (event.kbd.keycode) {
			case Common::KEYCODE_d:
				if (event.kbd.flags & Common::KBD_CTRL) {
					_console->attach();
					_console->onFrame();
				}
				break;
			case Common::KEYCODE_SPACE:
				pauseGame();
				break;
			case Common::KEYCODE_F4:
				_showHotspots = !_showHotspots;
				if (_showHotspots) {
					for (uint16 i = 0; i < _hotspotCount; i++)
						_gfx->drawRect(_hotspots[i].rect, _hotspots[i].enabled);
					needsUpdate = true;
				} else
					refreshCard();
				break;
			case Common::KEYCODE_F5:
				runDialog(*_optionsDialog);
				updateZipMode();
				break;
			case Common::KEYCODE_r:
				// Return to the main menu in the demo on ctrl+r
				if (event.kbd.flags & Common::KBD_CTRL && getFeatures() & GF_DEMO) {
					if (_curStack != aspit)
						changeToStack(aspit);
					changeToCard(1);
				}
				break;
			case Common::KEYCODE_p:
				// Play the intro videos in the demo on ctrl+p
				if (event.kbd.flags & Common::KBD_CTRL && getFeatures() & GF_DEMO) {
					if (_curStack != aspit)
						changeToStack(aspit);
					changeToCard(6);
				}
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	if (_curHotspot >= 0)
		runHotspotScript(_curHotspot, kMouseInsideScript);

	// Update the screen if we need to
	if (needsUpdate)
		_system->updateScreen();

	// Cut down on CPU usage
	_system->delayMillis(10);
}

// Stack/Card-Related Functions

void MohawkEngine_Riven::changeToStack(uint16 n) {
	// The endings are in reverse order because of the way the 1.02 patch works.
	// The only "Data3" file is j_Data3.mhk from that patch. Patch files have higher
	// priorities over the regular files and are therefore loaded and checked first.
	static const char *endings[] = { "_Data3.mhk", "_Data2.mhk", "_Data1.mhk", "_Data.mhk", "_Sounds.mhk" };

	// Don't change stack to the current stack (if the files are loaded)
	if (_curStack == n && !_mhk.empty())
		return;

	_curStack = n;

	// Stop any videos playing
	_video->stopVideos();
	_video->clearMLST();

	// Clear the old stack files out
	for (uint32 i = 0; i < _mhk.size(); i++)
		delete _mhk[i];
	_mhk.clear();

	// Get the prefix character for the destination stack
	char prefix = getStackName(_curStack)[0];

	// Load any file that fits the patterns
	for (int i = 0; i < ARRAYSIZE(endings); i++) {
		Common::String filename = Common::String(prefix) + endings[i];
		if (Common::File::exists(filename)) {
			MohawkArchive *mhk = new MohawkArchive();
			mhk->open(filename);
			_mhk.push_back(mhk);
		}
	}

	// Make sure we have loaded files
	if (_mhk.empty())
		error("Could not load stack %s", getStackName(_curStack).c_str());

	// Stop any currently playing sounds
	_sound->stopAllSLST();
}

// Riven uses some hacks to change stacks for linking books
// Otherwise, script command 27 changes stacks
struct RivenSpecialChange {
	byte startStack;
	uint32 startCardRMAP;
	byte targetStack;
	uint32 targetCardRMAP;
} rivenSpecialChange[] = {
	{ aspit,  0x1f04, ospit,  0x44ad },		// Trap Book
	{ bspit, 0x1c0e7, ospit,  0x2e76 },		// Dome Linking Book
	{ gspit, 0x111b1, ospit,  0x2e76 },		// Dome Linking Book
	{ jspit, 0x28a18, rspit,   0xf94 },		// Tay Linking Book
	{ jspit, 0x26228, ospit,  0x2e76 },		// Dome Linking Book
	{ ospit,  0x5f0d, pspit,  0x3bf0 },		// Return from 233rd Age
	{ ospit,  0x470a, jspit, 0x1508e },		// Return from 233rd Age
	{ ospit,  0x5c52, gspit, 0x10bea },		// Return from 233rd Age
	{ ospit,  0x5d68, bspit, 0x1adfd },		// Return from 233rd Age
	{ ospit,  0x5e49, tspit,   0xe87 },		// Return from 233rd Age
	{ pspit,  0x4108, ospit,  0x2e76 },		// Dome Linking Book
	{ rspit,  0x32d8, jspit, 0x1c474 },		// Return from Tay
	{ tspit, 0x21b69, ospit,  0x2e76 }		// Dome Linking Book
};

void MohawkEngine_Riven::changeToCard(uint16 dest) {
	_curCard = dest;
	debug (1, "Changing to card %d", _curCard);

	if (!(getFeatures() & GF_DEMO)) {
		for (byte i = 0; i < 13; i++)
			if (_curStack == rivenSpecialChange[i].startStack && _curCard == matchRMAPToCard(rivenSpecialChange[i].startCardRMAP)) {
				changeToStack(rivenSpecialChange[i].targetStack);
				_curCard = matchRMAPToCard(rivenSpecialChange[i].targetCardRMAP);
			}
	}

	if (_cardData.hasData)
		runCardScript(kCardLeaveScript);

	loadCard(_curCard);
	refreshCard(); // Handles hotspots and scripts
}

void MohawkEngine_Riven::refreshCard() {
	loadHotspots(_curCard);

	_gfx->_updatesEnabled = true;
	_gfx->clearWaterEffects();
	_gfx->_activatedPLSTs.clear();
	_video->stopVideos();
	_gfx->drawPLST(1);
	_activatedSLST = false;

	runCardScript(kCardLoadScript);
	_gfx->updateScreen();
	runCardScript(kCardOpenScript);

	// Activate the first sound list if none have been activated
	if (!_activatedSLST)
		_sound->playSLST(1, _curCard);

	if (_showHotspots) {
		for (uint16 i = 0; i < _hotspotCount; i++)
			_gfx->drawRect(_hotspots[i].rect, _hotspots[i].enabled);
	}

	// Now we need to redraw the cursor if necessary and handle mouse over scripts
	_curHotspot = -1;
	checkHotspotChange();
}

void MohawkEngine_Riven::loadCard(uint16 id) {
	// NOTE: The card scripts are cleared by the RivenScriptManager automatically.

	Common::SeekableReadStream* inStream = getRawData(ID_CARD, id);

	_cardData.name = inStream->readSint16BE();
	_cardData.zipModePlace = inStream->readUint16BE();
	_cardData.scripts = _scriptMan->readScripts(inStream);
	_cardData.hasData = true;

	delete inStream;

	if (_cardData.zipModePlace) {
		Common::String cardName = getName(CardNames, _cardData.name);
		if (cardName.empty())
			return;
		ZipMode zip;
		zip.name = cardName;
		zip.id = id;
		if (!(Common::find(_zipModeData.begin(), _zipModeData.end(), zip) != _zipModeData.end()))
			_zipModeData.push_back(zip);
	}
}

void MohawkEngine_Riven::loadHotspots(uint16 id) {
	// Clear old hotspots
	delete[] _hotspots;
	
	// NOTE: The hotspot scripts are cleared by the RivenScriptManager automatically.

	Common::SeekableReadStream *inStream = getRawData(ID_HSPT, id);

	_hotspotCount = inStream->readUint16BE();
	_hotspots = new RivenHotspot[_hotspotCount];

	for (uint16 i = 0; i < _hotspotCount; i++) {
		_hotspots[i].enabled = true;

		_hotspots[i].blstID = inStream->readUint16BE();
		_hotspots[i].name_resource = inStream->readSint16BE();

		int16 left = inStream->readSint16BE();
		int16 top = inStream->readSint16BE();
		int16 right = inStream->readSint16BE();
		int16 bottom = inStream->readSint16BE();

		// Riven has some invalid rects, disable them here
		// Known weird hotspots:
		// - tspit 371 (DVD: 377), hotspot 4
		if (left >= right || top >= bottom) {
			warning("%s %d hotspot %d is invalid: (%d, %d, %d, %d)", getStackName(_curStack).c_str(), _curCard, i, left, top, right, bottom);
			left = top = right = bottom = 0; 	 
			_hotspots[i].enabled = 0; 	 
		}

		_hotspots[i].rect = Common::Rect(left, top, right, bottom);

		_hotspots[i].u0 = inStream->readUint16BE();
		_hotspots[i].mouse_cursor = inStream->readUint16BE();
		_hotspots[i].index = inStream->readUint16BE();
		_hotspots[i].u1 = inStream->readSint16BE();
		_hotspots[i].zipModeHotspot = inStream->readUint16BE();

		// Read in the scripts now
		_hotspots[i].scripts = _scriptMan->readScripts(inStream);
	}

	delete inStream;
	updateZipMode();
}

void MohawkEngine_Riven::updateZipMode() {
	// Check if a zip mode hotspot is enabled by checking the name/id against the ZIPS records.

	for (uint32 i = 0; i < _hotspotCount; i++) {
		if (_hotspots[i].zipModeHotspot) {
			if (*getVar("azip") != 0) {
				// Check if a zip mode hotspot is enabled by checking the name/id against the ZIPS records.
				Common::String hotspotName = getName(HotspotNames, _hotspots[i].name_resource);

				bool foundMatch = false;

				if (!hotspotName.empty())
					for (uint16 j = 0; j < _zipModeData.size(); j++)
						if (_zipModeData[j].name == hotspotName) {
							foundMatch = true;
							break;
						}

				_hotspots[i].enabled = foundMatch;
			} else // Disable the hotspot if zip mode is disabled
				_hotspots[i].enabled = false;
		}
	}
}

void MohawkEngine_Riven::checkHotspotChange() {
	uint16 hotspotIndex = 0;
	bool foundHotspot = false;
	for (uint16 i = 0; i < _hotspotCount; i++)
		if (_hotspots[i].enabled && _hotspots[i].rect.contains(_eventMan->getMousePos())) {
			foundHotspot = true;
			hotspotIndex = i;
		}

	if (foundHotspot) {
		if (_curHotspot != hotspotIndex) {
			_curHotspot = hotspotIndex;
			_gfx->changeCursor(_hotspots[_curHotspot].mouse_cursor);
		}
	} else {
		_curHotspot = -1;
		_gfx->changeCursor(kRivenMainCursor);
	}
}

Common::String MohawkEngine_Riven::getHotspotName(uint16 hotspot) {
	assert(hotspot < _hotspotCount);

	if (_hotspots[hotspot].name_resource < 0)
		return Common::String();

	return getName(HotspotNames, _hotspots[hotspot].name_resource);
}

void MohawkEngine_Riven::checkInventoryClick() {
	Common::Point mousePos = _eventMan->getMousePos();

	// Don't even bother. We're not in the inventory portion of the screen.
	if (mousePos.y < 392)
		return;

	// In the demo, check if we've clicked the exit button
	if (getFeatures() & GF_DEMO) {
		if (g_demoExitRect->contains(mousePos)) {
			if (_curStack == aspit && _curCard == 1) {
				// From the main menu, go to the "quit" screen
				changeToCard(12);
			} else if (_curStack == aspit && _curCard == 12) {
				// From the "quit" screen, just quit
				_gameOver = true;
			} else {
				// Otherwise, return to the main menu
				if (_curStack != aspit)
					changeToStack(aspit);
				changeToCard(1);
			}
		}
		return;
	}

	// No inventory shown on aspit
	if (_curStack == aspit)
		return;

	// Set the return stack/card id's.
	*getVar("returnstackid") = _curStack;
	*getVar("returncardid") = _curCard;

	// See RivenGraphics::showInventory() for an explanation
	// of the variables' meanings.
	bool hasCathBook = *getVar("acathbook") != 0;
	bool hasTrapBook = *getVar("atrapbook") != 0;

	// Go to the book if a hotspot contains the mouse
	if (!hasCathBook) {
		if (g_atrusJournalRect1->contains(mousePos)) {
			_gfx->hideInventory();
			changeToStack(aspit);
			changeToCard(5);
		}
	} else if (!hasTrapBook) {
		if (g_atrusJournalRect2->contains(mousePos)) {
			_gfx->hideInventory();
			changeToStack(aspit);
			changeToCard(5);
		} else if (g_cathJournalRect2->contains(mousePos)) {
			_gfx->hideInventory();
			changeToStack(aspit);
			changeToCard(6);
		}
	} else {
		if (g_atrusJournalRect3->contains(mousePos)) {
			_gfx->hideInventory();
			changeToStack(aspit);
			changeToCard(5);
		} else if (g_cathJournalRect3->contains(mousePos)) {
			_gfx->hideInventory();
			changeToStack(aspit);
			changeToCard(6);
		} else if (g_trapBookRect3->contains(mousePos)) {
			_gfx->hideInventory();
			changeToStack(aspit);
			changeToCard(7);
		}
	}
}

Common::SeekableReadStream *MohawkEngine_Riven::getExtrasResource(uint32 tag, uint16 id) {
	return _extrasFile->getRawData(tag, id);
}

Common::String MohawkEngine_Riven::getName(uint16 nameResource, uint16 nameID) {
	Common::SeekableReadStream* nameStream = getRawData(ID_NAME, nameResource);
	uint16 fieldCount = nameStream->readUint16BE();
	uint16* stringOffsets = new uint16[fieldCount];
	Common::String name;
	char c;

	if (nameID < fieldCount) {
		for (uint16 i = 0; i < fieldCount; i++)
			stringOffsets[i] = nameStream->readUint16BE();
		for (uint16 i = 0; i < fieldCount; i++)
			nameStream->readUint16BE();	// Skip unknown values

		nameStream->seek(stringOffsets[nameID], SEEK_CUR);
		c = (char)nameStream->readByte();

		while (c) {
			name += c;
			c = (char)nameStream->readByte();
		}
	}

	delete nameStream;
	delete [] stringOffsets;
	return name;
}

uint16 MohawkEngine_Riven::matchRMAPToCard(uint32 rmapCode) {
	uint16 index = 0;
	Common::SeekableReadStream *rmapStream = getRawData(ID_RMAP, 1);

	for (uint16 i = 1; rmapStream->pos() < rmapStream->size(); i++) {
		uint32 code = rmapStream->readUint32BE();
		if (code == rmapCode)
			index = i;
	}

	delete rmapStream;

	if (!index)
		error ("Could not match RMAP code %08x", rmapCode);

	return index - 1;
}

uint32 MohawkEngine_Riven::getCurCardRMAP() {
	Common::SeekableReadStream *rmapStream = getRawData(ID_RMAP, 1);
	rmapStream->seek(_curCard * 4);
	uint32 rmapCode = rmapStream->readUint32BE();
	delete rmapStream;
	return rmapCode;
}

void MohawkEngine_Riven::runCardScript(uint16 scriptType) {
	assert(_cardData.hasData);
	for (uint16 i = 0; i < _cardData.scripts.size(); i++)
		if (_cardData.scripts[i]->getScriptType() == scriptType) {
			_cardData.scripts[i]->runScript();
			break;
		}
}

void MohawkEngine_Riven::runHotspotScript(uint16 hotspot, uint16 scriptType) {
	assert(hotspot < _hotspotCount);
	for (uint16 i = 0; i < _hotspots[hotspot].scripts.size(); i++)
		if (_hotspots[hotspot].scripts[i]->getScriptType() == scriptType) {
			_hotspots[hotspot].scripts[i]->runScript();
			break;
		}
}

void MohawkEngine_Riven::delayAndUpdate(uint32 ms) {
	uint32 startTime = _system->getMillis();

	while (_system->getMillis() < startTime + ms && !shouldQuit()) {
		bool needsUpdate = _gfx->runScheduledWaterEffects();
		needsUpdate |= _video->updateBackgroundMovies();

		Common::Event event;
		while (_system->getEventManager()->pollEvent(event))
			;

		if (needsUpdate)
			_system->updateScreen();

		_system->delayMillis(10); // Ease off the CPU
	}
}

void MohawkEngine_Riven::runLoadDialog() {
	GUI::SaveLoadChooser slc(_("Load game:"), _("Load"));
	slc.setSaveMode(false);

	Common::String gameId = ConfMan.get("gameid");

	const EnginePlugin *plugin = 0;
	EngineMan.findGame(gameId, &plugin);

	int slot = slc.runModal(plugin, ConfMan.getActiveDomainName());
	if (slot >= 0)
		loadGameState(slot);

	slc.close();
}

Common::Error MohawkEngine_Riven::loadGameState(int slot) {
	return _saveLoad->loadGame(_saveLoad->generateSaveGameList()[slot]) ? Common::kNoError : Common::kUnknownError;
}

Common::Error MohawkEngine_Riven::saveGameState(int slot, const char *desc) {
	Common::StringArray saveList = _saveLoad->generateSaveGameList();

	if ((uint)slot < saveList.size())
		_saveLoad->deleteSave(saveList[slot]);

	return _saveLoad->saveGame(Common::String(desc)) ? Common::kNoError : Common::kUnknownError;
}

Common::String MohawkEngine_Riven::getStackName(uint16 stack) const {
	static const char *rivenStackNames[] = {
		"aspit",
		"bspit",
		"gspit",
		"jspit",
		"ospit",
		"pspit",
		"rspit",
		"tspit"
	};

	return rivenStackNames[stack];
}

bool ZipMode::operator== (const ZipMode &z) const {
	return z.name == name && z.id == id;
}

} // End of namespace Mohawk
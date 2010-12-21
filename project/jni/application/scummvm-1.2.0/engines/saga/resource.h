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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-2-0/engines/saga/resource.h $
 * $Id: resource.h 49046 2010-05-16 10:23:44Z thebluegr $
 *
 */

// RSC Resource file management header file

#ifndef SAGA_RESOURCE_H
#define SAGA_RESOURCE_H

#include "common/file.h"
#include "common/list.h"

namespace Saga {

#define MAC_BINARY_HEADER_SIZE 128
#define RSC_TABLEINFO_SIZE 8
#define RSC_TABLEENTRY_SIZE 8

#define RSC_MIN_FILESIZE (RSC_TABLEINFO_SIZE + RSC_TABLEENTRY_SIZE + 1)

struct PatchData {
	Common::File *_patchFile;
	const char *_fileName;
	bool _deletePatchFile;

	PatchData(const char *fileName): _fileName(fileName), _deletePatchFile(true) {
		_patchFile = new Common::File();
	}
	PatchData(Common::File *patchFile, const char *fileName): _patchFile(patchFile), _fileName(fileName), _deletePatchFile(false) {
	}

	~PatchData() {
		if (_deletePatchFile) {
			delete _patchFile;
		}
	}
};

struct ResourceData {
	uint32 id;		// SAGA2
	size_t offset;
	size_t size;
	PatchData *patchData;

	ResourceData() :
		id(0), offset(0), size(0), patchData(NULL) {
	}

	~ResourceData() {
		if (patchData) {
			delete patchData;
			patchData = NULL;
		}
	}

	bool isExternal() {	// SAGA2
		return ((offset & (1L<<31)) != 0L);
	}
};

class ResourceDataArray : public Common::Array<ResourceData> {
};

class ResourceContext {
friend class Resource;
protected:
	const char *_fileName;
	uint16 _fileType;
	bool _isCompressed;
	int _serial;					// IHNM speech files

	bool _isBigEndian;
	ResourceDataArray _table;
	Common::File _file;
	int32 _fileSize;

	bool load(SagaEngine *_vm, Resource *resource);
	bool loadResV1(uint32 contextOffset, uint32 contextSize);

	virtual bool loadMac() = 0;
	virtual bool loadRes(uint32 contextOffset, uint32 contextSize) = 0;
public:

	ResourceContext():
		_fileName(NULL), _fileType(0), _isCompressed(false), _serial(0),
		_isBigEndian(false),
		_fileSize(0) {
	}

	virtual ~ResourceContext() {
	}

	bool isCompressed() const {
		return _isCompressed;
	}

	uint16 fileType() const {
		return _fileType;
	}

	int32 fileSize() const {
		return _fileSize;
	}

	int serial() const {
		return _serial;
	}

	bool isBigEndian() const {
		return _isBigEndian;
	}

	const char * fileName() const {
		return _fileName;
	}

	Common::File *getFile(ResourceData *resourceData) {
		Common::File *file;
		const char * fn;
		if (resourceData && resourceData->patchData != NULL) {
			file = resourceData->patchData->_patchFile;
			fn = resourceData->patchData->_fileName;
		} else {
			file = &_file;
			fn = _fileName;
		}
		if (!file->isOpen())
			file->open(fn);
		return file;
	}

	bool validResourceId(uint32 resourceId) const {
		return (resourceId < _table.size());
	}

	ResourceData *getResourceData(uint32 resourceId) {
		if (resourceId >= _table.size()) {
			error("ResourceContext::getResourceData() wrong resourceId %d", resourceId);
		}
		return &_table[resourceId];
	}

	// SAGA 2
	int32 getEntryNum(uint32 id) {
		int32 num = 0;
		for (ResourceDataArray::const_iterator i = _table.begin(); i != _table.end(); ++i) {
			if (i->id == id) {
				return num;
			}
			num++;
		}
		return -1;
	}
};

class ResourceContextList : public Common::List<ResourceContext*> {
};

struct MetaResource {
	int16 sceneIndex;
	int16 objectCount;
	int32 objectsStringsResourceID;
	int32 inventorySpritesID;
	int32 mainSpritesID;
	int32 objectsResourceID;
	int16 actorCount;
	int32 actorsStringsResourceID;
	int32 actorsResourceID;
	int32 protagFaceSpritesID;
	int32 field_22;
	int16 field_26;
	int16 protagStatesCount;
	int32 protagStatesResourceID;
	int32 cutawayListResourceID;
	int32 songTableID;

	MetaResource() {
		memset(this, 0, sizeof(*this));
	}
};

class Resource {
public:
	Resource(SagaEngine *vm);
	virtual ~Resource();
	bool createContexts();
	void clearContexts();
	void loadResource(ResourceContext *context, uint32 resourceId, byte*&resourceBuffer, size_t &resourceSize);

	virtual uint32 convertResourceId(uint32 resourceId) = 0;
	virtual void loadGlobalResources(int chapter, int actorsEntrance) = 0;

	ResourceContext *getContext(uint16 fileType, int serial = 0);
protected:
	SagaEngine *_vm;
	ResourceContextList _contexts;
	char _voicesFileName[8][256];
	char _musicFileName[256];
	char _soundFileName[256];

	void addContext(const char *fileName, uint16 fileType, bool isCompressed = false, int serial = 0);
	virtual ResourceContext *createContext() = 0;
public:
	virtual MetaResource* getMetaResource() = 0;
};

// ITE
class ResourceContext_RSC: public ResourceContext {
protected:
	virtual bool loadMac();
	virtual bool loadRes(uint32 contextOffset, uint32 contextSize) {
		return loadResV1(contextOffset, contextSize);
	}
};

class Resource_RSC : public Resource {
public:
	Resource_RSC(SagaEngine *vm) : Resource(vm) {}
	virtual uint32 convertResourceId(uint32 resourceId);
	virtual void loadGlobalResources(int chapter, int actorsEntrance) {}
	virtual MetaResource* getMetaResource() {
		MetaResource *dummy = 0;
		return dummy;
	}
protected:
	virtual ResourceContext *createContext() {
		return new ResourceContext_RSC();
	}
};

#ifdef ENABLE_IHNM
// IHNM
class ResourceContext_RES: public ResourceContext {
protected:
	virtual bool loadMac() {
		return false;
	}
	virtual bool loadRes(uint32 contextOffset, uint32 contextSize) {
		return loadResV1(0, contextSize);
	}
};

//TODO: move load routines from sndres
class VoiceResourceContext_RES: public ResourceContext {
protected:
	virtual bool loadMac() {
		return false;
	}
	virtual bool loadRes(uint32 contextOffset, uint32 contextSize) {
		return false;
	}
public:
	VoiceResourceContext_RES() : ResourceContext() {
		_fileType = GAME_VOICEFILE;
		_isBigEndian = true;
	}
};

class Resource_RES : public Resource {
public:
	Resource_RES(SagaEngine *vm) : Resource(vm) {}
	virtual uint32 convertResourceId(uint32 resourceId) { return resourceId; }
	virtual void loadGlobalResources(int chapter, int actorsEntrance);
	virtual MetaResource* getMetaResource() { return &_metaResource; }
protected:
	virtual ResourceContext *createContext() {
		return new ResourceContext_RES();
	}
private:
	MetaResource _metaResource;
};
#endif

#ifdef ENABLE_SAGA2
// DINO, FTA2
class ResourceContext_HRS: public ResourceContext {
protected:
	ResourceDataArray _categories;

	virtual bool loadMac() {
		return false;
	}
	virtual bool loadRes(uint32 contextOffset, uint32 contextSize) {
		return loadResV2(contextSize);
	}
	bool loadResV2(uint32 contextSize);
};

class Resource_HRS : public Resource {
public:
	Resource_HRS(SagaEngine *vm) : Resource(vm) {}
	virtual uint32 convertResourceId(uint32 resourceId) { return resourceId; }
	virtual void loadGlobalResources(int chapter, int actorsEntrance) {}
	virtual MetaResource* getMetaResource() {
		MetaResource *dummy = 0;
		return dummy;
	}
protected:
	virtual ResourceContext *createContext() {
		return new ResourceContext_HRS();
	}
};
#endif

} // End of namespace Saga

#endif
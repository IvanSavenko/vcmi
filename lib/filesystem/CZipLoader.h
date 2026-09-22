/*
 * CZipLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ISimpleResourceLoader.h"
#include "CInputStream.h"
#include "ResourcePath.h"
#include "CCompressedStream.h"

#include "MinizipExtensions.h"

/// Archive handle shared between loader and all streams opened from it.
/// Opening archive requires locating its central directory, which is expensive, so archive is opened only once
struct CZipArchiveHandle : boost::noncopyable
{
	std::shared_ptr<CIOApi> ioApi;
	zlib_filefunc64_def zlibApi;
	unzFile file = nullptr;

	/// guards all access to file, including current file position within archive
	std::mutex mutex;
	/// stream that currently has its file opened within archive, if any
	const void * activeStream = nullptr;

	CZipArchiveHandle(const boost::filesystem::path & archive, std::shared_ptr<CIOApi> api);
	~CZipArchiveHandle();
};

class CZipStream : public CBufferedStream
{
	std::shared_ptr<CZipArchiveHandle> archive;
	unz64_file_pos filepos;
	si64 fileSize;
	ui32 fileCRC;
	/// number of bytes already read from file by this stream
	si64 bytesRead = 0;

	/// Makes this stream's file current in shared archive handle. Must be called with archive mutex locked
	void activate();

public:
	/**
	 * @brief constructs zip stream from already opened archive
	 * @param archive shared handle of opened archive
	 * @param filepos position of file to open
	 */
	CZipStream(std::shared_ptr<CZipArchiveHandle> archive, unz64_file_pos filepos);
	~CZipStream();

	si64 getSize() override;
	ui32 calculateCRC32() override;

protected:
	si64 readMore(ui8 * data, si64 size) override;
};

class CZipLoader : public ISimpleResourceLoader
{
	boost::filesystem::path archiveName;
	std::string mountPoint;
	std::shared_ptr<CZipArchiveHandle> archive;

	std::unordered_map<ResourcePath, unz64_file_pos> files;

	std::unordered_map<ResourcePath, unz64_file_pos> listFiles(const std::string & mountPoint);
public:
	CZipLoader(const std::string & mountPoint, const boost::filesystem::path & archive, std::shared_ptr<CIOApi> api = std::make_shared<CDefaultIOApi>());

	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const ResourcePath & resourceName) const override;
	bool existsResource(const ResourcePath & resourceName) const override;
	std::string getMountPoint() const override;
	void updateFilteredFiles(std::function<bool(const std::string &)> filter) override {}
	std::unordered_set<ResourcePath> getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const override;
	std::string getFullFileURI(const ResourcePath& resourceName) const override;
	std::time_t getLastWriteTime(const ResourcePath& resourceName) const override;
};

class DLL_LINKAGE ZipArchive : boost::noncopyable
{
	unzFile archive;

public:
	ZipArchive(const boost::filesystem::path & from);
	~ZipArchive();

	std::vector<std::string> listFiles();
	bool extract(const boost::filesystem::path & where, const std::vector<std::string> & what);
	bool extract(const boost::filesystem::path & where, const std::string & what);
};

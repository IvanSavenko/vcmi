/*
 * CZipLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CZipLoader.h"

#include "../ScopeGuard.h"
#include "../texts/TextOperations.h"

CZipArchiveHandle::CZipArchiveHandle(const boost::filesystem::path & archive, std::shared_ptr<CIOApi> api)
	: ioApi(std::move(api))
	, zlibApi(ioApi->getApiStructure())
{
	file = unzOpen2_64(archive.c_str(), &zlibApi);

	if(file == nullptr)
		logGlobal->error("%s failed to open", TextOperations::filesystemPathToUtf8(archive));
}

CZipArchiveHandle::~CZipArchiveHandle()
{
	if (file)
		unzClose(file);
}

CZipStream::CZipStream(std::shared_ptr<CZipArchiveHandle> archive, unz64_file_pos filepos)
	: archive(std::move(archive))
	, filepos(filepos)
{
	std::scoped_lock lock(this->archive->mutex);

	unz_file_info64 info;
	unzGoToFilePos64(this->archive->file, &this->filepos);
	unzGetCurrentFileInfo64(this->archive->file, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	fileSize = info.uncompressed_size;
	fileCRC = info.crc;
}

CZipStream::~CZipStream()
{
	std::scoped_lock lock(archive->mutex);
	if (archive->activeStream == this)
	{
		unzCloseCurrentFile(archive->file);
		archive->activeStream = nullptr;
	}
}

void CZipStream::activate()
{
	if (archive->activeStream == this)
		return;

	if (archive->activeStream != nullptr)
		unzCloseCurrentFile(archive->file);

	unzGoToFilePos64(archive->file, &filepos);
	unzOpenCurrentFile(archive->file);
	archive->activeStream = this;

	// another stream was using archive since our last read - restore our position within file
	std::array<ui8, 8 * 1024> skipBuffer{};
	si64 toSkip = bytesRead;
	while (toSkip > 0)
	{
		int skipped = unzReadCurrentFile(archive->file, skipBuffer.data(), static_cast<unsigned int>(std::min<si64>(toSkip, skipBuffer.size())));
		if (skipped <= 0)
			break;
		toSkip -= skipped;
	}
}

si64 CZipStream::readMore(ui8 * data, si64 size)
{
	std::scoped_lock lock(archive->mutex);
	activate();

	int result = unzReadCurrentFile(archive->file, data, static_cast<unsigned int>(size));
	if (result > 0)
		bytesRead += result;
	return result;
}

si64 CZipStream::getSize()
{
	return fileSize;
}

ui32 CZipStream::calculateCRC32()
{
	return fileCRC;
}

///CZipLoader
CZipLoader::CZipLoader(const std::string & mountPoint, const boost::filesystem::path & archivePath, std::shared_ptr<CIOApi> api):
	archiveName(archivePath),
	mountPoint(mountPoint),
	archive(std::make_shared<CZipArchiveHandle>(archivePath, std::move(api))),
	files(listFiles(mountPoint))
{
	logGlobal->trace("Zip archive loaded, %d files found", files.size());
}

std::unordered_map<ResourcePath, unz64_file_pos> CZipLoader::listFiles(const std::string & mountPoint)
{
	std::unordered_map<ResourcePath, unz64_file_pos> ret;

	std::scoped_lock lock(archive->mutex);
	unzFile file = archive->file;

	if (file != nullptr && unzGoToFirstFile(file) == UNZ_OK)
	{
		do
		{
			unz_file_info64 info;
			std::vector<char> filename;
			// Fill unz_file_info structure with current file info
			unzGetCurrentFileInfo64 (file, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			filename.resize(info.size_filename);
			// Get name of current file. Contrary to docs "info" parameter can't be null
			unzGetCurrentFileInfo64(file, &info, filename.data(), static_cast<uLong>(filename.size()), nullptr, 0, nullptr, 0);

			std::string filenameString(filename.data(), filename.size());
			unzGetFilePos64(file, &ret[ResourcePath(mountPoint + filenameString)]);
		}
		while (unzGoToNextFile(file) == UNZ_OK);
	}

	return ret;
}

std::unique_ptr<CInputStream> CZipLoader::load(const ResourcePath & resourceName) const
{
	return std::make_unique<CZipStream>(archive, files.at(resourceName));
}

bool CZipLoader::existsResource(const ResourcePath & resourceName) const
{
	return files.count(resourceName) != 0;
}

std::string CZipLoader::getMountPoint() const
{
	return mountPoint;
}

std::unordered_set<ResourcePath> CZipLoader::getFilteredFiles(std::function<bool(const ResourcePath &)> filter) const
{
	std::unordered_set<ResourcePath> foundID;

	for(const auto & file : files)
	{
		if (filter(file.first))
			foundID.insert(file.first);
	}
	return foundID;
}

std::string CZipLoader::getFullFileURI(const ResourcePath& resourceName) const
{
	auto relativePath = TextOperations::Utf8TofilesystemPath(resourceName.getName());
	auto path = boost::filesystem::canonical(archiveName) / relativePath;
	return TextOperations::filesystemPathToUtf8(path);
}

std::time_t CZipLoader::getLastWriteTime(const ResourcePath& resourceName) const
{
	auto path = boost::filesystem::canonical(archiveName);
	return  boost::filesystem::last_write_time(path);
}

/// extracts currently selected file from zip into stream "where"
static bool extractCurrent(unzFile file, std::ostream & where)
{
	std::array<char, 8 * 1024> buffer{};

	unzOpenCurrentFile(file);

	while(true)
	{
		int readSize = unzReadCurrentFile(file, buffer.data(), static_cast<unsigned int>(buffer.size()));

		if (readSize < 0) // error
			break;

		if (readSize == 0) // end-of-file. Also performs CRC check
			return unzCloseCurrentFile(file) == UNZ_OK;

		if (readSize > 0) // successful read
		{
			where.write(buffer.data(), readSize);
			if (!where.good())
				break;
		}
	}

	// extraction failed. Close file and exit
	unzCloseCurrentFile(file);
	return false;
}

boost::filesystem::path zipFilenameToFilesystemPath(const std::string & filename, bool isUtf8)
{
#ifdef VCMI_WINDOWS
	if (isUtf8)
		return TextOperations::Utf8TofilesystemPath(filename);

	return boost::filesystem::path(filename);
#else
	return boost::filesystem::path(filename);
#endif
}

std::vector<std::string> ZipArchive::listFiles()
{
	std::vector<std::string> ret;

	int result = unzGoToFirstFile(archive);

	if (result == UNZ_OK)
	{
		do
		{
			unz_file_info64 info;
			std::vector<char> zipFilename;

			unzGetCurrentFileInfo64 (archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			zipFilename.resize(info.size_filename);
			// Get name of current file. Contrary to docs "info" parameter can't be null
			unzGetCurrentFileInfo64(archive, &info, zipFilename.data(), static_cast<uLong>(zipFilename.size()), nullptr, 0, nullptr, 0);

			ret.emplace_back(zipFilename.data(), zipFilename.size());

			result = unzGoToNextFile(archive);
		}
		while (result == UNZ_OK);
	}
	return ret;
}

ZipArchive::ZipArchive(const boost::filesystem::path & from)
{
	CDefaultIOApi zipAPI;

#if MINIZIP_NEEDS_32BIT_FUNCS
	auto zipStructure = zipAPI.getApiStructure32();
	archive = unzOpen2(from.c_str(), &zipStructure);
#else
	auto zipStructure = zipAPI.getApiStructure();
	archive = unzOpen2_64(from.c_str(), &zipStructure);
#endif

	if (archive == nullptr)
		throw std::runtime_error("Failed to open file '" + TextOperations::filesystemPathToUtf8(from));
}

ZipArchive::~ZipArchive()
{
	unzClose(archive);
}

bool ZipArchive::extract(const boost::filesystem::path & where, const std::vector<std::string> & what)
{
	for (const std::string & file : what)
		if (!extract(where, file))
			return false;

	return true;
}

bool ZipArchive::extract(const boost::filesystem::path & where, const std::string & file)
{
	if (unzLocateFile(archive, file.c_str(), 1) != UNZ_OK)
		return false;

	unz_file_info64 info;
	unzGetCurrentFileInfo64(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

	constexpr uLong ZIP_UTF8_FILENAME_FLAG = 1 << 11;
	const bool isUtf8Filename = (info.flag & ZIP_UTF8_FILENAME_FLAG) != 0;

	const boost::filesystem::path relativeName = zipFilenameToFilesystemPath(file, isUtf8Filename);
	const boost::filesystem::path fullName = where / relativeName;
	const boost::filesystem::path fullPath = fullName.parent_path();

	boost::filesystem::create_directories(fullPath);
	// directory. No file to extract
	// TODO: better way to detect directory? Probably check return value of unzOpenCurrentFile?
	if (boost::algorithm::ends_with(file, "/"))
		return true;

	std::fstream destFile(fullName.c_str(), std::ios::out | std::ios::binary);
	if (!destFile.good())
	{
#ifdef VCMI_WINDOWS
		if (fullName.size() < 260)
			logGlobal->error("Failed to open file '%s'", TextOperations::filesystemPathToUtf8(fullName));
		else
			logGlobal->error("Failed to open file with long path '%s' (%d characters)", TextOperations::filesystemPathToUtf8(fullName), fullName.size());
#else
		logGlobal->error("Failed to open file '%s'", fullName.c_str());
#endif

		return false;
	}

	if (!extractCurrent(archive, destFile))
		return false;
	return true;
}

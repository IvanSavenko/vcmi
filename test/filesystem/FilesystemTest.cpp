/*
 * FilesystemTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/filesystem/AdapterLoaders.h"
#include "../../lib/filesystem/CFilesystemLoader.h"
#include "../../lib/filesystem/ResourcePath.h"

namespace test
{

class FilesystemTest : public ::testing::Test
{
	boost::filesystem::path directory;

protected:
	const boost::filesystem::path & getDirectory() const
	{
		return directory;
	}

	void SetUp() override
	{
		directory = boost::filesystem::current_path()
			/ boost::filesystem::unique_path("vcmi-filesystem-%%%%-%%%%-%%%%-%%%%");
		ASSERT_TRUE(boost::filesystem::create_directory(directory));
	}

	void TearDown() override
	{
		boost::filesystem::remove_all(directory);
	}
};

TEST_F(FilesystemTest, RemovesResourceAndUpdatesIndex)
{
	CFilesystemLoader loader("Test/", getDirectory());
	const ResourcePath resource("Test/example.txt");

	ASSERT_TRUE(loader.createResource("Test/example.txt"));
	ASSERT_TRUE(loader.existsResource(resource));
	ASSERT_TRUE(boost::filesystem::exists(getDirectory() / "example.txt"));

	EXPECT_TRUE(loader.removeResource(resource));
	EXPECT_FALSE(loader.existsResource(resource));
	EXPECT_FALSE(boost::filesystem::exists(getDirectory() / "example.txt"));
}

TEST_F(FilesystemTest, DoesNotCreateResourceOutsideOfDirectory)
{
	const auto loaderDirectory = getDirectory() / "loader";
	ASSERT_TRUE(boost::filesystem::create_directory(loaderDirectory));
	CFilesystemLoader loader("Test/", loaderDirectory);

	EXPECT_FALSE(loader.createResource("Test/../example.txt"));
	EXPECT_FALSE(loader.createResource("Test/folder/../../example.txt"));
	EXPECT_FALSE(loader.createResource("Test/" + (getDirectory() / "example.txt").string()));
	EXPECT_FALSE(boost::filesystem::exists(getDirectory() / "example.txt"));

	EXPECT_TRUE(loader.createResource("Test/folder/example.txt"));
	EXPECT_TRUE(boost::filesystem::exists(loaderDirectory / "folder" / "example.txt"));
}

TEST_F(FilesystemTest, FindsFileWithRepeatedExtensionByItsName)
{
	CFilesystemLoader loader("Test/", getDirectory());
	ASSERT_TRUE(loader.createResource("Test/example.h3m.h3m"));

	const ResourcePath storedName(ResourcePath("Test/example.h3m.h3m").getOriginalName(), EResType::MAP);
	EXPECT_TRUE(loader.existsResource(storedName));
}

TEST_F(FilesystemTest, ListsSharingLoaderFindFileFoundByRescanOfAnotherList)
{
	auto shared = std::make_shared<CFilesystemLoader>("Test/", getDirectory());
	const auto createList = [&shared]()
	{
		// enough children for the list to keep an index
		auto list = std::make_unique<CFilesystemList>();
		for(int i = 0; i < 10; ++i)
			list->addLoader(std::make_shared<CFilesystemList>(), false);
		list->addLoader(shared, false);
		return list;
	};
	auto first = createList();
	auto second = createList();

	std::ofstream(getDirectory() / "example.txt") << "example";
	const auto allMounts = [](const std::string &){ return true; };
	const ResourcePath resource("Test/example.txt");

	EXPECT_TRUE(first->updateFilteredFiles(allMounts));
	EXPECT_TRUE(first->existsResource(resource));

	EXPECT_TRUE(second->updateFilteredFiles(allMounts));
	EXPECT_TRUE(second->existsResource(resource));
}

TEST(ResourcePathTest, KeepsExtensionOfDifferentType)
{
	EXPECT_EQ(ResourcePath("Fonts/CALLI10R.FNT.txt").getName(), "FONTS/CALLI10R.FNT");
}

}

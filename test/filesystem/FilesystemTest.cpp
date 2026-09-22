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

TEST_F(FilesystemTest, LookupIndexFollowsChangesInNestedLoaders)
{
	ASSERT_TRUE(boost::filesystem::create_directory(getDirectory() / "first"));
	ASSERT_TRUE(boost::filesystem::create_directory(getDirectory() / "second"));

	auto firstLoader = std::make_unique<CFilesystemLoader>("Test/", getDirectory() / "first");
	auto secondLoader = std::make_unique<CFilesystemLoader>("Test/", getDirectory() / "second");
	auto * first = firstLoader.get();
	auto * second = secondLoader.get();

	auto nested = std::make_unique<CFilesystemList>();
	nested->addLoader(std::move(firstLoader), true);

	CFilesystemList indexed;
	indexed.enableLookupIndex();
	indexed.addLoader(std::move(nested), false);
	indexed.addLoader(std::move(secondLoader), false);

	const ResourcePath resource("Test/example.txt");
	EXPECT_FALSE(indexed.existsResource(resource));

	ASSERT_TRUE(first->createResource("Test/example.txt"));
	EXPECT_EQ(indexed.getResourceName(resource), getDirectory() / "first" / "example.txt");

	// later loader overrides earlier one
	ASSERT_TRUE(second->createResource("Test/example.txt"));
	EXPECT_EQ(indexed.getResourceName(resource), getDirectory() / "second" / "example.txt");

	ASSERT_TRUE(second->removeResource(resource));
	EXPECT_EQ(indexed.getResourceName(resource), getDirectory() / "first" / "example.txt");

	ASSERT_TRUE(first->removeResource(resource));
	EXPECT_FALSE(indexed.existsResource(resource));
}

}

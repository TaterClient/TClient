#include <base/str.h>

#include <game/localization.h>

#include <gtest/gtest.h>

#include <array>

static const std::array<const char *, 15> gs_apStrings = {
	"zero", "one", "two", "three", "four", "five", "six", "seven",
	"eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen"};

static const char *Find(const CLocalizationDatabase &Db, const char *pStr, const char *pContext = "")
{
	return Db.FindString(str_quickhash(pStr), str_quickhash(pContext));
}

// AddString must keep m_vStrings sorted so that the binary search in FindString
// works, even though AddString is called before Load sorts the vector. Adding
// entries whose hashes are not in ascending order used to call std::equal_range
// on an unsorted range, which is undefined behaviour and aborts when the
// libstdc++ debug mode (enabled in Debug builds) is used.
TEST(Localization, AddStringKeepsSorted)
{
	CLocalizationDatabase Db;
	for(const char *pStr : gs_apStrings)
		Db.AddString(pStr, pStr, "");
	for(const char *pStr : gs_apStrings)
		EXPECT_STREQ(Find(Db, pStr), pStr);
}

// Adding the same original string twice must not create a duplicate, otherwise
// FindString's check for a unique match fails and the translation is lost.
TEST(Localization, AddStringNoDuplicates)
{
	CLocalizationDatabase Db;
	for(const char *pStr : gs_apStrings)
	{
		Db.AddString(pStr, pStr, "");
		Db.AddString(pStr, "ignored", "");
	}
	for(const char *pStr : gs_apStrings)
		EXPECT_STREQ(Find(Db, pStr), pStr);
}

// Contexts are part of the lookup key and a missing context falls back to the
// empty ("default") context.
TEST(Localization, Contexts)
{
	CLocalizationDatabase Db;
	for(const char *pStr : gs_apStrings)
	{
		Db.AddString(pStr, "default", "");
		Db.AddString(pStr, "context", "mycontext");
	}
	for(const char *pStr : gs_apStrings)
	{
		EXPECT_STREQ(Find(Db, pStr, "mycontext"), "context");
		EXPECT_STREQ(Find(Db, pStr, ""), "default");
	}
	EXPECT_STREQ(Find(Db, "zero", "othercontext"), "default");
	EXPECT_EQ(Find(Db, "missing"), nullptr);
}

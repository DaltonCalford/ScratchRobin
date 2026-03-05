// Copyright (c) 2025 Silverstone Data Systems
// robin-migrate: Query History Unit Tests

#include "test_framework.h"
#include "../../src/core/query_history.h"

using namespace scratchrobin::core;
using namespace scratchrobin::testing;

// Test adding entries
static TestFailure Test_AddEntry() {
  QueryHistory& history = QueryHistory::GetInstance();
  history.SetStoragePath("");  // Don't persist for tests
  history.ClearHistory();  // Clear for clean test
  
  QueryHistoryEntry entry;
  entry.sql = "SELECT * FROM test";
  entry.database = "test_db";
  entry.rows_affected = 10;
  entry.successful = true;
  
  int id = history.AddEntry(entry);
  ASSERT_EQ(1, id);
  ASSERT_EQ(1, history.GetTotalCount());
  
  return TestFailure{"", "", 0, true};
}

// Test favorites
static TestFailure Test_Favorites() {
  QueryHistory& history = QueryHistory::GetInstance();
  history.SetStoragePath("");
  history.ClearHistory();
  
  QueryHistoryEntry entry1;
  entry1.sql = "SELECT 1";
  int id1 = history.AddEntry(entry1);
  
  QueryHistoryEntry entry2;
  entry2.sql = "SELECT 2";
  entry2.is_favorite = true;
  int id2 = history.AddEntry(entry2);
  
  ASSERT_EQ(1, history.GetFavoriteCount());
  
  auto favorites = history.GetFavoriteEntries();
  ASSERT_EQ(1, (int)favorites.size());
  ASSERT_EQ(id2, favorites[0].id);
  
  return TestFailure{"", "", 0, true};
}

// Test search
static TestFailure Test_Search() {
  QueryHistory& history = QueryHistory::GetInstance();
  history.SetStoragePath("");
  history.ClearHistory();
  
  QueryHistoryEntry entry1;
  entry1.sql = "SELECT * FROM users";
  history.AddEntry(entry1);
  
  QueryHistoryEntry entry2;
  entry2.sql = "INSERT INTO orders";
  history.AddEntry(entry2);
  
  auto results = history.SearchEntries("SELECT");
  ASSERT_EQ(1, (int)results.size());
  
  return TestFailure{"", "", 0, true};
}

// Test max entries limit
static TestFailure Test_MaxEntries() {
  QueryHistory& history = QueryHistory::GetInstance();
  history.SetStoragePath("");
  history.ClearHistory();
  history.SetMaxEntries(5);
  
  for (int i = 0; i < 10; i++) {
    QueryHistoryEntry entry;
    entry.sql = "Query " + std::to_string(i);
    history.AddEntry(entry);
  }
  
  ASSERT_EQ(5, history.GetTotalCount());
  
  return TestFailure{"", "", 0, true};
}

// Test tags
static TestFailure Test_Tags() {
  QueryHistory& history = QueryHistory::GetInstance();
  history.SetStoragePath("");
  history.ClearHistory();
  
  QueryHistoryEntry entry;
  entry.sql = "SELECT * FROM test";
  int id = history.AddEntry(entry);
  
  history.AddTag(id, "important");
  history.AddTag(id, "production");
  
  auto tagged = history.GetEntriesByTag("important");
  ASSERT_EQ(1, (int)tagged.size());
  
  auto all_tags = history.GetAllTags();
  ASSERT_EQ(2, (int)all_tags.size());
  
  return TestFailure{"", "", 0, true};
}

// Register all tests
static struct QueryHistoryTests {
  QueryHistoryTests() {
    UnitTestFramework::RegisterTest("QueryHistory", "AddEntry", Test_AddEntry);
    UnitTestFramework::RegisterTest("QueryHistory", "Favorites", Test_Favorites);
    UnitTestFramework::RegisterTest("QueryHistory", "Search", Test_Search);
    UnitTestFramework::RegisterTest("QueryHistory", "MaxEntries", Test_MaxEntries);
    UnitTestFramework::RegisterTest("QueryHistory", "Tags", Test_Tags);
  }
} _query_history_tests;


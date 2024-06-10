
#include <csv/datasource/icu/DataSource.hpp>
#include <csv/datasource/utf8/DataSource.hpp>
#include <csv/parser.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

std::vector<csv::record> AddRecords(csv::IDataSource &data) {
  std::vector<csv::record> records;
  auto recordAdder = [&records](const csv::record &record,
                                [[maybe_unused]] double _complete) -> bool {
    records.push_back(record);
    return true;
  };
  csv::parse(data, NULL, recordAdder);
  return records;
}

std::vector<csv::record> AddRecordsMax(csv::IDataSource &data,
                                       size_t maxCount) {
  std::vector<csv::record> records;
  auto recordAdder = [&](const csv::record &record,
                         [[maybe_unused]] double _complete) -> bool {
    assert(record.row == records.size());
    records.push_back(record);
    return (record.row != maxCount - 1);
  };
  csv::parse(data, NULL, recordAdder);
  return records;
}

std::vector<csv::record> AddRecords(const std::string &str) {
  std::vector<csv::record> records;

  csv::utf8::StringDataSource input;
  input.set(str);

  auto recordAdder = [&](const csv::record &record,
                         [[maybe_unused]] double complete) -> bool {
    records.push_back(record);
    return true;
  };

  csv::parse(input, NULL, recordAdder);
  return records;
}

std::string getResource(const char *name) {

  const auto path = std::filesystem::path{"./resources"} / name;

  if (!std::filesystem::exists(path)) {
    return "";
  }

  return path.string();
}

TEST(CSVTests, Simple) {
  std::vector<csv::record> records;

  csv::utf8::StringDataSource input;
  ASSERT_TRUE(input.set("cat, dog, fish"));
  records = AddRecords(input);
  ASSERT_EQ(1, records.size());
  ASSERT_EQ(3, records[0].size());
  ASSERT_EQ("cat", records[0][0].content);
  ASSERT_EQ("dog", records[0][1].content);
  ASSERT_EQ("fish", records[0][2].content);

  ASSERT_TRUE(input.set("cat, \"dog\", fi\"sh"));
  records = AddRecords(input);
  ASSERT_EQ(1, records.size());
  ASSERT_EQ(3, records[0].size());
  ASSERT_EQ("cat", records[0][0].content);
  ASSERT_EQ("dog", records[0][1].content);
  ASSERT_EQ("fi\"sh", records[0][2].content);

  ASSERT_TRUE(input.set("cat, \"do\"\"g\", fish   "));
  records = AddRecords(input);
  ASSERT_EQ(1, records.size());
  ASSERT_EQ(3, records[0].size());
  ASSERT_EQ("cat", records[0][0].content);
  ASSERT_EQ("do\"g", records[0][1].content);
  ASSERT_EQ("fish   ", records[0][2].content);

  input.trimLeadingWhitespace = false;
  ASSERT_TRUE(input.set("   cat, \"do\"\"g\", fish   "));
  records = AddRecords(input);
  ASSERT_EQ(1, records.size());
  ASSERT_EQ(3, records[0].size());
  ASSERT_EQ("   cat", records[0][0].content);
  ASSERT_EQ(" \"do\"g\"", records[0][1].content);
  ASSERT_EQ(" fish   ", records[0][2].content);
}

void checkRowIndexes(const std::vector<csv::record> &records) {
  // Check that the row values match.  This
  size_t count = 0;
  for (const auto &record : records) {
    ASSERT_EQ(count++, record.row);
  }
}

TEST(CSVTests, Example) {
  csv::utf8::FileDataSource input;

  const auto url = getResource("orig.csv");
  ASSERT_FALSE(url.empty());

  ASSERT_TRUE(input.open(url.c_str()));

  std::vector<csv::record> records = AddRecords(input);
  ASSERT_EQ(8, records.size());

  ASSERT_EQ("John \n\"Da Man\" 大夨天 🤪 太夫", records[0][0].content);

  // Check that the row values match.  This
  checkRowIndexes(records);
}

TEST(CSVTests, Exceptions) {
  // Invalid filename
  ASSERT_THROW(csv::utf8::FileDataSource("caterpillar"), std::runtime_error);

#ifdef ALLOW_ICU_EXTENSIONS

  // Invalid filename
  ASSERT_THROW(csv::icu::FileDataSource("caterpillar", NULL),
               std::runtime_error);
#endif

  // Now check with a proper file
  const auto url = getResource("orig.csv");
  ASSERT_FALSE(url.empty());

  ASSERT_NO_THROW(csv::utf8::FileDataSource{url.c_str()});

#ifdef ALLOW_ICU_EXTENSIONS

  // Check throw if we pass a dodgy codepage
  ASSERT_THROW(csv::icu::FileDataSource(url, "cccc"), std::runtime_error);

#endif
}

TEST(CSVTests, RFC) {
  std::string i26 = "\"aaa\",\"b \r\nbb\",\"ccc\" \r\n	zzz,yyy,xxx";
  std::vector<csv::record> records = AddRecords(i26);

  ASSERT_EQ(2, records.size());
  ASSERT_EQ(3, records[0].size());
  ASSERT_EQ(3, records[1].size());

  ASSERT_EQ("aaa", records[0][0].content);
  ASSERT_EQ("b \r\nbb", records[0][1].content);
  ASSERT_EQ("ccc", records[0][2].content);

  ASSERT_EQ("\tzzz", records[1][0].content);
  ASSERT_EQ("yyy", records[1][1].content);
  ASSERT_EQ("xxx", records[1][2].content);

  std::string i27 = "aaa,b\"\"bb,ccc";

  records = AddRecords(i27);
  ASSERT_EQ(3, records[0].size());

  ASSERT_EQ("aaa", records[0][0].content);
  ASSERT_EQ("b\"bb", records[0][1].content);
  ASSERT_EQ("ccc", records[0][2].content);

  std::string i25 = "\"aaa\",\"bbb\",\"ccc\"\r\nzzz,yyy,xxx";
  records = AddRecords(i25);
  ASSERT_EQ(2, records.size());

  ASSERT_EQ(3, records[0].size());
  ASSERT_EQ("aaa", records[0][0].content);
  ASSERT_EQ("bbb", records[0][1].content);
  ASSERT_EQ("ccc", records[0][2].content);

  ASSERT_EQ(3, records[1].size());
  ASSERT_EQ("zzz", records[1][0].content);
  ASSERT_EQ("yyy", records[1][1].content);
  ASSERT_EQ("xxx", records[1][2].content);
}

TEST(CSVTests, WhitespaceBeforeEOL) {
  std::vector<csv::record> records;

  csv::utf8::StringDataSource input;
  ASSERT_TRUE(input.set("cat, dog, fish, \nwhale, pig, snork"));
  records = AddRecords(input);

  ASSERT_EQ(2, records.size());

  ASSERT_EQ(4, records[0].size());
  ASSERT_EQ(3, records[1].size());

  // Check that the row values match.  This
  checkRowIndexes(records);
}

TEST(CSVTests, Quoting) {
  csv::utf8::StringDataSource input;
  ASSERT_TRUE(
      input.set("\"cat\",     \"dog\"   , \"fish\", \nwhale, pig, snork"));
  std::vector<csv::record> records = AddRecords(input);

  ASSERT_EQ(2, records.size());

  ASSERT_EQ(4, records[0].size());
  ASSERT_EQ(3, records[1].size());

  // Check that the row values match.  This
  checkRowIndexes(records);
}

TEST(CSVTests, Comment) {
  csv::utf8::StringDataSource input;
  input.comment = '#';

  // Comments should only start at the beginning of a record.  Any field
  // containing or starting with the comment character should not be treated as
  // a comment
  ASSERT_TRUE(
      input.set("# Here is my amazing, and \"groovy\", commented file!\r"
                "cat,#dog,pig\r\n"
                "#Here is the next comment...\n"
                "\"cat\n#fish\", truck, snozzle"));
  std::vector<csv::record> records = AddRecords(input);

  ASSERT_EQ(2, records.size());
  ASSERT_EQ(3, records[0].size());
  ASSERT_EQ(3, records[1].size());

  ASSERT_EQ("cat", records[0][0].content);
  ASSERT_EQ("#dog", records[0][1].content);
  ASSERT_EQ("pig", records[0][2].content);

  ASSERT_EQ("cat\n#fish", records[1][0].content);
  ASSERT_EQ("truck", records[1][1].content);
  ASSERT_EQ("snozzle", records[1][2].content);

  // Check that the row values match.  This
  checkRowIndexes(records);
}

TEST(CSVTests, TinyFileSmallerThanUTF8BOM) {
  // Check filesize less than BOM
  std::vector<csv::record> records;

  csv::utf8::StringDataSource input;
  ASSERT_TRUE(input.set("a"));
  records = AddRecords(input);

  ASSERT_EQ(1, records.size());
  ASSERT_EQ(1, records[0].size());
  ASSERT_EQ("a", records[0][0].content);
}

TEST(CSVTests, UTF8BOMOnly) {
  csv::utf8::StringDataSource input;
  ASSERT_TRUE(input.set("\xEF\xBB\xBF"));
  std::vector<csv::record> records = AddRecords(input);
  ASSERT_EQ(0, records.size());

  // Check for a source that has the same number of characters as the BOM works
  // as expected
  csv::utf8::StringDataSource input2;
  ASSERT_TRUE(input2.set("8,9"));
  std::vector<csv::record> records2 = AddRecords(input2);
  ASSERT_EQ(1, records2.size());
  ASSERT_EQ(1, records2[0].size() == 2);
  ASSERT_EQ("8", records2[0][0].content);
  ASSERT_EQ("9", records2[0][1].content);

  // Input with two fields, one empty
  csv::utf8::StringDataSource input3;
  ASSERT_TRUE(input3.set(",5"));
  std::vector<csv::record> records3 = AddRecords(input3);
  ASSERT_EQ(1, records3.size());
  ASSERT_EQ(1, records3[0].size() == 2);
  ASSERT_EQ("", records3[0][0].content);
  ASSERT_EQ("5", records3[0][1].content);

  /// Input with a single character
  csv::utf8::StringDataSource input4;
  ASSERT_TRUE(input4.set("4"));
  std::vector<csv::record> records4 = AddRecords(input4);
  ASSERT_EQ(1, records4.size());
  ASSERT_EQ(1, records4[0].size() == 1);
  ASSERT_EQ("4", records4[0][0].content);
}

TEST(CSVTests, UTF8BOMRemoval) {
  csv::utf8::StringDataSource input;
  ASSERT_TRUE(input.set("\xEF\xBB\xBF"
                        "cat, dog, fish"));
  std::vector<csv::record> records = AddRecords(input);

  ASSERT_EQ(1, records.size());
  ASSERT_EQ(3, records[0].size());

  ASSERT_EQ("cat", records[0][0].content);
  ASSERT_EQ("dog", records[0][1].content);
  ASSERT_EQ("fish", records[0][2].content);

  // Now the file version
  csv::utf8::FileDataSource parser;
  parser.separator = '\t';

  const auto url = getResource("simple_csv_utf8_bom.tsv");
  ASSERT_FALSE(url.empty());

  ASSERT_TRUE(parser.open(url.c_str()));

  records = AddRecords(parser);
  ASSERT_EQ(1, records.size());
  ASSERT_EQ("fish", records[0][0].content);
  ASSERT_EQ("pig", records[0][1].content);
  ASSERT_EQ("snort", records[0][2].content);

  // Check that the row values match.  This
  checkRowIndexes(records);
}

TEST(CSVTests, VerySmallSampleDataWithoutBOM) {
  csv::utf8::FileDataSource parser;

  const auto url1 = getResource("small_sample.tsv");
  ASSERT_FALSE(url1.empty());

  ASSERT_TRUE(parser.open(url1.c_str()));
  std::vector<csv::record> records = AddRecords(parser);
  ASSERT_EQ(1, records.size());
  ASSERT_EQ("1", records[0][0].content);
  ASSERT_EQ("", records[0][1].content);

  const auto url2 = getResource("small_sample_equal_not_equal_bom.tsv");
  ASSERT_FALSE(url2.empty());

  ASSERT_TRUE(parser.open(url2.c_str()));
  records = AddRecords(parser);
  ASSERT_EQ(1, records.size());
  ASSERT_EQ("r", records[0][0].content);
  ASSERT_EQ("t", records[0][1].content);
}

TEST(CSVTests, ColumnOffsetCounter) {
  csv::utf8::StringDataSource input;
  input.separator = '\t';
  input.trimLeadingWhitespace = false;
  ASSERT_TRUE(input.set("cat\tdog\tfish\t\nwhale\tpig\t snork"));

  std::vector<csv::record> records;

  std::vector<size_t> expected = {0, 1, 2, 3, 0, 1, 2};
  std::vector<size_t> fields;

  auto fieldChecker = [&fields](const csv::field &field) -> bool {
    fields.push_back(field.column);
    return true;
  };

  auto recordAdder = [&records](const csv::record &record,
                                [[maybe_unused]] double complete) -> bool {
    records.push_back(record);
    return true;
  };
  csv::parse(input, fieldChecker, recordAdder);

  // Check field ordering
  ASSERT_EQ(expected, fields);

  // Check that the row values match.  This
  checkRowIndexes(records);

  ASSERT_EQ(2, records.size());
  ASSERT_EQ(4, records[0].size());
  ASSERT_EQ(3, records[1].size());
}

TEST(CSVTests, Separator) {
  csv::utf8::StringDataSource input;
  input.separator = '\t';
  input.trimLeadingWhitespace = false;
  ASSERT_TRUE(input.set("cat\tdog\tfish\t\nwhale\tpig\t snork"));
  std::vector<csv::record> records = AddRecords(input);

  ASSERT_EQ(2, records.size());
  ASSERT_EQ(4, records[0].size());
  ASSERT_EQ(3, records[1].size());

  // Check that the row values match.  This
  checkRowIndexes(records);
}

TEST(CSVTests, LastCharacterIsASeparator) {
  csv::utf8::StringDataSource input;
  input.separator = '\t';
  input.trimLeadingWhitespace = false;

  // Separator at end of line means an empty field finishes the row
  // Separator at end of file means that an empty field finishes the file
  ASSERT_TRUE(input.set("cat\tdog\tfish\t\nwhale\tpig\t snork\t"));
  std::vector<csv::record> records = AddRecords(input);

  ASSERT_EQ(2, records.size());
  ASSERT_EQ(4, records[0].size());
  ASSERT_EQ(4, records[1].size());

  // Check that the row values match.
  checkRowIndexes(records);
}

#ifdef ALLOW_ICU_EXTENSIONS
TEST(CSVTests, ExampleMax) {
  csv::icu::FileDataSource input;

  const auto url = getResource("korean.csv");
  ASSERT_FALSE(url.empty());

  ASSERT_TRUE(input.open(url, NULL));

  std::vector<csv::record> records = AddRecordsMax(input, 4);
  ASSERT_EQ(4, records.size());

  // Check that the row values match.  This
  checkRowIndexes(records);
}

TEST(CSVTests, TotalAColumn) {
  csv::icu::FileDataSource input;

  const auto url = getResource("ford_escort.csv");
  ASSERT_FALSE(url.empty());
  ASSERT_TRUE(input.open(url, NULL));

  size_t count = 0;
  auto fieldAdder = [&count](const csv::field &field) -> bool {
    // Add all entries in the mileage column
    if (field.row != 0 && field.column == 2) {
      count += std::stoi(field.content);
    }
    return true;
  };
  csv::parse(input, fieldAdder, NULL);

  // Check the total mileage for all entries
  ASSERT_EQ(count, 214642);
}
#endif

#ifdef HAVE_OBJ_SUPPORT
TEST(CSVTests, Objc) {
  DSFCSVDataSource *source = [DSFCSVDataSource
      dataSourceWithUTF8String:@"cat, dog, fish, whale\ngoober, nostradamus"
                     separator:','];

  [DSFCSVParser parseWithDataSource:source
      fieldCallback:^BOOL(const NSUInteger row, const NSUInteger column,
                          const NSString *field) {
        NSLog(@"Field (%ld, %ld): %@", row, column, field);
        return YES;
      }
      recordCallback:^BOOL(const NSUInteger row,
                           const NSArray<NSString *> *record,
                           CGFloat complete) {
        NSLog(@"record:\n%@", record);
        return YES;
      }];
}
#endif

TEST(CSVTests, FileWithBlankLinesAndLineEndingsInQuotedStrings) {
  csv::utf8::FileDataSource input;

  const auto url = getResource("classification.csv");
  ASSERT_FALSE(url.empty());

  ASSERT_TRUE(input.open(url.c_str()));

  std::vector<csv::record> records = AddRecords(input);
  ASSERT_EQ(10, records.size());

  // Check that the row values match.  This
  checkRowIndexes(records);

  // Parse the same file, but keep empty lines.
  csv::utf8::FileDataSource input2;
  input2.skipBlankLines = false;

  ASSERT_TRUE(input2.open(url.c_str()));

  records = AddRecords(input2);
  ASSERT_EQ(29, records.size());

  // Check that the row values match.  This
  checkRowIndexes(records);
}

#if defined(HAVE_OBJ_SUPPORT) && defined(ALLOW_ICU_EXTENSIONS)
TEST(CSVTests, ObjcICU) {
  NSURL *url = [self resourceWithName:@"korean" extension:@"csv"];
  XCTAssertNotNil(url);

  DSFCSVDataSource *source = [DSFCSVDataSource dataSourceWithFileURL:url
                                                         icuCodepage:NULL
                                                           separator:','];
  XCTAssertNotNil(source);

  [DSFCSVParser parseWithDataSource:source
      fieldCallback:^BOOL(const NSUInteger row, const NSUInteger column,
                          const NSString *field) {
        // NSLog(@"Field:\n%@", field);
        return YES;
      }
      recordCallback:^BOOL(const NSUInteger row,
                           const NSArray<NSString *> *record,
                           CGFloat complete) {
        // NSLog(@"record:\n%@", record);
        return YES;
      }];
}
#endif

#ifdef HAVE_OBJ_SUPPORT
TEST(CSVTests, ObjcBuiltInGuess) {
  NSURL *url = [self resourceWithName:@"korean-small" extension:@"csv"];
  XCTAssertNotNil(url);

  NSMutableArray *records = [NSMutableArray arrayWithCapacity:20];

  NSData *data = [NSData dataWithContentsOfURL:url];
  XCTAssertNotNil(data);
  DSFCSVDataSource *source =
      [DSFCSVDataSource dataSourceWithData:data separator:','];
  XCTAssertNotNil(source);

  [DSFCSVParser parseWithDataSource:source
                      fieldCallback:nil
                     recordCallback:^BOOL(const NSUInteger row,
                                          const NSArray<NSString *> *record,
                                          CGFloat complete) {
                       [records addObject:record];
                       return YES;
                     }];

  ASSERT_EQ(20, [records count]);
  ASSERT_EQObjects(records[0][0], @"단행본");  // First cell
  ASSERT_EQObjects(records[19][3], @"미진사"); // Random middle cell
  ASSERT_EQObjects(records[7][5],
                   @"650.4-유14-개정증보판");            // Random middle cell
  ASSERT_EQObjects(records[19][7], @"인쇄자료(책자형)"); // Last cell
}
#endif

TEST(CSVTests, SeparatorAtEndOfLineAndFile) {
  csv::utf8::StringDataSource input;
  ASSERT_TRUE(input.set(",,a,,\n,,b,,\n,,c,,"));
  std::vector<csv::record> records = AddRecords(input);

  ASSERT_EQ(3, records.size());
  ASSERT_EQ(5, records[0].size());
  ASSERT_EQ(5, records[1].size());
  ASSERT_EQ(5, records[2].size());
}

TEST(CSVTests, QuoteAtEndOfLineAndFile) {
  csv::utf8::StringDataSource input;
  ASSERT_TRUE(input.set(",,a,,\n,,b,,\n,,c,,\""));
  std::vector<csv::record> records = AddRecords(input);

  ASSERT_EQ(3, records.size());
  ASSERT_EQ(5, records[0].size());
  ASSERT_EQ(5, records[1].size());
  ASSERT_EQ(5, records[2].size());
}

TEST(CSVTests, Exception) {
  bool caught = false;

  try {
    csv::utf8::FileDataSource input("/tmp/blah.12345");
  } catch (const csv::file_exception &e) {
    caught = true;
  }
  ASSERT_TRUE(caught) << "Didn't catch file exception";

  // Check ICU exception
  caught = false;

#ifdef ALLOW_ICU_EXTENSIONS
  const auto url = getResource("korean-small.csv");
  ASSERT_FALSE(url.empty());
  try {
    csv::icu::FileDataSource input(url.fileSystemRepresentation, "asdf");
  } catch (const csv::file_exception &e) {
    caught = true;
  }
  ASSERT_TRUE(caught) << "Didn't catch file exception";
#endif
}

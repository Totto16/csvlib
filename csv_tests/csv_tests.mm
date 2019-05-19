//
//  csv_tests.m
//  csv_tests
//
//  Created by Darren Ford on 7/10/18.
//  Copyright © 2018 Darren Ford. All rights reserved.
//
//  MIT license
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all copies or substantial
//  portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
//  OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
//  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#import <XCTest/XCTest.h>

#include <csv/parser.hpp>
#include <csv/datasource/utf8/DataSource.hpp>
#include <csv/datasource/icu/DataSource.hpp>

#import <csv/objc/DSFCSVParser.h>

@interface csv_tests : XCTestCase

@end

@implementation csv_tests

- (void)setUp {
	// Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
	// Put teardown code here. This method is called after the invocation of each test method in the class.
}

std::vector<csv::record> AddRecords(csv::IDataSource& data) {
	std::vector<csv::record> records;
	auto recordAdder = [&records](const csv::record& record) -> bool {
		records.push_back(record);
		return true;
	};
	csv::parse(data, NULL, recordAdder);
	return records;
}

std::vector<csv::record> AddRecordsMax(csv::IDataSource& data, size_t maxCount) {
	std::vector<csv::record> records;
	auto recordAdder = [&](const csv::record& record) -> bool {
		assert(record.row == records.size());
		records.push_back(record);
		return (record.row != maxCount - 1);
	};
	csv::parse(data, NULL, recordAdder);
	return records;
}

std::vector<csv::record> AddRecords(const std::string& str) {
	std::vector<csv::record> records;

	csv::utf8::StringDataSource input;
	input.set(str);

	auto recordAdder = [&](const csv::record& record) -> bool {
		records.push_back(record);
		return true;
	};

	csv::parse(input, NULL, recordAdder);
	return records;
}

- (void)testSimple {
	std::vector<csv::record> records;

	csv::utf8::StringDataSource input;
	XCTAssertTrue(input.set("cat, dog, fish"));
	records = AddRecords(input);
	XCTAssertEqual(1, records.size());
	XCTAssertEqual(3, records[0].size());
	XCTAssertEqual("cat", records[0][0].content);
	XCTAssertEqual("dog", records[0][1].content);
	XCTAssertEqual("fish", records[0][2].content);

	XCTAssertTrue(input.set("cat, \"dog\", fi\"sh"));
	records = AddRecords(input);
	XCTAssertEqual(1, records.size());
	XCTAssertEqual(3, records[0].size());
	XCTAssertEqual("cat", records[0][0].content);
	XCTAssertEqual("dog", records[0][1].content);
	XCTAssertEqual("fi\"sh", records[0][2].content);

	XCTAssertTrue(input.set("cat, \"do\"\"g\", fish   "));
	records = AddRecords(input);
	XCTAssertEqual(1, records.size());
	XCTAssertEqual(3, records[0].size());
	XCTAssertEqual("cat", records[0][0].content);
	XCTAssertEqual("do\"g", records[0][1].content);
	XCTAssertEqual("fish   ", records[0][2].content);

	input.trimLeadingWhitespace = false;
	XCTAssertTrue(input.set("   cat, \"do\"\"g\", fish   "));
	records = AddRecords(input);
	XCTAssertEqual(1, records.size());
	XCTAssertEqual(3, records[0].size());
	XCTAssertEqual("   cat", records[0][0].content);
	XCTAssertEqual(" \"do\"g\"", records[0][1].content);
	XCTAssertEqual(" fish   ", records[0][2].content);
}

- (NSURL*)resourceWithName:(NSString*)name extension:(NSString*)extension {
	NSBundle* bundle = [NSBundle bundleForClass:[self class]];
	return [bundle URLForResource:name withExtension:extension];
}

- (void)checkRowIndexes:(const std::vector<csv::record>&)records {
	// Check that the row values match.  This
	size_t count = 0;
	for (const auto& record: records)
	{
		XCTAssertEqual(count++, record.row);
	}
}

- (void)testExample {
	csv::utf8::FileDataSource input;

	NSURL* url = [self resourceWithName:@"orig" extension:@"csv"];
	XCTAssertNotNil(url);

	XCTAssertTrue(input.open([url fileSystemRepresentation]));

	std::vector<csv::record> records = AddRecords(input);
	XCTAssertEqual(8, records.size());

	XCTAssertEqual("John \n\"Da Man\" 大夨天 🤪 太夫", records[0][0].content);

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

- (void)testExceptions {
	// Invalid filename
	XCTAssertThrows(csv::utf8::FileDataSource("caterpillar"));

	// Invalid filename
	XCTAssertThrows(csv::icu::FileDataSource("caterpillar", NULL));

	// Now check with a proper file
	NSURL* url = [self resourceWithName:@"orig" extension:@"csv"];
	XCTAssertNotNil(url);
	XCTAssertNoThrow(csv::utf8::FileDataSource([url fileSystemRepresentation]));

	// Check throw if we pass a dodgy codepage
	XCTAssertThrows(csv::icu::FileDataSource([url fileSystemRepresentation], "cccc"));
}

- (void)testRFC {
	std::string i26 = "\"aaa\",\"b \r\nbb\",\"ccc\" \r\n	zzz,yyy,xxx";
	std::vector<csv::record> records = AddRecords(i26);

	XCTAssertEqual(2, records.size());
	XCTAssertEqual(3, records[0].size());
	XCTAssertEqual(3, records[1].size());

	XCTAssertEqual("aaa", records[0][0].content);
	XCTAssertEqual("b \r\nbb", records[0][1].content);
	XCTAssertEqual("ccc", records[0][2].content);

	XCTAssertEqual("\tzzz", records[1][0].content);
	XCTAssertEqual("yyy", records[1][1].content);
	XCTAssertEqual("xxx", records[1][2].content);

	std::string i27 = "aaa,b\"\"bb,ccc";

	records = AddRecords(i27);
	XCTAssertEqual(3, records[0].size());

	XCTAssertEqual("aaa", records[0][0].content);
	XCTAssertEqual("b\"bb", records[0][1].content);
	XCTAssertEqual("ccc", records[0][2].content);

	std::string i25 = "\"aaa\",\"bbb\",\"ccc\"\r\nzzz,yyy,xxx";
	records = AddRecords(i25);
	XCTAssertEqual(2, records.size());

	XCTAssertEqual(3, records[0].size());
	XCTAssertEqual("aaa", records[0][0].content);
	XCTAssertEqual("bbb", records[0][1].content);
	XCTAssertEqual("ccc", records[0][2].content);

	XCTAssertEqual(3, records[1].size());
	XCTAssertEqual("zzz", records[1][0].content);
	XCTAssertEqual("yyy", records[1][1].content);
	XCTAssertEqual("xxx", records[1][2].content);
}

- (void)testWhitespaceBeforeEOL {
	std::vector<csv::record> records;

	csv::utf8::StringDataSource input;
	XCTAssertTrue(input.set("cat, dog, fish, \nwhale, pig, snork"));
	records = AddRecords(input);

	XCTAssertEqual(2, records.size());

	XCTAssertEqual(4, records[0].size());
	XCTAssertEqual(3, records[1].size());

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

- (void)testQuoting {
	csv::utf8::StringDataSource input;
	XCTAssertTrue(input.set("\"cat\",     \"dog\"   , \"fish\", \nwhale, pig, snork"));
	std::vector<csv::record> records = AddRecords(input);

	XCTAssertEqual(2, records.size());

	XCTAssertEqual(4, records[0].size());
	XCTAssertEqual(3, records[1].size());

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

- (void)testComment {
	csv::utf8::StringDataSource input;
	input.comment = '#';

	// Comments should only start at the beginning of a record.  Any field containing or starting with the comment
	// character should not be treated as a comment
	XCTAssertTrue(input.set("# Here is my amazing, and \"groovy\", commented file!\r"
							"cat,#dog,pig\r\n"
							"#Here is the next comment...\n"
							"\"cat\n#fish\", truck, snozzle"));
	std::vector<csv::record> records = AddRecords(input);

	XCTAssertEqual(2, records.size());
	XCTAssertEqual(3, records[0].size());
	XCTAssertEqual(3, records[1].size());

	XCTAssertEqual("cat", records[0][0].content);
	XCTAssertEqual("#dog", records[0][1].content);
	XCTAssertEqual("pig", records[0][2].content);

	XCTAssertEqual("cat\n#fish", records[1][0].content);
	XCTAssertEqual("truck", records[1][1].content);
	XCTAssertEqual("snozzle", records[1][2].content);

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

- (void)testTinyFileSmallerThanUTF8BOM {
	// Check filesize less than BOM
	std::vector<csv::record> records;

	csv::utf8::StringDataSource input;
	XCTAssertTrue(input.set("a"));
	records = AddRecords(input);

	XCTAssertEqual(1, records.size());
	XCTAssertEqual(1, records[0].size());
	XCTAssertEqual("a", records[0][0].content);
}

- (void)testUTF8BOMRemoval {
	csv::utf8::StringDataSource input;
	XCTAssertTrue(input.set("\xEF\xBB\xBF cat, dog, fish"));
	std::vector<csv::record> records = AddRecords(input);

	XCTAssertEqual(1, records.size());
	XCTAssertEqual(3, records[0].size());

	XCTAssertEqual("cat", records[0][0].content);
	XCTAssertEqual("dog", records[0][1].content);
	XCTAssertEqual("fish", records[0][2].content);

	// Now the file version
	csv::utf8::FileDataSource parser;
	parser.separator = '\t';
	NSURL* url = [self resourceWithName:@"simple_csv_utf8_bom" extension:@"tsv"];
	XCTAssertNotNil(url);

	XCTAssertTrue(parser.open([url fileSystemRepresentation]));

	records = AddRecords(parser);
	XCTAssertEqual(1, records.size());
	XCTAssertEqual("fish", records[0][0].content);
	XCTAssertEqual("pig", records[0][1].content);
	XCTAssertEqual("snort", records[0][2].content);

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

- (void)testColumnOffsetCounter {
	csv::utf8::StringDataSource input;
	input.separator = '\t';
	input.trimLeadingWhitespace = false;
	XCTAssertTrue(input.set("cat\tdog\tfish\t\nwhale\tpig\t snork"));

	std::vector<csv::record> records;

	std::vector<size_t> expected = {0, 1, 2, 3, 0, 1, 2};
	std::vector<size_t> fields;

	auto fieldChecker = [&fields](const csv::field& field) -> bool {
		fields.push_back(field.column);
		return true;
	};

	auto recordAdder = [&records](const csv::record& record) -> bool {
		records.push_back(record);
		return true;
	};
	csv::parse(input, fieldChecker, recordAdder);

	// Check field ordering
	XCTAssertEqual( expected, fields );

	// Check that the row values match.  This
	[self checkRowIndexes:records];

	XCTAssertEqual(2, records.size());
	XCTAssertEqual(4, records[0].size());
	XCTAssertEqual(3, records[1].size());
}

- (void)testSeparator {
	csv::utf8::StringDataSource input;
	input.separator = '\t';
	input.trimLeadingWhitespace = false;
	XCTAssertTrue(input.set("cat\tdog\tfish\t\nwhale\tpig\t snork"));
	std::vector<csv::record> records = AddRecords(input);

	XCTAssertEqual(2, records.size());
	XCTAssertEqual(4, records[0].size());
	XCTAssertEqual(3, records[1].size());

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

/// I had an issue if the file ended on a separator.  This tests for that.
- (void)testLastCharacterIsASeparator {
	csv::utf8::StringDataSource input;
	input.separator = '\t';
	input.trimLeadingWhitespace = false;

	// Separator at end of line means an empty field finishes the row
	// Separator at end of file means that an empty field finishes the file
	XCTAssertTrue(input.set("cat\tdog\tfish\t\nwhale\tpig\t snork\t"));
	std::vector<csv::record> records = AddRecords(input);

	XCTAssertEqual(2, records.size());
	XCTAssertEqual(4, records[0].size());
	XCTAssertEqual(4, records[1].size());

	// Check that the row values match.
	[self checkRowIndexes:records];
}

- (void)testExampleMax {
	csv::icu::FileDataSource input;

	NSURL* url = [self resourceWithName:@"korean" extension:@"csv"];
	XCTAssertNotNil(url);
	XCTAssertTrue(input.open([url fileSystemRepresentation], NULL));

	std::vector<csv::record> records = AddRecordsMax(input, 4);
	XCTAssertEqual(4, records.size());

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

- (void)testTotalAColumn {
	csv::icu::FileDataSource input;

	NSURL* url = [self resourceWithName:@"ford_escort" extension:@"csv"];
	XCTAssertNotNil(url);
	XCTAssertTrue(input.open([url fileSystemRepresentation], NULL));

	size_t count = 0;
	auto fieldAdder = [&count](const csv::field& field) -> bool {
		// Add all entries in the mileage column
		if (field.row != 0 && field.column == 2) {
			count += std::stoi( field.content );
		}
		return true;
	};
	csv::parse(input, fieldAdder, NULL);

	// Check the total mileage for all entries
	XCTAssertEqual(count, 214642);
}

- (void)testObjc {
	DSFCSVDataSource* source = [DSFCSVDataSource dataSourceWithUTF8String:@"cat, dog, fish, whale\ngoober, nostradamus" separator:','];

	[DSFCSVParser parseWithDataSource:source
						fieldCallback:^BOOL(const NSUInteger row, const NSUInteger column, const NSString* field) {
							NSLog(@"Field (%ld, %ld): %@", row, column, field);
							return YES;
						}
					   recordCallback:^BOOL(const NSUInteger row, const NSArray<NSString*>* record) {
						   NSLog(@"record:\n%@", record);
						   return YES;
					   }];
}

- (void)testFileWithBlankLinesAndLineEndingsInQuotedStrings {
	csv::utf8::FileDataSource input;

	NSURL* url = [self resourceWithName:@"classification" extension:@"csv"];
	XCTAssertNotNil(url);

	XCTAssertTrue(input.open([url fileSystemRepresentation]));

	std::vector<csv::record> records = AddRecords(input);
	XCTAssertEqual(10, records.size());

	// Check that the row values match.  This
	[self checkRowIndexes:records];

	// Parse the same file, but keep empty lines.
	csv::utf8::FileDataSource input2;
	input2.skipBlankLines = false;

	XCTAssertTrue(input2.open([url fileSystemRepresentation]));

	records = AddRecords(input2);
	XCTAssertEqual(29, records.size());

	// Check that the row values match.  This
	[self checkRowIndexes:records];
}

- (void)testObjcICU {
	NSURL* url = [self resourceWithName:@"korean" extension:@"csv"];
	XCTAssertNotNil(url);

	DSFCSVDataSource* source = [DSFCSVDataSource dataSourceWithFileURL:url
														   icuCodepage:NULL
															 separator:','];
	XCTAssertNotNil(source);

	[DSFCSVParser parseWithDataSource:source
						fieldCallback:^BOOL(const NSUInteger row, const NSUInteger column, const NSString* field) {
							// NSLog(@"Field:\n%@", field);
							return YES;
						} recordCallback:^BOOL(const NSUInteger row, const NSArray<NSString*>* record) {
							// NSLog(@"record:\n%@", record);
							return YES;
						}];
}

- (void)testObjcBuiltInGuess {
	NSURL* url = [self resourceWithName:@"korean-small" extension:@"csv"];
	XCTAssertNotNil(url);

	NSMutableArray* records = [NSMutableArray arrayWithCapacity:20];

	NSData* data = [NSData dataWithContentsOfURL:url];
	XCTAssertNotNil(data);
	DSFCSVDataSource* source = [DSFCSVDataSource dataSourceWithData:data separator:','];
	XCTAssertNotNil(source);

	[DSFCSVParser parseWithDataSource:source
						fieldCallback:nil
					   recordCallback:^BOOL(const NSUInteger row, const NSArray<NSString*>* record) {
						   [records addObject:record];
						   return YES;
					   }];

	XCTAssertEqual(20, [records count]);
	XCTAssertEqualObjects(records[0][0], @"단행본");				// First cell
	XCTAssertEqualObjects(records[19][3], @"미진사");				// Random middle cell
	XCTAssertEqualObjects(records[7][5], @"650.4-유14-개정증보판");	// Random middle cell
	XCTAssertEqualObjects(records[19][7], @"인쇄자료(책자형)");		// Last cell
}

- (void)testSeparatorAtEndOfLineAndFile {
	csv::utf8::StringDataSource input;
	XCTAssertTrue(input.set(",,a,,\n,,b,,\n,,c,,"));
	std::vector<csv::record> records = AddRecords(input);

	XCTAssertEqual(3, records.size());
	XCTAssertEqual(5, records[0].size());
	XCTAssertEqual(5, records[1].size());
	XCTAssertEqual(5, records[2].size());
}

- (void)testQuoteAtEndOfLineAndFile {
	csv::utf8::StringDataSource input;
	XCTAssertTrue(input.set(",,a,,\n,,b,,\n,,c,,\""));
	std::vector<csv::record> records = AddRecords(input);

	XCTAssertEqual(3, records.size());
	XCTAssertEqual(5, records[0].size());
	XCTAssertEqual(5, records[1].size());
	XCTAssertEqual(5, records[2].size());
}

@end

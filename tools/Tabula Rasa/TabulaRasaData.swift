//
//  TabulaRasaData.swift
//  Tabula Rasa
//
//  Created by Darren Ford on 13/10/18.
//  Copyright © 2018 Darren Ford. All rights reserved.
//

import Cocoa

class TabulaRasaData: NSObject {

	private let url: URL
	private var cancelled: Bool = false
	private var loading: Bool = false

	private let group = DispatchGroup()

	enum FileType: NSInteger
	{
		case csv = 0
		case tsv = 1
	}

	let type: FileType
	private(set) var rawData: [[String]] = [[String]]()

	init(url: URL, type: FileType) {
		self.url = url
		self.type = type
		super.init()
	}

	private func addRecords(record: [String])
	{
		self.rawData.append(record);
	}

	func cancel()
	{
		if (self.loading)
		{
			self.cancelled = true
			self.group.wait()
		}
	}

	func load(async completion: @escaping () -> Void) -> Bool
	{
		assert(self.loading == false)

		// Prepare
		self.cancelled = false
		self.rawData.removeAll()

		let sep: UnicodeScalar = self.type == .csv ? "," : "\t"
		let separator = Int8(sep.value)

		let source = DSFCSVDataSource.init(fileURL: self.url, icuCodepage: nil, separator: separator)
		guard let dataSource = source else
		{
			return false;
		}

		DispatchQueue.global(qos: .userInitiated).async
		{ [weak self] in
			if let blockSelf = self
			{
				blockSelf.load(source: dataSource, completion: completion)
			}
		}

		return true;
	}

	private func load(source: DSFCSVDataSource, completion: @escaping () -> Void)
	{
		self.loading = true

		self.group.enter()

		DSFCSVParser.parse(with: source,
						   fieldCallback: nil)
		{ (row: UInt, record: [String]) -> Bool in
			self.addRecords(record: record)
			return !self.cancelled
		}

		self.loading = false
		self.group.leave()

		if !self.cancelled
		{
			/// Only call the completion block if we weren't cancelled
			DispatchQueue.main.async {
				completion()
			}
		}
	}

}

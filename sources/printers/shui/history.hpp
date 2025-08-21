#pragma once

#include "printers/history.hpp"
#include "printers/storage.hpp"
#include "printers/state.hpp"

class ShuiPrinterHistory: public PrinterHistory {
	std::vector<HistoryEntry> m_History;
	std::filesystem::path m_HistoryPath;

	PrinterStorage &m_Storage;

	std::optional<HistoryEntry> m_PendingEntry;
	std::optional<PrinterState> m_LastState;
public:
	ShuiPrinterHistory(const std::filesystem::path &filepath, PrinterStorage &storage);

	const std::vector<HistoryEntry> &GetHistory()const override{ return m_History; }

	void Load();

	void Save();

	void OnStateChanged(std::optional<PrinterState> state);

	void Emit(const HistoryEntry &entry);
};
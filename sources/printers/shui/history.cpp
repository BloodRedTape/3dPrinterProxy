#include "history.hpp"
#include <bsl/file.hpp>
#include <bsl/log.hpp>

DEFINE_LOG_CATEGORY(ShuiPrinterHistory)

static UnixTime Now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

ShuiPrinterHistory::ShuiPrinterHistory(const std::filesystem::path& filepath, PrinterStorage &storage):
	m_HistoryPath(filepath),
	m_Storage(storage)
{
	Load();
}

void ShuiPrinterHistory::Load() {
	try{
		m_History = nlohmann::json::parse(File::ReadEntire(m_HistoryPath), nullptr, false, false);
	} catch (const std::exception &e) {
		LogShuiPrinterHistory(Error, "Can't read history: %", e.what());
	}
}

void ShuiPrinterHistory::Save() {
	File::WriteEntire(m_HistoryPath, nlohmann::json(m_History).dump());
}

void ShuiPrinterHistory::OnStateChanged(std::optional<PrinterState> state){
	if (m_PendingEntry && (!state || !state->Print || !state->Print->Filename.size())) {
		m_PendingEntry->PrintEnd = Now();

		if(m_LastState && m_LastState->Print){
			const auto &print = *m_LastState->Print;
			auto bytes_left = print.TargetBytesPrinted - print.CurrentBytesPrinted;

			m_PendingEntry->FinishReason = print.Progress >= 99 || bytes_left < print.TargetBytesPrinted * 0.02 ? PrintFinishReason::Complete : PrintFinishReason::Interrupted;
		} else {
			m_PendingEntry->FinishReason = PrintFinishReason::Unknown;
		}
		
		if(m_LastState->Print)
			m_PendingEntry->LastKnownPrintState = *m_LastState->Print;

		Emit(*m_PendingEntry);

		m_PendingEntry = {};
	}

	if(!m_PendingEntry && state && state->Print && state->Print->Filename.size()) {
		m_PendingEntry = HistoryEntry();
		m_PendingEntry->PrintStart = Now();
		m_PendingEntry->Filename = state->Print->Filename;
		m_PendingEntry->FileId = ToString(m_Storage.GetContentHashForFilename(m_PendingEntry->Filename).value_or(0));
	}

	m_LastState = state;
}

void ShuiPrinterHistory::Emit(const HistoryEntry& entry){
	m_History.push_back(entry);

	Save();
}

#pragma once

#include <cassert>
#include <string>
#include <functional>
#include <optional>
#include <queue>

#define SHUI_VERBOSE_LOGGING 0

enum class GCodeState {
	Enqueued,
	Sent	
};

struct GCodeSubmissionState {
	using OnResultType = std::function<void(std::optional<std::string>)>;

	std::string GCode;
	OnResultType OnResult;
	std::int64_t Retries = 0;
	GCodeState State = GCodeState::Enqueued;
	std::string ResultAccumulator;
	std::int64_t SubmitedAfterLine = 0;
	std::int64_t SystemLinesAfterSubmission = 0;
	
	bool CanRetry()const {
		return Retries > 0;
	}

	void MakeRetry() {
		assert(CanRetry());
		
		Retries--;
		State = GCodeState::Enqueued;
		ResultAccumulator = "";
		SubmitedAfterLine = 0;
		SystemLinesAfterSubmission = 0;
	}
};

class GCodeExecutionEngine {
	std::queue<GCodeSubmissionState> m_SubmittedCommands;

	static constexpr std::int64_t PreambleLinesCount = 3;
	static constexpr std::int64_t MaxSystemLinesAfterSubmission = 3;
public:
	std::function<void(const std::string &)> WriteGCode;
public:

	void Submit(std::string gcode, GCodeSubmissionState::OnResultType on_result, std::int64_t retries);

	static bool IsSystemLine(const std::string& line);

	static bool IsBusySystemLine(const std::string &line);

	static bool IsOkSystemLine(const std::string &line);

	void OnReadingDone(std::int64_t last_index);

	void OnLine(const std::string& line, std::int64_t index);

	void CancelAll();

	bool AllDone()const {
		return !m_SubmittedCommands.size();
	}
};

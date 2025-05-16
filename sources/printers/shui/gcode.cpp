#include "gcode.hpp"
#include <boost/algorithm/string.hpp>
#include <bsl/log.hpp>

DEFINE_LOG_CATEGORY(GCodeExecution)

void GCodeExecutionEngine::Submit(std::string gcode, GCodeSubmissionState::OnResultType on_result, std::int64_t retries) {
	m_SubmittedCommands.push({gcode, on_result, retries});
}

bool GCodeExecutionEngine::IsSystemLine(const std::string& line){
	return IsBusySystemLine(line) || IsOkSystemLine(line);
}

bool GCodeExecutionEngine::IsBusySystemLine(const std::string& line) {
	return boost::starts_with(line, "busy") 
		//|| boost::starts_with(line, "echo") 
		|| boost::starts_with(line, "busyok T0:") 
		|| boost::starts_with(line, "busyT0:") ;
}

bool GCodeExecutionEngine::IsOkSystemLine(const std::string& line) {
	return boost::starts_with(line, "ok") 
		|| boost::starts_with(line, "ok T0:") 
		|| boost::starts_with(line, "T0");
}

void GCodeExecutionEngine::OnReadingDone(std::int64_t last_index) {

	if(!m_SubmittedCommands.size() || last_index <= PreambleLinesCount)
		return;

	GCodeSubmissionState &current = m_SubmittedCommands.front();

	if(current.State == GCodeState::Enqueued){
		LogGCodeExecutionIf(!WriteGCode, Error, "GCode writing callback is null");

		if(WriteGCode) WriteGCode(current.GCode);
#if SHUI_VERBOSE_LOGGING
		Println("[Written]: %", current.GCode);
#endif

		current.State = GCodeState::Sent;
		current.SubmitedAfterLine = last_index;
	}
}

void GCodeExecutionEngine::OnLine(const std::string& line, std::int64_t index) {
	if(!m_SubmittedCommands.size())
		return;

	GCodeSubmissionState &current = m_SubmittedCommands.front();
	if (current.State != GCodeState::Sent)
		return;

	auto FinishCurrentCommand = [&](std::optional<std::string> result) {
		if (!result.has_value() && current.CanRetry()) {
			current.MakeRetry();
			return;
		}

		if(current.OnResult) current.OnResult(result);

		m_SubmittedCommands.pop();
	};

	auto FinishWithAccumulator = [&]{
		if(current.ResultAccumulator.size())
			current.ResultAccumulator.pop_back();

		FinishCurrentCommand(current.ResultAccumulator);
	};

	//connection restarted
	if (index < current.SubmitedAfterLine)
		return FinishCurrentCommand(std::nullopt);

	if (!IsSystemLine(line)) {
		current.ResultAccumulator += line + "\n";
		return;
	}
	
	//System Line
	if(current.ResultAccumulator.size())
		return FinishWithAccumulator();

	current.SystemLinesAfterSubmission++;

	if (current.SystemLinesAfterSubmission > MaxSystemLinesAfterSubmission) 
		FinishWithAccumulator();
}

void GCodeExecutionEngine::CancelAll() {
	while(m_SubmittedCommands.size())
		m_SubmittedCommands.pop();
}
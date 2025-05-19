#include "printer.hpp"
#include <boost/algorithm/string.hpp>
#include "upload.hpp"
#include <chrono>
#include <queue>
#include <bsl/log.hpp>

DEFINE_LOG_CATEGORY(Shui)

void LogShuiIf(const boost::system::error_code &ec) {
	LogShuiIf((bool)ec, Error, "%", ec.what());
}

ShuiPrinter::ShuiPrinter(std::string ip, std::uint16_t port, const std::filesystem::path &data_path):
    m_DataPath(data_path),
	m_Ip(std::move(ip)),
	m_Port(port),
    m_Storage(m_Ip, data_path / "storage")
{
	m_Connection = std::make_unique<ShuiPrinterConnection>(m_Ip, m_Port);
	m_Connection->OnConnect = std::bind(&ShuiPrinter::OnConnectionConnect, this);
	m_Connection->OnTick = std::bind(&ShuiPrinter::OnConnectionTick, this);
	m_Connection->OnTimeout = std::bind(&ShuiPrinter::OnConnectionTimeout, this, std::placeholders::_1);
	m_Connection->OnPrinterLine = std::bind(&ShuiPrinter::OnConnectionPrinterLine, this, std::placeholders::_1, std::placeholders::_2);
}

void ShuiPrinter::RunAsync(){
	m_Connection->RunAsync();

    //m_Connection->SubmitGCodeAsync("M2020", GCodeExecutionEngine::VerboseGCodeCallback, 1);
    
//#define RETRIES_TEST
#ifdef RETRIES_TEST
    static std::int64_t success, fail;
    auto Handle = [](std::optional<std::string> result) {
        if(result.has_value())
            success++;
        else
            fail++;
    };

    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", Handle, 1);
    m_Connection->SubmitGCode("M20", [&](auto result) {
        Handle(result);

        Println("Success: %, Fail: %", success, fail);
    }, 1);
#endif
}

void ShuiPrinter::IdentifyAsync() {
    m_Connection->SubmitGCodeAsync("M300", GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::SetTargetBedTemperatureAsync(std::int64_t temperature){
    m_Connection->SubmitGCodeAsync(Format("M140 S%", temperature), GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::SetTargetExtruderTemperatureAsync(std::int64_t temperature){
    m_Connection->SubmitGCodeAsync(Format("M104 T0 S%", temperature), GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

static std::string NormalizeMessage(std::string message) {
    auto pos = message.find(ShuiPrinterConnection::PrinterStreamLineSeparator);

    if(pos != std::string::npos){
        message = message.substr(0, pos);
        LogShui(Warning, "SetLCDMessage: found PrinterStreamLineSeparator in message, trimming...");
    }

    return message;
}

void ShuiPrinter::SetLCDMessageAsync(std::string message) {
    m_Connection->SubmitGCodeAsync(Format("M117 %", NormalizeMessage(message)), GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::SetDialogMessageAsync(std::string message, std::optional<int> display_time){
    m_Connection->SubmitGCodeAsync(Format("M2011% %", display_time.has_value() ? Format(" S%", display_time.value()) : "", NormalizeMessage(message)), GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::SetFanSpeedAsync(std::uint8_t speed){
    m_Connection->SubmitGCodeAsync(Format("M106 S%", (int)speed), GCodeExecutionEngine::DefaultGCodeCallback, 1);
}
void ShuiPrinter::PauseUntillUserInputAsync(std::string message){
    m_Connection->SubmitGCodeAsync(Format("M0 %", message), GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::PausePrintAsync(){
    m_Connection->SubmitGCodeAsync("M25", GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::ResumePrintAsync(){
    m_Connection->SubmitGCodeAsync("M24", GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::ReleaseMotorsAsync() {
    m_Connection->SubmitGCodeAsync("M84", GCodeExecutionEngine::DefaultGCodeCallback, 1);
}

void ShuiPrinter::CancelPrintAsync() {
    //XXX find a way to reliably send a gcode sequence
    PausePrintAsync();
    //XXX stop print
    m_Connection->SubmitGCodeAsync("G91", GCodeExecutionEngine::DefaultGCodeCallback, 1);
    m_Connection->SubmitGCodeAsync("G1 Z20", GCodeExecutionEngine::DefaultGCodeCallback, 1);
    m_Connection->SubmitGCodeAsync("M84", GCodeExecutionEngine::DefaultGCodeCallback, 1);
    ReleaseMotorsAsync();
    SetFanSpeedAsync(0);
}

ShuiPrinterStorage& ShuiPrinter::Storage(){
    return m_Storage;
}

void ShuiPrinter::OnConnectionConnect() {
    //SubmitReportSequence();
}

void ShuiPrinter::OnConnectionTick() {

}

void ShuiPrinter::OnConnectionTimeout(std::int64_t timeout) {
#if SHUI_VERBOSE_LOGGING
    LogShui(Display, "\tTimeout");
#endif

	if(timeout >= 3 && m_State.has_value()){
		m_State = std::nullopt;
		
		std::call(OnStateChanged);

        m_Connection->CancelAllGCode();
	}
}

void ShuiPrinter::OnConnectionPrinterLine(const std::string& line, std::int64_t index) {
	if (!GCodeExecutionEngine::IsSystemLine(line))
		return;

    SubmitReportSequenceAsync();

    UpdateStateFromSystemLine(line);
}

void ShuiPrinter::SubmitReportSequenceAsync() {
    if(!m_Connection->GCodeDone())
        return;

    m_Connection->SubmitGCodeAsync("M27", [&](std::optional<std::string> result) {
        if(!result.has_value()){
#if SHUI_VERBOSE_LOGGING
            Println("\tM27 Failed");
#endif
            return;
        }

#if SHUI_VERBOSE_LOGGING
        Println("\tM27: %", result.value());
#endif
        
        UpdateStateFromSdCardStatus(result.value());
    });

    m_Connection->SubmitGCodeAsync("M27 C", [&](std::optional<std::string> result) {
        if(!result.has_value()){
#if SHUI_VERBOSE_LOGGING
            Println("\tM27 C Failed");
#endif
            return;
        }

#if SHUI_VERBOSE_LOGGING
        Println("\tM27 C: %", result.value());
#endif
        UpdateStateFromSelectedFile(result.value());
    });

    m_Connection->SubmitGCodeAsync("M123", [&](std::optional<std::string> result) {
        if(!result.has_value()){
#if SHUI_VERBOSE_LOGGING
            Println("\tM123 Failed");
#endif
            return;
        }

#if SHUI_VERBOSE_LOGGING
        Println("\tM123: %", result.value());
#endif
        UpdateStateFromSelectedFile(result.value());
    });
}

void ShuiPrinter::UpdateStateFromSystemLine(const std::string& line) {
    auto& state = State();

    std::istringstream iss(line);
    std::string token;

    bool changed = false;

    if (GCodeExecutionEngine::IsBusySystemLine(line)) {
        if (!state.Print.has_value()) {
            state.Print = PrintState();
            changed = true;
        }

        auto& print = state.Print.value();

        if (print.Status != PrintStatus::Busy) {
            print.Status = PrintStatus::Busy;
            changed = true;
        }
    }

    while (iss >> token) {
        if (token.rfind("T0:", 0) == 0) {
            token = token.substr(3);
            std::string other;
            iss >> other;
            token += other;
            size_t slashPos = token.find('/');
            if (slashPos != std::string::npos) {
                try {
                    float newValue = std::round(std::stof(token.substr(0, slashPos)));
                    float newTargetValue = std::round(std::stof(token.substr(slashPos + 1)));

                    if(state.ExtruderTemperature != newValue)
                        changed = true; 
                    if(state.TargetExtruderTemperature != newTargetValue)
                        changed = true; 

                    state.ExtruderTemperature = newValue;
                    state.TargetExtruderTemperature = newTargetValue;
                } catch (const std::exception&) { }
            }
        } else if (token.rfind("B:", 0) == 0) {
            token = token.substr(2); // Remove "B:"
            std::string other;
            iss >> other;
            token += other;
            size_t slashPos = token.find('/');
            if (slashPos != std::string::npos) {
                try {
                    float newValue = std::round(std::stof(token.substr(0, slashPos)));
                    float newTargetValue = std::round(std::stof(token.substr(slashPos + 1)));

                    if(state.BedTemperature != newValue)
                        changed = true; 
                    if(state.TargetBedTemperature != newTargetValue)
                        changed = true; 

                    state.BedTemperature = newValue;
                    state.TargetBedTemperature = newTargetValue;
                } catch (const std::exception&) {}
            }
        }
    }

    
    if(changed) 
        std::call(OnStateChanged);
}

void ShuiPrinter::UpdateStateFromSdCardStatus(const std::string& line) {
    static const std::string NoSdPrinting = "Not SD printing";
    static const std::string SdPrintingPrefix = "SD printing byte ";

    if (line.find(NoSdPrinting) != std::string::npos && State().Print.has_value()) {
        State().Print = std::nullopt;

        std::call(OnStateChanged);
    }


    auto pos = line.find(SdPrintingPrefix);

    if(pos == std::string::npos)
        return;

    std::string progress = line.substr(pos + SdPrintingPrefix.size());

    pos = progress.find(ShuiPrinterConnection::PrinterStreamLineSeparator);

    if(pos != std::string::npos)
        progress = progress.substr(0, pos);

    auto separator = progress.find('/');

    if(separator == std::string::npos)
        return;

    std::string current_string = progress.substr(0, separator);
    std::string target_string = progress.substr(separator + 1);
    
    bool changed = false;

    if (!State().Print.has_value()){
        State().Print = PrintState();
        changed = true;
    }

    try {
        auto& print = State().Print.value();

        std::int64_t current = std::stoi(current_string);
        std::int64_t target = std::stoi(target_string);
        
        if(print.CurrentBytesPrinted != current)
            changed = true;
        if(print.TargetBytesPrinted != target)
            changed = true;

        if(print.Status != PrintStatus::Printing){
            print.Status = PrintStatus::Printing;
            changed = true;
        }

        print.CurrentBytesPrinted = current;
        print.TargetBytesPrinted = target;

        const GCodeFileRuntimeData *runtime = m_Storage.GetRuntimeData(print.Filename);

        if (runtime) {
            GCodeRuntimeState state = runtime->GetStateNear(current);

            if(print.Progress != state.Percent)
                changed = true;
            if(print.Layer != state.Layer)
                changed = true;
            if(print.Height != state.Height)
                changed = true;

            print.Progress = state.Percent;
            print.Layer = state.Layer;
            print.Height = state.Height;
        }

    }catch(const std::exception& e){ }
    
    if(changed)
        std::call(OnStateChanged);
}

void ShuiPrinter::UpdateStateFromSelectedFile(const std::string& line) {
    static const std::string FilenamePrefix = "Current file: ";
    static const std::string NoFile = "(no file)";

    auto pos = line.find(FilenamePrefix);

    if(pos == std::string::npos)
        return;
    
    std::string filename = line.substr(pos + FilenamePrefix.size());

    pos = filename.find(ShuiPrinterConnection::PrinterStreamLineSeparator);

    if (pos != std::string::npos)
        filename = filename.substr(0, pos);
    
    {
        const std::string *long_filename = m_Storage.GetLongFilename(filename);

        if(long_filename)
            filename = *long_filename;
    }

    bool printing = filename != NoFile;

    if (!State().Print.has_value() && printing && filename.size()) {
        State().Print = PrintState();
        State().Print.value().Filename = filename;

        std::call(OnStateChanged);
        return;
    }

    if (State().Print.has_value() && printing && State().Print.value().Filename != filename && filename.size()) {
        State().Print = PrintState();
        State().Print.value().Filename = filename;

        std::call(OnStateChanged);
        return;
    }

    if (State().Print.has_value() && !printing) {
        State().Print = std::nullopt;

        std::call(OnStateChanged);
        return;
    }
}

std::optional<PrinterState> ShuiPrinter::GetPrinterState() const {
	return m_State;
}

PrinterState& ShuiPrinter::State() {
    if(!m_State.has_value()){
        m_State = PrinterState();
        std::call(OnStateChanged);
    }
    return m_State.value();
}

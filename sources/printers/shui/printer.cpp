#include "printer.hpp"
#include <boost/algorithm/string.hpp>
#include "upload.hpp"
#include <chrono>
#include <queue>
#include <bsl/log.hpp>
#include <bsl/defer.hpp>

DEFINE_LOG_CATEGORY(Shui)

void LogShuiIf(const boost::system::error_code &ec) {
	LogShuiIf((bool)ec, Error, "%", ec.what());
}

ShuiPrinter::ShuiPrinter(std::string ip, std::uint16_t port, const std::filesystem::path &data_path):
    m_DataPath(data_path),
	m_Ip(std::move(ip)),
	m_Port(port),
    m_Storage(m_Ip, data_path / "storage"),
    m_History(data_path / "history.json", m_Storage)
{
	m_Connection = std::make_unique<ShuiPrinterConnection>(m_Ip, m_Port);
	m_Connection->OnConnect = std::bind(&ShuiPrinter::OnConnectionConnect, this);
	m_Connection->OnTick = std::bind(&ShuiPrinter::OnConnectionTick, this);
	m_Connection->OnTimeout = std::bind(&ShuiPrinter::OnConnectionTimeout, this, std::placeholders::_1);
	m_Connection->OnFailedConnect = std::bind(&ShuiPrinter::OnConnectFailed, this, std::placeholders::_1);
	m_Connection->OnPrinterLine = std::bind(&ShuiPrinter::OnConnectionPrinterLine, this, std::placeholders::_1, std::placeholders::_2);

    Manufacturer = "Two Trees";
    Model = "Bluer";
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

void ShuiPrinter::HandleStateChanged(){
    m_History.OnStateChanged(m_State);

    std::call(OnStateChanged);
}

static auto MakeGCodeEngineCallback(const ShuiPrinter *printer, GCodeCallback &&callback) {
    return [callback = std::move(callback), printer](std::optional<std::string> result) {
        if(result.has_value())
            std::call(callback, GCodeResult::Ok);

        if(!printer->GetPrinterState().has_value())
            std::call(callback, GCodeResult::NoConnection);

        std::call(callback, GCodeResult::Busy);
    };
}

void ShuiPrinter::IdentifyAsync(GCodeCallback callback) {
    m_Connection->SubmitGCodeAsync("M300", MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::SetTargetBedTemperatureAsync(std::int64_t temperature, GCodeCallback callback){
    m_Connection->SubmitGCodeAsync(Format("M140 S%", temperature), MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::SetTargetExtruderTemperatureAsync(std::int64_t temperature, GCodeCallback callback){
    m_Connection->SubmitGCodeAsync(Format("M104 T0 S%", temperature), MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::SetFeedRatePercentAsync(float feed_rate, GCodeCallback callback){
    if(feed_rate < 0){
        LogShui(Error, "SetFeedRatePercentAsync: feedrate % is less than zero", feed_rate);
        return;
    }

    m_Connection->SubmitGCodeAsync(Format("M220 S%", (int)feed_rate), MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

static std::string NormalizeMessage(std::string message) {
    auto pos = message.find(ShuiPrinterConnection::PrinterStreamLineSeparator);

    if(pos != std::string::npos){
        message = message.substr(0, pos);
        LogShui(Warning, "SetLCDMessage: found PrinterStreamLineSeparator in message, trimming...");
    }

    return message;
}

void ShuiPrinter::SetLCDMessageAsync(std::string message, GCodeCallback callback) {
    m_Connection->SubmitGCodeAsync(Format("M117 %", NormalizeMessage(message)), MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::SetDialogMessageAsync(std::string message, std::optional<int> display_time, GCodeCallback callback){
    m_Connection->SubmitGCodeAsync(Format("M2011% %", display_time.has_value() ? Format(" S%", display_time.value()) : "", NormalizeMessage(message)), MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::SetFanSpeedAsync(std::uint8_t speed, GCodeCallback callback){
    m_Connection->SubmitGCodeAsync(Format("M106 S%", (int)speed), MakeGCodeEngineCallback(this, std::move(callback)), 1);
}
void ShuiPrinter::PauseUntillUserInputAsync(std::string message, GCodeCallback callback){
    m_Connection->SubmitGCodeAsync(Format("M0 %", message), MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::PausePrintAsync(GCodeCallback callback){
    m_Connection->SubmitGCodeAsync("M25", MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::ResumePrintAsync(GCodeCallback callback){
    m_Connection->SubmitGCodeAsync("M24", MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::ReleaseMotorsAsync(GCodeCallback callback) {
    m_Connection->SubmitGCodeAsync("M84", MakeGCodeEngineCallback(this, std::move(callback)), 1);
}

void ShuiPrinter::CancelPrintAsync(GCodeCallback callback) {
    callback(GCodeResult::Unsupported);

    //XXX find a way to reliably send a gcode sequence
    PausePrintAsync(Printer::DefaultGCodeCallback);
    //XXX stop print
    m_Connection->SubmitGCodeAsync("G91", GCodeExecutionEngine::DefaultGCodeCallback, 1);
    m_Connection->SubmitGCodeAsync("G1 Z20", GCodeExecutionEngine::DefaultGCodeCallback, 1);
    m_Connection->SubmitGCodeAsync("M84", GCodeExecutionEngine::DefaultGCodeCallback, 1);
    ReleaseMotorsAsync(Printer::DefaultGCodeCallback);
    SetFanSpeedAsync(0, Printer::DefaultGCodeCallback);
}

PrinterStorage& ShuiPrinter::Storage(){
    return m_Storage;
}

const PrinterHistory& ShuiPrinter::History() const{
    return m_History;
}

bool ShuiPrinter::IsConnected()const {
    return m_State.has_value();
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

	if(timeout >= 4){
        OnConnectionLost();
	}
}

void ShuiPrinter::OnConnectFailed(std::int64_t times){
#if SHUI_VERBOSE_LOGGING
    LogShui(Display, "\tConnect Failed");
#endif
	if(times >= 6){
        OnConnectionLost();
	}
}

void ShuiPrinter::OnConnectionLost(){
    if(!m_State.has_value())
        return;

	m_State = std::nullopt;
		
	HandleStateChanged();

    m_Connection->CancelAllGCode();
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

    m_Connection->SubmitGCodeAsync("M220", [&](std::optional<std::string> result) {
        if(!result.has_value()){
#if SHUI_VERBOSE_LOGGING
            Println("\tM220 Failed");
#endif
            return;
        }

#if SHUI_VERBOSE_LOGGING
        Println("\tM220 C: %", result.value());
#endif
        UpdateStateFromFeedRate(result.value());
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

        if (TargetTemperaturesReached() && !print.CurrentBytesPrinted && print.Status != PrintStatus::Busy) {
            print.Status = PrintStatus::Busy;
            changed = true;
        }else if (AllHeatersOn() && !TargetTemperaturesReached() && print.Status != PrintStatus::Heating) {
            print.Status = PrintStatus::Heating;
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
        HandleStateChanged();
}

void ShuiPrinter::UpdateStateFromSdCardStatus(const std::string& line) {
    static const std::string NoSdPrinting = "Not SD printing";
    static const std::string SdPrintingPrefix = "SD printing byte ";

    if (line.find(NoSdPrinting) != std::string::npos && State().Print.has_value()) {
        State().Print = std::nullopt;

        HandleStateChanged();
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
    
    if(changed) HandleStateChanged();

    if (!State().Print.has_value()){
        State().Print = PrintState();
        changed = true;
    }

    auto& print = State().Print.value();
    
    errno = 0;
    std::int64_t current = std::atoi(current_string.c_str());
    std::int64_t target = std::atoi(target_string.c_str());
    if(errno){
        if(changed) HandleStateChanged();
        return;
    }
    
    if(print.CurrentBytesPrinted != current)
        changed = true;
    if(print.TargetBytesPrinted != target)
        changed = true;

    if (TargetTemperaturesReached() && print.Status != PrintStatus::Printing){
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

    if(changed) HandleStateChanged();
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

        HandleStateChanged();
        return;
    }

    if (State().Print.has_value() && printing && State().Print.value().Filename != filename && filename.size()) {
        State().Print.value().Filename = filename;

        HandleStateChanged();
        return;
    }

    if (State().Print.has_value() && !printing) {
        State().Print = std::nullopt;

        HandleStateChanged();
        return;
    }
}

void ShuiPrinter::UpdateStateFromFeedRate(const std::string& line){
    //reported as FR:150%
    static const std::string FeedratePrefix = "FR:";

    auto pos = line.find(FeedratePrefix);

    if(pos == std::string::npos)
        return;
    
    std::string feedrate_string = line.substr(pos + FeedratePrefix.size());

    pos = feedrate_string.find('%');

    if(pos == std::string::npos)
        return;

    errno = 0;
    std::int64_t feedrate = std::atoi(feedrate_string.c_str());
    if(errno)
        return;

    if (State().FeedRate != feedrate){
        State().FeedRate = feedrate;
        HandleStateChanged();
    }
}

std::optional<PrinterState> ShuiPrinter::GetPrinterState() const {
	return m_State;
}

bool ShuiPrinter::TargetTemperaturesReached() const {
    if(!m_State.has_value())
        return false;

    float bed_diviation = 6;
    float extruder_diviation = 10;

    return m_State.value().BedTemperature + bed_diviation > m_State.value().TargetBedTemperature
        && m_State.value().ExtruderTemperature + extruder_diviation > m_State.value().TargetExtruderTemperature;
}

bool ShuiPrinter::AllHeatersOn() const{
    if(!m_State.has_value())
        return false;

    return m_State.value().TargetBedTemperature && m_State.value().TargetExtruderTemperature;
}

PrinterState& ShuiPrinter::State() {
    if(!m_State.has_value()){
        m_State = PrinterState();
        HandleStateChanged();
    }
    return m_State.value();
}

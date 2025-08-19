import 'package:flutter/cupertino.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcnui;
import 'bloc.dart';
import 'device.dart';
import 'websocket_cubit.dart';
import 'websocket_state.dart';

class PrinterApp extends StatefulWidget {
  const PrinterApp({super.key});

  @override
  State<PrinterApp> createState() => _PrinterAppState();
}

class _PrinterAppState extends State<PrinterApp> {
  late final DeviceCubit deviceCubit;
  late final DeviceInfoCubit deviceInfoCubit;
  late final PrinterProxyCubit printerProxyCubit;

  @override
  void initState() {
    super.initState();
    deviceCubit = DeviceCubit();
    deviceInfoCubit = DeviceInfoCubit('ttb_1');
    printerProxyCubit = PrinterProxyCubit(deviceId: 'ttb_1');

    deviceInfoCubit.fetch();
    printerProxyCubit.connect();
  }

  @override
  void dispose() {
    printerProxyCubit.close();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MultiBlocProvider(
      providers: [BlocProvider(create: (_) => deviceCubit), BlocProvider(create: (_) => deviceInfoCubit), BlocProvider(create: (_) => printerProxyCubit)],
      child: BlocBuilder<DeviceCubit, String>(
        builder: (context, selectedDevice) {
          return BlocBuilder<DeviceInfoCubit, DeviceInfo?>(
            builder: (context, deviceInfo) {
              return BlocBuilder<PrinterProxyCubit, PrinterProxyState>(
                builder: (context, proxyState) {
                  String title = deviceInfo != null ? '${deviceInfo.manufacturer} ${deviceInfo.model}' : 'Loading...';

                  PrinterState? printerState = proxyState.printers[selectedDevice];

                  return CupertinoPageScaffold(
                    navigationBar: CupertinoNavigationBar(
                      middle: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Text(title, style: const TextStyle(fontSize: 16)),
                          Text(
                            proxyState.connected ? 'Connected' : 'Disconnected',
                            style: TextStyle(fontSize: 12, color: proxyState.connected ? CupertinoColors.systemGreen : CupertinoColors.systemRed),
                          ),
                        ],
                      ),
                    ),
                    child: SafeArea(child: printerState != null ? _buildPrinterState(context, printerState) : _buildNoData(context, proxyState)),
                  );
                },
              );
            },
          );
        },
      ),
    );
  }

  Widget _buildPrinterState(BuildContext context, PrinterState printerState) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          shadcnui.ShadcnUI(
            child: shadcnui.Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text('Temperatures', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                    const shadcnui.Gap(12),
                    Row(
                      children: [
                        Expanded(
                          child: Column(
                            children: [
                              const Text('Bed'),
                              Text('${printerState.bedTemperature.toStringAsFixed(1)}°C'),
                              Text('Target: ${printerState.targetBedTemperature.toStringAsFixed(1)}°C', style: const TextStyle(fontSize: 12)),
                            ],
                          ),
                        ),
                        Expanded(
                          child: Column(
                            children: [
                              const Text('Extruder'),
                              Text('${printerState.extruderTemperature.toStringAsFixed(1)}°C'),
                              Text('Target: ${printerState.targetExtruderTemperature.toStringAsFixed(1)}°C', style: const TextStyle(fontSize: 12)),
                            ],
                          ),
                        ),
                      ],
                    ),
                    const shadcnui.Gap(12),
                    Text('Feed Rate: ${printerState.feedRate.toStringAsFixed(1)}%'),
                  ],
                ),
              ),
            ),
          ),
          const shadcnui.Gap(16),
          if (printerState.print != null) _buildPrintState(printerState.print!),
        ],
      ),
    );
  }

  Widget _buildPrintState(PrintState printState) {
    return shadcnui.ShadcnUI(
      child: shadcnui.Card(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text('Print Job', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
              const shadcnui.Gap(12),
              Text('File: ${printState.filename}'),
              const shadcnui.Gap(8),
              Text('Progress: ${(printState.progress * 100).toStringAsFixed(1)}%'),
              const shadcnui.Gap(4),
              shadcnui.LinearProgressIndicator(value: printState.progress),
              const shadcnui.Gap(8),
              Text('Layer: ${printState.layer}'),
              Text('Height: ${printState.height.toStringAsFixed(2)}mm'),
              Text('Status: ${printState.status.name}'),
              const shadcnui.Gap(8),
              Text('Progress: ${printState.currentBytesPrinted} / ${printState.targetBytesPrinted} bytes'),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildNoData(BuildContext context, PrinterProxyState proxyState) {
    List<Widget> children = [];

    if (proxyState.error != null) {
      children.addAll([
        const Icon(CupertinoIcons.exclamationmark_triangle, size: 48, color: CupertinoColors.systemRed),
        const shadcnui.Gap(16),
        Text('Error: ${proxyState.error}', textAlign: TextAlign.center),
      ]);
    } else if (!proxyState.connected) {
      children.addAll([const CupertinoActivityIndicator(), const shadcnui.Gap(16), const Text('Connecting to printer...')]);
    } else {
      children.add(const Text('No printer data available'));
    }

    return Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: children));
  }
}

void main() {
  runApp(
    shadcnui.ShadcnApp(
      title: 'Printer Proxy',
      home: const PrinterApp(),
      theme: shadcnui.ThemeData(colorScheme: shadcnui.ColorSchemes.lightBlue(), radius: 0.0),
    ),
  );
}

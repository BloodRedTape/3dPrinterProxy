import 'package:flutter/cupertino.dart';
import 'package:flutter/foundation.dart';
import 'package:frontend_flutter/ui/control.dart';
import 'package:frontend_flutter/ui/history.dart';
import 'package:frontend_flutter/ui/print.dart';
import 'package:frontend_flutter/ui/upload.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';
import 'bloc.dart';
import 'device.dart';
import 'proxy.dart';
import 'states.dart';
import 'package:flutter_material_design_icons/flutter_material_design_icons.dart';
import 'dart:html' as html;

class PrinterPage extends BlocWidget<DeviceInfo> {
  final PrinterProxyCubit proxy;
  PrinterPage(Cubit<DeviceInfo> cubit, this.proxy) : super(cubit);

  @override
  Widget buildFromState(BuildContext context, DeviceInfo state) {
    final historyCubit = context.read<HistoryCubit>();

    final cubit = PrinterStateCubit(proxy, state.id);
    return Padding(
      padding: EdgeInsets.all(16),
      child: Column(
        children: [
          PrinterUploadCard(PrinterStorageUploadStateCubit(proxy, state.id)),
          const SizedBox(height: 20),
          PrinterControlCard(cubit, proxy),
          const SizedBox(height: 20),
          PrinterPrintCard(cubit),
          const SizedBox(height: 20),
          Expanded(child: PrinterHistoryCard(historyCubit)),
        ],
      ),
    );
  }
}

class PowerButton extends CubitWidget<PrinterPowerCubit, String?> {
  PowerButton(super.cubit);

  @override
  Widget buildFromState(BuildContext context, String? state) {
    bool? isOn = _parseState(state);

    if (isOn == null) return IconButton.ghost(icon: Icon(Icons.question_mark));

    final onPressed = () => getCubit().requestPower(!isOn);

    if (isOn) {
      return IconButton.primary(icon: Icon(Icons.power), onPressed: onPressed);
    } else {
      return IconButton.secondary(icon: Icon(Icons.power_off), onPressed: onPressed);
    }
  }

  bool? _parseState(String? state) {
    return state?.toLowerCase() == 'on'
        ? true
        : state?.toLowerCase() == 'off'
        ? false
        : null;
  }
}

class ProxyFrontend extends CubitWidget<DeviceInfoCubit, DeviceInfo?> {
  final PrinterProxyCubit proxyCubit;
  final PrinterStateCubit stateCubit;
  final DeviceInfoCubit deviceCubit;
  final HistoryCubit historyCubit;
  final PrinterPowerCubit powerCubit;

  ProxyFrontend(this.proxyCubit, this.stateCubit, this.deviceCubit, this.historyCubit, this.powerCubit, {super.key}) : super(deviceCubit) {
    getCubit().fetch();
  }

  @override
  Widget buildFromState(BuildContext context, DeviceInfo? info) {
    final refresh = IconButton.secondary(icon: const Icon(Icons.refresh), onPressed: () => getCubit().fetch());
    final power = PowerButton(powerCubit);

    return Scaffold(
      headers: [
        AppBar(
          backgroundColor: Theme.of(context).colorScheme.card,
          title: Row(children: [power, const SizedBox(width: 16), Text(info?.name() ?? 'Unknown Printer')]),
          trailing: [refresh],
        ),
      ],
      child: MultiBlocProvider(
        providers: [
          BlocProvider<PrinterStateCubit>(create: (_) => stateCubit),
          BlocProvider<DeviceInfoCubit>(create: (_) => deviceCubit),
          BlocProvider<HistoryCubit>(create: (_) => historyCubit),
        ],
        child: info != null ? PrinterPage(Cubit2(info), proxyCubit) : Center(child: Text('Proxy is out of reach')),
      ),
    );
  }
}

ColorScheme lightBlue() {
  return ColorScheme(
    brightness: Brightness.light,
    background: const HSLColor.fromAHSL(1, 0.0, 0.0, 0.92).toColor(),
    foreground: const HSLColor.fromAHSL(1, 222.2, 0.84, 0.05).toColor(),
    card: const HSLColor.fromAHSL(1, 0.0, 0.0, 1.0).toColor(),
    cardForeground: const HSLColor.fromAHSL(1, 222.2, 0.84, 0.05).toColor(),
    popover: const HSLColor.fromAHSL(1, 0.0, 0.0, 1.0).toColor(),
    popoverForeground: const HSLColor.fromAHSL(1, 222.2, 0.84, 0.05).toColor(),
    primary: const HSLColor.fromAHSL(1, 221.2, 0.83, 0.53).toColor(),
    primaryForeground: const HSLColor.fromAHSL(1, 210.0, 0.4, 0.98).toColor(),
    secondary: const HSLColor.fromAHSL(1, 210.0, 0.4, 0.96).toColor(),
    secondaryForeground: const HSLColor.fromAHSL(1, 222.2, 0.47, 0.11).toColor(),
    muted: const HSLColor.fromAHSL(1, 210.0, 0.4, 0.96).toColor(),
    mutedForeground: const HSLColor.fromAHSL(1, 215.4, 0.16, 0.47).toColor(),
    accent: const HSLColor.fromAHSL(1, 210.0, 0.4, 0.96).toColor(),
    accentForeground: const HSLColor.fromAHSL(1, 222.2, 0.47, 0.11).toColor(),
    destructive: const HSLColor.fromAHSL(1, 0.0, 0.84, 0.6).toColor(),
    destructiveForeground: const HSLColor.fromAHSL(1, 210.0, 0.4, 0.98).toColor(),
    border: const HSLColor.fromAHSL(1, 214.3, 0.32, 0.91).toColor(),
    input: const HSLColor.fromAHSL(1, 214.3, 0.32, 0.91).toColor(),
    ring: const HSLColor.fromAHSL(1, 221.2, 0.83, 0.53).toColor(),
    chart1: const HSLColor.fromAHSL(1, 12.0, 0.76, 0.61).toColor(),
    chart2: const HSLColor.fromAHSL(1, 173.0, 0.58, 0.39).toColor(),
    chart3: const HSLColor.fromAHSL(1, 197.0, 0.37, 0.24).toColor(),
    chart4: const HSLColor.fromAHSL(1, 43.0, 0.74, 0.66).toColor(),
    chart5: const HSLColor.fromAHSL(1, 27.0, 0.87, 0.67).toColor(),
  );
}

void main() {
  final proxyCubit = PrinterProxyCubit()..connect();
  final deviceCubit = DeviceInfoCubit('ttb_1');
  final printerCubit = PrinterStateCubit(proxyCubit, deviceCubit.device);
  final historyCubit = HistoryCubit()..fetch();
  final powerCubit = PrinterPowerCubit();

  powerCubit.connect();

  printerCubit.stream.listen(historyCubit.onStateChanged);

  html.document.onContextMenu.listen((event) {
    event.stopPropagation(); // let the browser handle it
  });

  runApp(
    ShadcnApp(
      title: 'Printer Proxy',
      home: ProxyFrontend(proxyCubit, printerCubit, deviceCubit, historyCubit, powerCubit),
      theme: ThemeData(colorScheme: lightBlue(), radius: 0.0),
    ),
  );
}

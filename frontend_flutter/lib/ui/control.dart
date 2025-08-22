import 'dart:ui';

import 'package:flutter/cupertino.dart';
import 'package:flutter_material_design_icons/flutter_material_design_icons.dart';
import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/device.dart';
import 'package:frontend_flutter/ui/common.dart';
import 'package:frontend_flutter/proxy.dart';
import 'package:frontend_flutter/states.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';

class PersistentTextField extends BlocWidget<double> {
  final double width;
  PersistentTextField(super.cubit, {this.width = 80});

  @override
  Widget buildFromState(BuildContext context, double state) {
    return SizedBox(
      width: width,
      child: TextField(
        initialValue: state.toString(),
        onChanged: (value) {
          emit(double.tryParse(value) ?? 0);
        },
        features: const [InputFeature.spinner()],
        submitFormatters: [TextInputFormatters.mathExpression()],
      ),
    );
  }
}

class TempPair extends StatelessWidget {
  IconData icon;
  double? current;
  double target;
  void Function(double) setTarget;
  double max;
  final cubit = Cubit2<double>(0);

  TempPair(this.icon, this.current, this.target, this.setTarget, this.max);

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(icon),
        const SizedBox(width: 8),
        SizedBox(width: 70, child: Text(current != null ? '$current/$target' : '$target')),
        const SizedBox(width: 8),
        PersistentTextField(cubit),
        const SizedBox(width: 8),
        Button.secondary(
          child: Text('set'),
          onPressed: () {
            setTarget(clampDouble(cubit.state, 0, max));
          },
        ),
      ],
    );
  }
}

class Preset {
  String name;
  double bed;
  double extruder;

  Preset(this.name, this.bed, this.extruder);
}

class ConnectedPrinterCardContent extends CubitWidget<PrinterStateCubit, PrinterState?> {
  final PrinterProxyCubit proxy;

  const ConnectedPrinterCardContent(super.cubit, this.proxy, {super.key});

  @override
  Widget buildFromState(BuildContext context, PrinterState? newState) {
    PrinterState state = newState!;

    final DeviceInfoCubit device = context.read<DeviceInfoCubit>();

    void setTragetBedTemperature(double temp) {
      proxy.setTargetBedTemperature(device.device, temp);
    }

    void setTargetExtruderTemperature(double temp) {
      proxy.setTargetExtruderTemperature(device.device, temp);
    }

    void setFeedRate(double rate) {
      proxy.setFeedRate(device.device, rate);
    }

    List<Preset> presets = [Preset('Pla', 55, 210), Preset('Petg', 65, 230), Preset('Cool', 0, 0)];

    List<Widget> presetButtons =
        presets
            .map(
              (p) => Button.secondary(
                child: Text(p.name),
                onPressed: () {
                  setTargetExtruderTemperature(p.extruder);
                  setTragetBedTemperature(p.bed);
                },
              ),
            )
            .toList();

    return Row(
      children: [
        Padding(
          padding: EdgeInsets.all(20),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.start,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text("Wheels"),
              const SizedBox(height: 8),
              TempPair(MdiIcons.printer3dNozzle, state.extruderTemperature, state.targetExtruderTemperature, setTargetExtruderTemperature, 240),
              const SizedBox(height: 8),
              TempPair(MdiIcons.printer3d, state.bedTemperature, state.targetBedTemperature, setTragetBedTemperature, 70),
              const SizedBox(height: 8),
              TempPair(MdiIcons.speedometer, null, state.feedRate, setFeedRate, 70),
              const SizedBox(height: 8),
              Text("Presets"),
              const SizedBox(height: 8),
              Row(spacing: 16, children: presetButtons),
            ],
          ),
        ),
      ],
    );
  }
}

class PrinterControlCard extends CubitWidget<PrinterStateCubit, PrinterState?> {
  final PrinterProxyCubit proxy;
  PrinterControlCard(super.cubit, this.proxy);

  @override
  Widget buildFromState(BuildContext context, PrinterState? state) {
    return PrinterCard(title: 'Control', child: state != null ? ConnectedPrinterCardContent(getCubit(), proxy) : MessageCardContent('Printer is Off'));
  }
}

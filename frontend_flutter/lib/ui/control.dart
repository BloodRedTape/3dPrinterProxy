import 'package:flutter/cupertino.dart';
import 'package:flutter_material_design_icons/flutter_material_design_icons.dart';
import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/common.dart';
import 'package:frontend_flutter/proxy.dart';
import 'package:frontend_flutter/states.dart';

class TempPair extends StatelessWidget {
  IconData icon;
  double current;
  double target;
  void Function(double) setTarget;
  double max;

  TempPair(this.icon, this.current, this.target, this.setTarget, this.max);

  @override
  Widget build(BuildContext context) {
    return Row(children: [Icon(icon), const SizedBox(width: 8), Text('${current}/${target}')]);
  }
}

class ConnectedPrinterCardContent extends CubitWidget<PrinterStateCubit, PrinterState?> {
  const ConnectedPrinterCardContent(super.cubit, {super.key});

  @override
  Widget buildFromState(BuildContext context, PrinterState? newState) {
    PrinterState state = newState!;

    return Row(
      children: [
        Padding(
          padding: EdgeInsets.all(20),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.start,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              TempPair(MdiIcons.printer3dNozzle, state.extruderTemperature, state.targetExtruderTemperature, (_) {}, 240),
              TempPair(MdiIcons.printer3d, state.bedTemperature, state.targetBedTemperature, (_) {}, 70),
            ],
          ),
        ),
      ],
    );
  }
}

class PrinterControlCard extends CubitWidget<PrinterStateCubit, PrinterState?> {
  PrinterControlCard(super.cubit);

  @override
  Widget buildFromState(BuildContext context, PrinterState? state) {
    return PrinterCard(title: 'Control', child: state != null ? ConnectedPrinterCardContent(getCubit()) : MessageCardContent('Printer Disconnected'));
  }
}

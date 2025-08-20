import 'package:flutter/cupertino.dart';
import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/common.dart';
import 'package:frontend_flutter/control.dart';
import 'package:frontend_flutter/proxy.dart';
import 'package:frontend_flutter/states.dart';

class PrinterPrintCard extends CubitWidget<PrinterStateCubit, PrinterState?> {
  PrinterPrintCard(super.cubit);

  @override
  Widget buildFromState(BuildContext context, PrinterState? state) {
    return PrinterCard(title: 'Print', child: state != null && state.print != null ? ConnectedPrinterCardContent(getCubit()) : MessageCardContent('Idle'));
  }
}

import 'package:flutter/cupertino.dart';
import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/ui/common.dart';
import 'package:frontend_flutter/ui/control.dart';
import 'package:frontend_flutter/proxy.dart';
import 'package:frontend_flutter/states.dart';

class PrinterUploadCard extends CubitWidget<PrinterStateCubit, PrinterState?> {
  PrinterUploadCard(super.cubit);

  @override
  Widget buildFromState(BuildContext context, PrinterState? state) {
    return PrinterCard(
      title: 'Upload',
      child: state != null && state.print != null ? ConnectedPrinterCardContent(getCubit()) : MessageCardContent('Network is quiet'),
    );
  }
}

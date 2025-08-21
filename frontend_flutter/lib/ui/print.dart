import 'package:flutter/cupertino.dart';
import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/device.dart';
import 'package:frontend_flutter/ui/common.dart';
import 'package:frontend_flutter/ui/control.dart';
import 'package:frontend_flutter/proxy.dart';
import 'package:frontend_flutter/states.dart';
import 'package:frontend_flutter/ui/preview.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';

class PrintCardContent extends StatelessWidget {
  final PrintState state;

  const PrintCardContent({super.key, required this.state});

  @override
  Widget build(BuildContext context) {
    final double imageSize = 150;
    final double progressWidth = MediaQuery.of(context).size.width * 0.8 - imageSize;
    return Padding(
      padding: EdgeInsets.all(16),
      child: Row(
        children: [
          Preview(imageSize, state.filename.isNotEmpty ? state.filename : 'Unknown'),
          SizedBox(width: 16),
          Expanded(
            child: SizedBox(
              height: imageSize,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [Text(state.filename).semiBold, const SizedBox(height: 16), Text(state.status.name)],
                  ),
                  Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      SizedBox(
                        width: progressWidth,
                        child: Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [Text('${state.progress}%'), Text('Layer ${state.layer}')]),
                      ),
                      const SizedBox(height: 16),
                      SizedBox(width: progressWidth, child: Progress(progress: state.progress, min: 0.0, max: 100.0)),
                    ],
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class PrinterPrintCard extends CubitWidget<PrinterStateCubit, PrinterState?> {
  PrinterPrintCard(super.cubit);

  @override
  Widget buildFromState(BuildContext context, PrinterState? state) {
    return PrinterCard(title: 'Print', child: state != null && state.print != null ? PrintCardContent(state: state.print!) : MessageCardContent('Idle'));
  }
}

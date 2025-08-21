import 'package:flutter/cupertino.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';
import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/ui/common.dart';
import 'package:frontend_flutter/proxy.dart';
import 'package:frontend_flutter/states.dart';

class UploadCardContent extends StatelessWidget {
  final PrinterStorageUploadState state;

  const UploadCardContent({super.key, required this.state});

  @override
  Widget build(BuildContext context) {
    final double padding = 16;
    final double progressWidth = MediaQuery.of(context).size.width - padding * 2;
    final int percent = state.target > 0 ? (state.current / state.target * 100).toInt() : 0;
    return Padding(
      padding: EdgeInsets.all(padding),
      child: Row(
        children: [
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    SizedBox(
                      width: progressWidth,
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [Text('${state.current}/${state.target} bytes'), Text('$percent%')],
                      ),
                    ),
                    const SizedBox(height: 16),
                    SizedBox(width: progressWidth, child: Progress(progress: percent.toDouble(), min: 0.0, max: 100.0)),
                  ],
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildToast(BuildContext context, ToastOverlay overlay) {
    return SurfaceCard(
      child: Basic(
        title: Text('${state.filename} ${state.status}'),
        trailing: PrimaryButton(
          size: ButtonSize.small,
          onPressed: () {
            overlay.close();
          },
          child: const Text('Ok'),
        ),
        trailingAlignment: Alignment.center,
      ),
    );
  }
}

class PrinterUploadCard extends CubitWidget<PrinterStorageUploadStateCubit, PrinterStorageUploadState?> {
  PrinterUploadCard(super.cubit);

  @override
  Widget buildFromState(BuildContext context, PrinterStorageUploadState? state) {
    return PrinterCard(
      title: state != null ? 'Upload - ${state.filename}' : 'Upload',
      child: state != null ? UploadCardContent(state: state) : MessageCardContent('Network is quiet'),
    );
  }
}

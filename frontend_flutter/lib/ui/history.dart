import 'dart:convert';

import 'package:frontend_flutter/ui/preview.dart';
import 'package:http/http.dart' as http;
import 'package:flutter/cupertino.dart';
import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/device.dart';
import 'package:frontend_flutter/ui/common.dart';
import 'package:frontend_flutter/ui/control.dart';
import 'package:frontend_flutter/proxy.dart';
import 'package:frontend_flutter/states.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';

class HistoryCubit extends Cubit<List<HistoryEntry>> {
  HistoryCubit() : super([]);

  Future<void> fetch() async {
    final String device = 'ttb_1';
    final String url = '${getOrigin()}/api/v1/printers/$device/history';

    try {
      final response = await http.get(Uri.parse(url));

      if (response.statusCode == 200) {
        final List<dynamic> data = jsonDecode(response.body);
        final entries = data.map((e) => HistoryEntry.fromJson(e)).toList();
        emit(entries);
      } else {
        emit([]);
      }
    } catch (e) {
      emit([]);
    }
  }
}

class PrinterHistoryEntry extends StatelessWidget {
  final HistoryEntry entry;

  const PrinterHistoryEntry({super.key, required this.entry});

  @override
  Widget build(BuildContext context) {
    final double previewSize = 100;

    return Padding(
      padding: EdgeInsets.all(16),
      child: Row(
        children: [
          Preview(previewSize, entry.contentHash),
          const SizedBox(width: 16),
          SizedBox(
            height: previewSize * 0.8,
            child: Column(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [Text(entry.filename).semiBold, Text(entry.getPrettyDuration())],
            ),
          ),
        ],
      ),
    );
  }
}

class PrinterHistoryCard extends CubitWidget<HistoryCubit, List<HistoryEntry>> {
  PrinterHistoryCard() : super(HistoryCubit()..fetch());

  @override
  Widget buildFromState(BuildContext context, List<HistoryEntry> state) {
    return PrinterCard(
      title: 'History',
      child: state.length == 0 ? MessageCardContent('Empty') : Column(children: state.map((e) => PrinterHistoryEntry(entry: e)).toList()),
    );
  }
}

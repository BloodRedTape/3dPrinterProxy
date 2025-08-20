import 'dart:convert';
import 'dart:io';
import 'package:duration/duration.dart';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:flutter/foundation.dart';
import 'bloc.dart';
import 'device.dart';

enum MessageType {
  init('init'),
  state('state');

  const MessageType(this.name);
  final String name;

  static MessageType fromString(String type) {
    switch (type) {
      case 'init':
        return MessageType.init;
      case 'state':
        return MessageType.state;
      default:
        return MessageType.state;
    }
  }
}

enum PrintStatus {
  heating('Heating'),
  busy('Busy'),
  printing('Printing');

  const PrintStatus(this.name);
  final String name;

  static PrintStatus fromString(String status) {
    switch (status.toLowerCase()) {
      case 'heating':
        return PrintStatus.heating;
      case 'busy':
        return PrintStatus.busy;
      case 'printing':
        return PrintStatus.printing;
      default:
        return PrintStatus.busy;
    }
  }
}

class PrintState {
  final String filename;
  final double progress;
  final int currentBytesPrinted;
  final int targetBytesPrinted;
  final int layer;
  final double height;
  final PrintStatus status;

  PrintState({
    required this.filename,
    required this.progress,
    required this.currentBytesPrinted,
    required this.targetBytesPrinted,
    required this.layer,
    required this.height,
    required this.status,
  });

  factory PrintState.fromJson(Map<String, dynamic> json) {
    return PrintState(
      filename: json['filename'] ?? '',
      progress: (json['progress'] ?? 0.0).toDouble(),
      currentBytesPrinted: json['bytes_current'] ?? 0,
      targetBytesPrinted: json['bytes_target'] ?? 0,
      layer: json['layer'] ?? 0,
      height: (json['height'] ?? 0.0).toDouble(),
      status: PrintStatus.fromString(json['status'] ?? 'busy'),
    );
  }
}

class PrinterState {
  final double bedTemperature;
  final double targetBedTemperature;
  final double extruderTemperature;
  final double targetExtruderTemperature;
  final double feedRate;
  final PrintState? print;

  PrinterState({
    required this.bedTemperature,
    required this.targetBedTemperature,
    required this.extruderTemperature,
    required this.targetExtruderTemperature,
    required this.feedRate,
    this.print,
  });

  factory PrinterState.fromJson(Map<String, dynamic> json) {
    return PrinterState(
      bedTemperature: (json['bed'] ?? 0.0).toDouble(),
      targetBedTemperature: (json['target_bed'] ?? 0.0).toDouble(),
      extruderTemperature: (json['extruder'] ?? 0.0).toDouble(),
      targetExtruderTemperature: (json['target_extruder'] ?? 0.0).toDouble(),
      feedRate: (json['feedrate'] ?? 0.0).toDouble(),
      print: json['print'] != null ? PrintState.fromJson(json['print']) : null,
    );
  }
}

class PrinterProxyMessage {
  final MessageType type;
  final String id;
  final Map<String, dynamic>? content;

  PrinterProxyMessage({required this.type, required this.id, this.content});

  factory PrinterProxyMessage.fromJson(Map<String, dynamic> json) {
    return PrinterProxyMessage(type: MessageType.fromString(json['type'] ?? 'state'), id: json['id'] ?? '', content: json['content']);
  }

  Map<String, dynamic> toJson() {
    return {'type': type.name, 'id': id, 'content': content};
  }
}

class PrinterProxyState {
  final bool connected;
  final Map<String, PrinterState> printers;
  final String? error;

  PrinterProxyState({this.connected = false, this.printers = const {}, this.error});

  PrinterProxyState copyWith({bool? connected, Map<String, PrinterState>? printers, String? error}) {
    return PrinterProxyState(connected: connected ?? this.connected, printers: printers ?? this.printers, error: error);
  }
}

class HistoryEntry {
  final String filename;
  final String contentHash;
  final int printStart;
  final int printEnd;

  HistoryEntry({required this.filename, required this.contentHash, required this.printStart, required this.printEnd});

  factory HistoryEntry.fromJson(Map<String, dynamic> json) {
    return HistoryEntry(
      filename: json['Filename'] as String? ?? '',
      contentHash: json['ContentHash']?.toString() ?? '0',
      printStart: _parseInt(json['PrintStart']),
      printEnd: _parseInt(json['PrintEnd']),
    );
  }

  static int _parseInt(dynamic v) {
    if (v is int) return v;
    return int.tryParse(v.toString()) ?? 0;
  }

  String get formattedDuration => Duration(seconds: (printEnd - printStart).clamp(0, double.infinity).toInt()).toString().split('.').first;

  String getPrettyDuration() {
    final seconds = (printEnd - printStart).clamp(0, double.infinity).toInt();
    return prettyDuration(
      Duration(seconds: seconds),
      abbreviated: true, // h / m / s
    );
  }
}

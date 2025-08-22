import 'dart:convert';
import 'dart:io';
import 'package:duration/duration.dart';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:flutter/foundation.dart';
import 'bloc.dart';
import 'device.dart';

enum MessageType {
  init('init'),
  state('state'),
  upload('upload'),
  set('set');

  const MessageType(this.name);
  final String name;

  static MessageType fromString(String type) {
    switch (type) {
      case 'init':
        return MessageType.init;
      case 'state':
        return MessageType.state;
      case 'upload':
        return MessageType.upload;
      case 'set':
        return MessageType.set;
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

enum PrintFinishReason {
  unknown('Unknown'),
  complete('Complete'),
  interrupted('Interrupted');

  const PrintFinishReason(this.name);
  final String name;

  static PrintFinishReason fromString(String value) {
    switch (value.toLowerCase()) {
      case 'unknown':
        return PrintFinishReason.unknown;
      case 'complete':
        return PrintFinishReason.complete;
      case 'interrupted':
        return PrintFinishReason.interrupted;
      default:
        return PrintFinishReason.unknown;
    }
  }

  @override
  String toString() => name;

  String toJson() => name;

  static PrintFinishReason fromJson(String value) => fromString(value);
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
      progress: (json['progress'] ?? json['Progress'] ?? 0.0).toDouble(),
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

class PrintFinishState {
  double progress;
  int bytes;
  int layer;
  double height;
  PrintFinishReason reason;

  PrintFinishState({
    this.progress = 0.0,
    this.bytes = 0,
    this.layer = 0,
    this.height = 0.0,
    this.reason = PrintFinishReason.complete, // default value like C++
  });

  // Deserialize from JSON
  factory PrintFinishState.fromJson(Map<String, dynamic> json) {
    return PrintFinishState(
      progress: (json['Progress'] ?? 0.0).toDouble(),
      bytes: json['Bytes'] ?? 0,
      layer: json['Layer'] ?? 0,
      height: (json['Height'] ?? 0.0).toDouble(),
      reason: json['Reason'] != null ? PrintFinishReason.fromJson(json['Reason']) : PrintFinishReason.complete,
    );
  }
}

class HistoryEntry {
  final String filename;
  final String fileId;
  final int printStart;
  final int printEnd;
  final PrintFinishState finishState;

  HistoryEntry({required this.filename, required this.fileId, required this.printStart, required this.printEnd, required this.finishState});

  factory HistoryEntry.fromJson(Map<String, dynamic> json) {
    return HistoryEntry(
      filename: json['Filename'] as String? ?? '',
      fileId: json['FileId'] as String? ?? '',
      printStart: _parseInt(json['PrintStart']),
      printEnd: _parseInt(json['PrintEnd']),
      finishState: PrintFinishState.fromJson(json['FinishState']),
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

enum PrinterStorageUploadStatus {
  sending('Sending'),
  success('Success'),
  failure('Failure');

  const PrinterStorageUploadStatus(this.name);
  final String name;

  static PrinterStorageUploadStatus fromString(String value) {
    switch (value.toLowerCase()) {
      case 'sending':
        return PrinterStorageUploadStatus.sending;
      case 'success':
        return PrinterStorageUploadStatus.success;
      case 'failure':
        return PrinterStorageUploadStatus.failure;
      default:
        return PrinterStorageUploadStatus.sending; // default like C++
    }
  }

  @override
  String toString() => name;

  String toJson() => name;

  static PrinterStorageUploadStatus fromJson(String value) => fromString(value);
}

class PrinterStorageUploadState {
  int current;
  int target;
  String filename;
  PrinterStorageUploadStatus status;

  PrinterStorageUploadState({
    required this.filename,
    this.current = 0,
    this.target = 0,
    this.status = PrinterStorageUploadStatus.sending, // default
  });

  // Factory to construct from JSON
  factory PrinterStorageUploadState.fromJson(Map<String, dynamic> json) {
    return PrinterStorageUploadState(
      filename: json['filename'] ?? '',
      current: json['current'] ?? 0,
      target: json['target'] ?? 0,
      status: json['status'] != null ? PrinterStorageUploadStatus.fromJson(json['status']) : PrinterStorageUploadStatus.sending,
    );
  }
}

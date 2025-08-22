import 'dart:async';
import 'dart:convert';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'bloc.dart';
import 'device.dart';
import 'states.dart';

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

typedef PrinterStateChangedCallback = void Function(PrinterState?);
typedef PrinterStorageUploadStateChangedCallback = void Function(PrinterStorageUploadState?);

class PrinterProxyCubit {
  WebSocketChannel? _channel;
  StreamSubscription? _subscription;
  Timer? _reconnectTimer;
  bool _disposed = false;

  Map<String, PrinterStateChangedCallback> onStateChanged = {};
  Map<String, PrinterStorageUploadStateChangedCallback> onUploadChanged = {};

  void connect() {
    if (_disposed) return;

    try {
      final origin = getOrigin();

      String wsUrl = "";
      if (origin.startsWith('https')) {
        wsUrl = '${origin.replaceFirst('https', 'ws')}/api/v1/ws';
      }
      if (origin.startsWith('http')) {
        wsUrl = '${origin.replaceFirst('http', 'ws')}/api/v1/ws';
      }

      final Uri uri = Uri.parse(wsUrl);

      print(uri);

      _channel = WebSocketChannel.connect(uri);

      _subscription = _channel!.stream.listen(_onMessage, onError: _onError, onDone: _onDisconnected);
    } catch (e) {
      _reset();
      _scheduleReconnect();
    }
  }

  void _reset() {
    for (var callback in onStateChanged.values) {
      callback(null);
    }

    for (var callback in onUploadChanged.values) {
      callback(null);
    }
  }

  void reconnect() {
    disconnect();
    connect();
  }

  void disconnect() {
    _cancelReconnectTimer();
    _subscription?.cancel();
    _channel?.sink.close();
    _channel = null;
    _subscription = null;

    if (!_disposed) {
      _reset();
    }
  }

  void sendMessage(PrinterProxyMessage message) {
    if (_channel != null) {
      try {
        final jsonString = jsonEncode(message.toJson());
        _channel!.sink.add(jsonString);
      } catch (e) {}
    }
  }

  void setTargetBedTemperature(String deviceId, double temperature) {
    final message = PrinterProxyMessage(type: MessageType.set, id: deviceId, content: {'property': 'target_bed', 'value': temperature});
    sendMessage(message);
  }

  void setTargetExtruderTemperature(String deviceId, double temperature) {
    final message = PrinterProxyMessage(type: MessageType.set, id: deviceId, content: {'property': 'target_extruder', 'value': temperature});
    sendMessage(message);
  }

  void setFeedRate(String deviceId, double feedRate) {
    final message = PrinterProxyMessage(type: MessageType.set, id: deviceId, content: {'property': 'feedrate', 'value': feedRate});
    sendMessage(message);
  }

  void _onMessage(dynamic data) {
    if (_disposed) return;

    try {
      final jsonData = jsonDecode(data as String);
      final message = PrinterProxyMessage.fromJson(jsonData);

      switch (message.type) {
        case MessageType.init:
          break;
        case MessageType.set:
          break;
        case MessageType.state:
          try {
            onStateChanged[message.id]?.call(PrinterState.fromJson(message.content!));
          } catch (e) {
            onStateChanged[message.id]?.call(null);
          }
          break;
        case MessageType.upload:
          try {
            final upload = PrinterStorageUploadState.fromJson(message.content!);
            onUploadChanged[message.id]?.call(upload);
          } catch (e) {
            onUploadChanged[message.id]?.call(null);
          }
          break;
      }
    } catch (e) {}
  }

  void _onError(error) {
    if (_disposed) return;

    _reset();
    _scheduleReconnect();
  }

  void _onDisconnected() {
    if (_disposed) return;

    _reset();
    _scheduleReconnect();
  }

  void _scheduleReconnect() {
    if (_disposed) return;

    _cancelReconnectTimer();
    _reconnectTimer = Timer(const Duration(seconds: 5), () {
      if (!_disposed) {
        connect();
      }
    });
  }

  void _cancelReconnectTimer() {
    _reconnectTimer?.cancel();
    _reconnectTimer = null;
  }
}

class PrinterStateCubit extends Cubit<PrinterState?> {
  PrinterProxyCubit proxy;
  String deviceId;

  PrinterStateCubit(this.proxy, this.deviceId) : super(null) {
    proxy.onStateChanged[deviceId] = onNewState;
  }

  void onNewState(PrinterState? state) {
    emit(state);
  }
}

class PrinterStorageUploadStateCubit extends Cubit<PrinterStorageUploadState?> {
  PrinterProxyCubit proxy;
  String deviceId;

  PrinterStorageUploadStateCubit(this.proxy, this.deviceId) : super(null) {
    proxy.onUploadChanged[deviceId] = onNewState;
  }

  void onNewState(PrinterStorageUploadState? state) {
    emit(state);
  }
}

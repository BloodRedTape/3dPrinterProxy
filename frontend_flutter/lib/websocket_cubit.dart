import 'dart:async';
import 'dart:convert';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:flutter/foundation.dart';
import 'bloc.dart';
import 'device.dart';
import 'websocket_state.dart';

class PrinterProxyCubit extends Cubit<PrinterProxyState> {
  WebSocketChannel? _channel;
  StreamSubscription? _subscription;
  Timer? _reconnectTimer;
  final String deviceId;
  bool _disposed = false;

  PrinterProxyCubit({required this.deviceId}) : super(PrinterProxyState());

  void connect() {
    if (_disposed) return;

    try {
      final origin = getOrigin();
      final wsUrl = origin.replaceFirst('http', 'ws') + '/api/v1/ws';
      
      _channel = WebSocketChannel.connect(Uri.parse(wsUrl));
      
      _subscription = _channel!.stream.listen(
        _onMessage,
        onError: _onError,
        onDone: _onDisconnected,
      );
      
      emit(state.copyWith(connected: true, error: null));
    } catch (e) {
      emit(state.copyWith(connected: false, error: e.toString()));
      _scheduleReconnect();
    }
  }

  void disconnect() {
    _cancelReconnectTimer();
    _subscription?.cancel();
    _channel?.sink.close();
    _channel = null;
    _subscription = null;
    
    if (!_disposed) {
      emit(state.copyWith(connected: false));
    }
  }

  void sendMessage(PrinterProxyMessage message) {
    if (_channel != null && state.connected) {
      try {
        final jsonString = jsonEncode(message.toJson());
        _channel!.sink.add(jsonString);
      } catch (e) {
        emit(state.copyWith(error: 'Failed to send message: $e'));
      }
    }
  }

  void setTargetBedTemperature(double temperature) {
    final message = PrinterProxyMessage(
      type: MessageType.state,
      id: deviceId,
      content: {
        'property': 'target_bed',
        'value': temperature,
      },
    );
    sendMessage(message);
  }

  void setTargetExtruderTemperature(double temperature) {
    final message = PrinterProxyMessage(
      type: MessageType.state,
      id: deviceId,
      content: {
        'property': 'target_extruder',
        'value': temperature,
      },
    );
    sendMessage(message);
  }

  void setFeedRate(double feedRate) {
    final message = PrinterProxyMessage(
      type: MessageType.state,
      id: deviceId,
      content: {
        'property': 'feedrate',
        'value': feedRate,
      },
    );
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
        case MessageType.state:
          if (message.content != null) {
            final printerState = PrinterState.fromJson(message.content!);
            final updatedPrinters = Map<String, PrinterState>.from(state.printers);
            updatedPrinters[message.id] = printerState;
            
            emit(state.copyWith(
              printers: updatedPrinters,
              error: null,
            ));
          }
          break;
      }
    } catch (e) {
      emit(state.copyWith(error: 'Failed to parse message: $e'));
    }
  }

  void _onError(error) {
    if (_disposed) return;
    
    emit(state.copyWith(connected: false, error: error.toString()));
    _scheduleReconnect();
  }

  void _onDisconnected() {
    if (_disposed) return;
    
    emit(state.copyWith(connected: false));
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

  @override
  Future<void> close() async {
    _disposed = true;
    disconnect();
    super.close();
  }
}
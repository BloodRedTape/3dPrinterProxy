import 'package:frontend_flutter/bloc.dart';
import 'package:frontend_flutter/config.dart';
import 'package:home_assistant_ws/home_assistant_ws.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:flutter/foundation.dart';

import 'package:web/web.dart' as web;

String getOrigin() {
  if (kReleaseMode)
    return web.window.location.origin;
  else
    //return 'http://127.0.0.1:2228';
    return 'http://192.168.1.209:2228';
}

class DeviceCubit extends Cubit<String> {
  DeviceCubit() : super('ttb_1');
}

class DeviceInfo {
  final String id;
  final String manufacturer;
  final String model;

  DeviceInfo(this.id, this.manufacturer, this.model);

  factory DeviceInfo.fromJson(String id, Map<String, dynamic> json) {
    return DeviceInfo(id, json['manufacturer'], json['model']);
  }

  String name() {
    return '$manufacturer $model';
  }
}

class DeviceInfoCubit extends Cubit<DeviceInfo?> {
  final String device;
  DeviceInfoCubit(this.device) : super(null);

  Future<void> fetch() async {
    try {
      final response = await http.get(Uri.parse('${getOrigin()}/api/v1/printers/$device'));

      final data = json.decode(response.body);

      emit(DeviceInfo.fromJson(device, data));
    } catch (e) {
      emit(null);
    }
  }
}

class PrinterPowerCubit extends Cubit<String?> {
  final HomeAssistantWs homeAssistantWs = HomeAssistantWs(token: haToken, baseUrl: haUrl);
  final String printerPowerId = 'switch.power_print3d_master';

  PrinterPowerCubit() : super(null);

  Future<void> connect() async {
    if (!await homeAssistantWs.isConnected()) {
      await homeAssistantWs.connect();
      homeAssistantWs.subscribeEntities(onEventMessage);
    }
  }

  void requestPower(bool power) async {
    await connect();
    await homeAssistantWs.executeServiceForEntity(printerPowerId, power ? 'turn_on' : 'turn_off');
  }

  void onEventMessage(EventMessage message) {
    if (message.available != null) {
      for (final Entity entity in message.available!.entities.where((e) => e.entityId == printerPowerId).toList()) {
        emit(entity.state);
      }
    }
    if (message.change != null) {
      for (final EntityChange entity in message.change!.changes.where((c) => c.entityId == printerPowerId).toList()) {
        if (entity.stateChange != null) {
          emit(entity.stateChange!.newValue);
        }
      }
    }
  }
}

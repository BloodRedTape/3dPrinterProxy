import 'package:frontend_flutter/bloc.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:flutter/foundation.dart';

import 'package:web/web.dart' as web;

String getOrigin() {
  if (kReleaseMode)
    return web.window.location.origin;
  else
    return 'http://localhost:2228';
}

class DeviceCubit extends Cubit<String> {
  DeviceCubit() : super('ttb_1');
}

class DeviceInfo {
  final String manufacturer;
  final String model;

  DeviceInfo(this.manufacturer, this.model);

  factory DeviceInfo.fromJson(Map<String, dynamic> json) {
    return DeviceInfo(json['manufacturer'], json['model']);
  }
}

class DeviceInfoCubit extends Cubit<DeviceInfo?> {
  final String device;
  DeviceInfoCubit(this.device) : super(null);

  Future<void> fetch() async {
    try {
      final response = await http.get(Uri.parse('${getOrigin()}/api/v1/printers/$device'));

      final data = json.decode(response.body);

      emit(DeviceInfo.fromJson(data));
    } catch (e) {
      emit(null);
    }
  }
}

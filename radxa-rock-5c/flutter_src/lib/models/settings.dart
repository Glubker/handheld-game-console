import 'dart:async';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

class Settings extends ChangeNotifier {
  static final Settings _instance = Settings._internal();
  factory Settings() => _instance;

  Settings._internal() {
    loadSettings().then((_) {
      _startMonitoring();
    });
  }

  double _brightness = 0.0;
  double _volume = 0.0;
  bool _isScreenBlank = false;
  Timer? _monitorTimer;

  // Getters
  double get brightness => _brightness;
  double get volume => _volume;
  bool get isScreenBlank => _isScreenBlank;

  // Setter for screen blank state
  set isScreenBlank(bool value) {
    _isScreenBlank = value;
    notifyListeners();
  }

  Future<void> loadSettings() async {
    try {
      final prefs = await SharedPreferences.getInstance();

      // Always read current system values first
      _brightness = await _getSystemBrightness();
      _volume = await _getSystemVolume();

      // Then apply stored values if they exist
      final storedBrightness = prefs.getDouble('brightness');
      if (storedBrightness != null) {
        await setBrightness(storedBrightness);
      }

      final storedVolume = prefs.getDouble('volume');
      if (storedVolume != null) {
        await setVolume(storedVolume);
      }

      notifyListeners();
    } catch (e) {
      print("Error loading settings: $e");
    }
  }

  void _startMonitoring() {
    _monitorTimer?.cancel();
    _monitorTimer = Timer.periodic(const Duration(seconds: 2), (_) async {
      final newBrightness = await _getSystemBrightness();
      final newVolume = await _getSystemVolume();

      bool changed = false;
      if (_brightness != newBrightness) {
        _brightness = newBrightness;
        changed = true;
      }
      if (_volume != newVolume) {
        _volume = newVolume;
        changed = true;
      }
      if (changed) notifyListeners();
    });
  }

  @override
  void dispose() {
    _monitorTimer?.cancel();
    super.dispose();
  }

  Future<void> setBrightness(double value) async {
    value = value.clamp(0.0, 1.0);
    if (_brightness != value) {
      _brightness = value;
      final prefs = await SharedPreferences.getInstance();
      await prefs.setDouble('brightness', _brightness);
      await _applySystemBrightness();
      notifyListeners();
    }
  }

  Future<void> setVolume(double value) async {
    value = value.clamp(0.0, 1.0);
    if (_volume != value) {
      _volume = value;
      final prefs = await SharedPreferences.getInstance();
      await prefs.setDouble('volume', _volume);
      await _applySystemVolume();
      notifyListeners();
    }
  }

  Future<void> turnScreenOff() async {
    _isScreenBlank = true;
    try {
      // turn off backlight
      await Process.run(
        'sh',
        ['-c', 'echo 1 | sudo tee /sys/class/backlight/dsi0-backlight/bl_power'],
      );
      notifyListeners();
    } catch (e) {
      print("Failed to turn screen off: $e");
    }
  }

  Future<void> turnScreenOn() async {
    _isScreenBlank = false;
    try {
      // turn on backlight
      await Process.run(
        'sh',
        ['-c', 'echo 0 | sudo tee /sys/class/backlight/dsi0-backlight/bl_power'],
      );
      // restore previous brightness
      await _applySystemBrightness();
      notifyListeners();
    } catch (e) {
      print("Failed to turn screen on: $e");
    }
  }

  Future<double> _getSystemBrightness() async {
    try {
      final dir = '/sys/class/backlight/dsi0-backlight';
      final cur = File('$dir/brightness');
      final maxf = File('$dir/max_brightness');
      if (!await cur.exists() || !await maxf.exists()) return 0.5;
      final current = int.parse((await cur.readAsString()).trim());
      final max = int.parse((await maxf.readAsString()).trim());
      return (max > 0 ? (current / max) : 0.5).clamp(0.0, 1.0);
    } catch (e) {
      print("Failed to read brightness: $e");
      return 0.5;
    }
  }

  Future<double> _getSystemVolume() async {
    final controls = ['Speaker', 'Headphone', 'Headphone Mixer'];
    for (final ctl in controls) {
      try {
        final res = await Process.run(
          '/usr/bin/amixer',
          ['-c', '1', 'get', ctl],
        );
        if (res.exitCode == 0) {
          final out = res.stdout as String;
          final m = RegExp(r'\[(\d{1,3})%\]').firstMatch(out);
          if (m != null) {
            return (int.parse(m.group(1)!) / 100).clamp(0.0, 1.0);
          }
        }
      } catch (_) {}
    }
    return 0.5;
  }

  Future<void> _applySystemBrightness() async {
    try {
      final dir = '/sys/class/backlight/dsi0-backlight';
      final max = int.parse((await File('$dir/max_brightness').readAsString()).trim());
      // Set a minimum brightness value (e.g., 5% of max, but at least 1)
      final minVal = (max * 0.05).round().clamp(1, max);
      var val = (_brightness * max).round();
      if (val < minVal) val = minVal;
      await Process.run(
        'sh',
        ['-c', 'echo $val | sudo tee $dir/brightness'],
      );
    } catch (e) {
      print("Failed to set brightness: $e");
    }
  }

  Future<void> _applySystemVolume() async {
    try {
      final volPct = '${(_volume * 100).round()}%';
      // Set all relevant controls on card 1
      final controls = ['Speaker', 'Headphone', 'Headphone Mixer'];
      for (final ctl in controls) {
        await Process.run(
          'sudo',
          ['/usr/bin/amixer', '-c', '1', 'sset', ctl, volPct],
        );
      }
    } catch (e) {
      print("Failed to set volume: $e");
    }
  }
}

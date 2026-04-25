import 'dart:async';
import 'dart:io';
import 'package:flutter/material.dart';
import 'settings_item.dart';

class PerformanceSettings {
  int _selectedMode = 1; // 0 = Performance, 1 = Balanced, 2 = Battery
  bool _isProcessing = false;
  String? _statusMessage;
  String? _errorMessage;

  final Function(VoidCallback) notifyParent;
  final BuildContext context;

  PerformanceSettings({
    required this.notifyParent,
    required this.context,
  });

  int get selectedMode => _selectedMode;
  bool get isProcessing => _isProcessing;
  String? get statusMessage => _statusMessage;
  String? get errorMessage => _errorMessage;

  static const _modes = [
    {
      'id': 0,
      'title': 'Performance Mode',
      'icon': Icons.flash_on,
      'desc': 'Maximum performance',
    },
    {
      'id': 1,
      'title': 'Balanced',
      'icon': Icons.speed,
      'desc': 'Balanced CPU usage',
    },
    {
      'id': 2,
      'title': 'Battery Saving',
      'icon': Icons.battery_saver,
      'desc': 'Save battery',
    },
  ];

  /// Only the 3 modes (no back, no double-selected, no dividers).
  List<SettingsItem> getPerformanceMenuItems() {
    return [
      SettingsItem.section(title: 'Performance Mode'),
      ...List.generate(_modes.length, (i) =>
        SettingsItem.action(
          id: _modes[i]['id'].toString(),
          title: _modes[i]['title'] as String,
          icon: _modes[i]['icon'] as IconData,
          subtitle: () =>
            (_selectedMode == i ? 'Current: ' : '') + (_modes[i]['desc'] as String),
          onSelect: _isProcessing ? null : () => setPerformanceMode(i),
        )
      ),
    ];
  }

  Widget buildPerformancePanel(
    List<SettingsItem> items,
    int selectedIndex,
    Function(int) onItemSelected,
    ScrollController scrollController
  ) {
    Widget? statusWidget;
    if (_isProcessing) {
      statusWidget = _StatusBanner(
        icon: Icons.sync,
        color: Colors.cyan,
        text: 'Applying...',
        subtext: 'Switching mode...',
        spinner: true,
      );
    } else if (_errorMessage != null) {
      statusWidget = _StatusBanner(
        icon: Icons.error_outline,
        color: Colors.red,
        text: 'Error',
        subtext: _errorMessage!,
        spinner: false,
      );
    } else if (_statusMessage != null) {
      statusWidget = _StatusBanner(
        icon: Icons.check_circle_outline,
        color: Colors.green,
        text: 'Success',
        subtext: _statusMessage!,
        spinner: false,
      );
    }

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const SizedBox(height: 18),
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 24),
          child: Text(
            'Performance Mode',
            style: const TextStyle(
              fontSize: 28,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
        ),
        if (statusWidget != null) ...[
          const SizedBox(height: 18),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 24),
            child: statusWidget,
          ),
          const SizedBox(height: 12),
        ],
        const SizedBox(height: 12),
        Expanded(
          child: ListView.builder(
            controller: scrollController,
            padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
            itemCount: _modes.length,
            itemBuilder: (context, i) {
              final item = items[i + 1]; // skip section header
              return SettingsItemWidget(
                item: item,
                isSelected: (i + 1) == selectedIndex,
                onTap: () {
                  if (!_isProcessing) onItemSelected(i + 1);
                },
              );
            },
          ),
        ),
      ],
    );
  }

  Future<void> setPerformanceMode(int mode) async {
    if (_isProcessing || mode == _selectedMode) return;
    _isProcessing = true;
    _statusMessage = null;
    _errorMessage = null;
    notifyParent(() {});

    String shellCommand;
    switch (mode) {
      case 0:
        shellCommand =
            "for CPU in /sys/devices/system/cpu/cpu[0-7]; do echo performance | sudo tee \$CPU/cpufreq/scaling_governor; done";
        break;
      case 1:
        shellCommand =
            "for CPU in /sys/devices/system/cpu/cpu[0-7]; do echo ondemand | sudo tee \$CPU/cpufreq/scaling_governor; done";
        break;
      case 2:
        shellCommand =
            "for CPU in /sys/devices/system/cpu/cpu[0-7]; do echo powersave | sudo tee \$CPU/cpufreq/scaling_governor; done";
        break;
      default:
        shellCommand = "";
    }

    try {
      final result = await Process.run('sh', ['-c', shellCommand]);
      if (result.exitCode == 0) {
        _selectedMode = mode;
        _statusMessage = '${_modes[mode]['title']} applied!';
        _errorMessage = null;
      } else {
        _statusMessage = null;
        _errorMessage = 'Failed: ${result.stderr}';
      }
    } catch (e) {
      _statusMessage = null;
      _errorMessage = 'Error: $e';
    }
    _isProcessing = false;
    notifyParent(() {});
  }
}

// Status/info box at top
class _StatusBanner extends StatelessWidget {
  final IconData icon;
  final Color color;
  final String text;
  final String subtext;
  final bool spinner;
  const _StatusBanner({
    required this.icon,
    required this.color,
    required this.text,
    required this.subtext,
    this.spinner = false,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: color.withOpacity(0.13),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: color.withOpacity(0.4), width: 1.5),
      ),
      padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 18),
      child: Row(
        children: [
          spinner
              ? SizedBox(
                  width: 28, height: 28,
                  child: CircularProgressIndicator(
                    valueColor: AlwaysStoppedAnimation<Color>(color),
                    strokeWidth: 3,
                  ),
                )
              : Icon(icon, size: 28, color: color),
          const SizedBox(width: 14),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  text,
                  style: TextStyle(
                    color: color,
                    fontWeight: FontWeight.bold,
                    fontSize: 18,
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  subtext,
                  style: TextStyle(
                    color: color.withOpacity(0.83),
                    fontSize: 15,
                  ),
                  maxLines: 2,
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
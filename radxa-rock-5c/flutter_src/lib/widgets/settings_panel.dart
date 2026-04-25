import 'dart:async';
import 'dart:io';
import 'dart:ui';

import 'package:control_panel/models/keyboard_manager.dart';
import 'package:control_panel/models/settings.dart';
import 'package:control_panel/widgets/performance_settings.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'settings_item.dart';
import 'wifi_settings.dart';
//import '_bluetooth_settings.dart';

class SettingsPanel extends StatefulWidget {
  final VoidCallback onClose;

  const SettingsPanel({
    super.key,
    required this.onClose,
  });

  @override
  State<SettingsPanel> createState() => _SettingsPanelState();
}

class _SettingsPanelState extends State<SettingsPanel>
    with TickerProviderStateMixin {
  // Settings state
  double _volumeLevel = 0.7;
  double _brightnessLevel = 0.8;

  final settings = Settings();
  Timer? _refreshTimer;

  // Navigation state
  String _currentPanel = 'main';
  int _selectedIndex = 0;
  final ScrollController _scrollController = ScrollController();

  // System info state
  String _storage = 'Loading…';
  String _memory = 'Loading…';
  String _cpuTemp = 'Loading…';
  String _ipAddress = 'Loading…';
  String _performanceGovernorLabel = 'Loading…';


  // Settings modules
  late WifiSettings _wifiSettings;
  late PerformanceSettings _performanceSettings;

  //late BluetoothSettings _bluetoothSettings;

  // Menu items
  late List<SettingsItem> _mainMenuItems;

  @override
  void initState() {
    super.initState();

    // Initialize settings modules with setState callback
    // Initialize settings modules with setState callback
    _wifiSettings = WifiSettings(
      notifyParent: setState,
      settings: settings,
      context: context,
    );

    _performanceSettings = PerformanceSettings(
      notifyParent: setState,
      context: context,
    );

    // Add this line to initialize the animation controllers
    _wifiSettings.initialize(this);

    /*_bluetoothSettings = BluetoothSettings(
      notifyParent: setState,
      context: context,
    );*/

    _refreshAll();
    // Then schedule periodic refresh every 15 seconds:
    _refreshTimer = Timer.periodic(
      const Duration(seconds: 15),
      (_) => _refreshAll(),
    );

    _initializeMainMenu();
  }

  Future<void> _refreshAll() async {
    getScreenBrightness();
    getSystemVolume();

    final results = await Future.wait([
      getStorageInfo(),
      getMemoryInfo(),
      getCpuTemp(),
      getIpAddress(),
      getPerformanceGovernorLabel(), // <--- Add this
    ]);
    if (!mounted) return;
    setState(() {
      _storage = results[0];
      _memory = results[1];
      _cpuTemp = results[2];
      _ipAddress = results[3];
      _performanceGovernorLabel = results[4]; // <--- Add this
    });
  }


  Future<String> getPerformanceGovernorLabel() async {
  try {
    final file = File('/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor');
    if (await file.exists()) {
      final gov = (await file.readAsString()).trim();
      switch (gov) {
        case 'performance':
          return 'Performance';
        case 'ondemand':
        case 'schedutil':
          return 'Balanced';
        case 'powersave':
          return 'Battery Saving';
        default:
          return gov[0].toUpperCase() + gov.substring(1);
      }
    }
    return 'Unknown';
  } catch (_) {
    return 'Unknown';
  }
}


  /// Returns something like "120.5 GB / 512 GB"
  Future<String> getStorageInfo() async {
    // -B1   → report in bytes
    // `/`  → root filesystem
    final result = await Process.run('df', ['-B1', '/']);
    if (result.exitCode != 0) return '— / —';

    final lines = (result.stdout as String).trim().split('\n');
    if (lines.length < 2) return '— / —';

    // df output: Filesystem 1B-blocks Used Available Use% Mounted on
    final cols = lines[1].split(RegExp(r'\s+'));
    final total = int.tryParse(cols[1]) ?? 0;
    final available = int.tryParse(cols[3]) ?? 0;

    String _fmt(int bytes) {
      final gb = bytes / (1024 * 1024 * 1024);
      if (gb >= 1) return '${gb.toStringAsFixed(1)} GB';
      return '${(bytes / (1024 * 1024)).toStringAsFixed(1)} MB';
    }

    final used = total - available;
    return '${_fmt(used)} / ${_fmt(total)}';

  }

  /// Formats a byte count as "x.x GB" or "y.y MB"
  String _fmtBytes(int bytes) {
    final gb = bytes / (1024 * 1024 * 1024);
    if (gb >= 1) return '${gb.toStringAsFixed(1)} GB';
    return '${(bytes / (1024 * 1024)).toStringAsFixed(1)} MB';
  }

  /// Returns something like "1.2 GB / 3.8 GB"
  Future<String> getMemoryInfo() async {
    final lines = await File('/proc/meminfo').readAsLines();
    int? totalKb, freeKb, buffersKb, cachedKb;

    for (final line in lines) {
      if (line.startsWith('MemTotal:')) {
        totalKb = int.tryParse(line.split(RegExp(r'\s+'))[1]);
      } else if (line.startsWith('MemFree:')) {
        freeKb = int.tryParse(line.split(RegExp(r'\s+'))[1]);
      } else if (line.startsWith('Buffers:')) {
        buffersKb = int.tryParse(line.split(RegExp(r'\s+'))[1]);
      } else if (line.startsWith('Cached:')) {
        cachedKb = int.tryParse(line.split(RegExp(r'\s+'))[1]);
      }

      if ([totalKb, freeKb, buffersKb, cachedKb].every((v) => v != null)) break;
    }

    if (totalKb == null || freeKb == null || buffersKb == null || cachedKb == null) {
      return '— / —';
    }

    final usedKb = totalKb - freeKb - buffersKb - cachedKb;
    final percentUsed = (usedKb / totalKb * 100).toStringAsFixed(0);

    return '${_fmtBytes(usedKb * 1024)} / ${_fmtBytes(totalKb * 1024)} ($percentUsed%)';
  }


  /// Returns something like "48.3 °C"
  Future<String> getCpuTemp() async {
    try {
      // On Raspberry Pi the CPU temp is here, in millidegrees Celsius
      final raw =
          await File('/sys/class/thermal/thermal_zone0/temp').readAsString();
      final milli = int.parse(raw.trim());
      final celsius = milli / 1000.0;
      return '${celsius.toStringAsFixed(1)} °C';
    } catch (e) {
      return '— °C';
    }
  }

  Future<String> getIpAddress() async {
    final ifaces = await NetworkInterface.list(type: InternetAddressType.IPv4);
    for (var iface in ifaces) {
      for (var addr in iface.addresses) {
        if (!addr.isLoopback) return addr.address;
      }
    }
    return '—';
  }

  void getScreenBrightness() {
    _brightnessLevel = settings.brightness;
  }

  void getSystemVolume() {
    _volumeLevel = settings.volume;
  }

  // Initialize main menu items
  void _initializeMainMenu() {
    // Main menu items
    _mainMenuItems = [
      // System section
      SettingsItem.section(title: 'System'),
      SettingsItem.slider(
        id: 'volume',
        title: 'Volume',
        icon: Icons.volume_up,
        getValue: () => _volumeLevel,
        setValue: (value) {
          setState(() {
            _volumeLevel = value;
          });
          settings.setVolume(value);
        },
      ),
      SettingsItem.slider(
        id: 'brightness',
        title: 'Brightness',
        icon: Icons.brightness_medium,
        getValue: () => _brightnessLevel,
        setValue: (value) {
          setState(() {
            _brightnessLevel = value;
          });
          settings.setBrightness(value);
        },
      ),
      SettingsItem.action(
        id: 'performance_mode',
        title: 'Performance Mode',
        icon: Icons.flash_on,
        subtitle: () => _performanceGovernorLabel,
        onSelect: () => _navigateToPanel('performance'),
      ),

      // Network section
      SettingsItem.section(title: 'Network'),
      SettingsItem.toggle(
        id: 'wifi',
        title: 'Wi-Fi',
        icon: Icons.wifi,
        subtitle: () =>
            _wifiSettings.isWifiEnabled ? 'Connected' : 'Disconnected',
        getValue: () => _wifiSettings.isWifiEnabled,
        setValue: (value) {
          setState(() {
            _wifiSettings.setWifiEnabled(value);
          });
        },
        hasSubmenu: true,
        onSelect: () => _navigateToPanel('wifi'),
      ),
      /*SettingsItem.toggle(
        id: 'bluetooth',
        title: 'Bluetooth',
        icon: Icons.bluetooth,
        subtitle: () => _bluetoothSettings.isBluetoothEnabled ? 'On' : 'Off',
        getValue: () => _bluetoothSettings.isBluetoothEnabled,
        setValue: (value) {
          setState(() {
            _bluetoothSettings.setBluetoothEnabled(value);
          });
        },
        hasSubmenu: true,
        onSelect: () => _navigateToPanel('bluetooth'),
      ),*/

      // Info section
      SettingsItem.section(title: 'Device Info'),
      SettingsItem.info(
        id: 'system',
        title: 'IP Address',
        icon: Icons.public,
        subtitle: () => _ipAddress,
      ),
      SettingsItem.info(
        id: 'storage',
        title: 'Storage',
        icon: Icons.storage,
        subtitle: () => _storage,
      ),
      SettingsItem.info(
        id: 'memory',
        title: 'Memory',
        icon: Icons.memory,
        subtitle: () => _memory,
      ),
      SettingsItem.info(
        id: 'cpu_temp',
        title: 'CPU Temperature',
        icon: Icons.thermostat,
        subtitle: () => _cpuTemp,
      ),

      // Power section
      SettingsItem.section(title: 'Power'),
      SettingsItem.action(
        id: 'restart',
        title: 'Restart',
        icon: Icons.restart_alt,
        subtitle: () => 'Restart the console',
        onSelect: () {
          _showConfirmationDialog(
            title: 'Restart Console',
            message: 'Are you sure you want to restart the console?',
            onConfirm: () async {
              await Process.run(
                'sudo',
                ['/bin/systemctl', 'reboot'],
              );
            },
          );
        },
      ),

      SettingsItem.action(
        id: 'power',
        title: 'Power Off',
        icon: Icons.power_settings_new,
        subtitle: () => 'Turn off the console',
        onSelect: () {
          _showConfirmationDialog(
            title: 'Power Off',
            message: 'Are you sure you want to turn off the console?',
            onConfirm: () async {
              await Process.run('sudo', ['/bin/systemctl', 'poweroff']);
            },
          );
        },
      ),
      SettingsItem.action(
        id: 'sleep',
        title: 'Sleep Mode',
        icon: Icons.nightlight,
        subtitle: () => 'Put the console to sleep',
        onSelect: () {
          settings.turnScreenOff();
        },
      ),
    ];
  }

  // Show confirmation dialog for power actions
  void _showConfirmationDialog({
    required String title,
    required String message,
    required VoidCallback onConfirm,
  }) {
    // Track which button is focused (0 = Cancel, 1 = Confirm)
    int focusedButton = 1; // Default to Confirm

    // Create a focus node for the dialog
    final dialogFocusNode = FocusNode();

    // Tell the keyboard manager a dialog is opening
    KeyboardManager().setDialogState(true);
    KeyboardManager().setActiveFocusNode(dialogFocusNode);

    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext dialogContext) {
        final screenSize = MediaQuery.of(context).size;
        final dialogWidth = screenSize.width * 0.85;

        return StatefulBuilder(
          builder: (BuildContext context, StateSetter setState) {
            return Focus(
              focusNode: dialogFocusNode,
              onKeyEvent: (FocusNode node, KeyEvent event) {
                if (event is KeyDownEvent) {
                  if (event.logicalKey == LogicalKeyboardKey.arrowUp ||
                      event.logicalKey == LogicalKeyboardKey.arrowDown) {
                    setState(() {
                      focusedButton = focusedButton == 0 ? 1 : 0;
                    });
                    return KeyEventResult.handled;
                  } else if (event.logicalKey == LogicalKeyboardKey.enter) {
                    if (focusedButton == 0) {
                      Navigator.of(dialogContext).pop();
                    } else {
                      Navigator.of(dialogContext).pop();
                      onConfirm();
                    }
                    return KeyEventResult.handled;
                  } else if (event.logicalKey == LogicalKeyboardKey.escape) {
                    Navigator.of(dialogContext).pop();
                    return KeyEventResult.handled;
                  }
                }
                return KeyEventResult.ignored;
              },
              child: RepaintBoundary(                      // ← ADDED
                child: BackdropFilter(
                  filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
                  child: Dialog(
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(24),
                    ),
                    elevation: 0,
                    backgroundColor: Colors.transparent,
                    insetPadding: const EdgeInsets.symmetric(
                      horizontal: 20,
                      vertical: 24,
                    ),
                    child: Container(
                      width: dialogWidth,
                      padding: const EdgeInsets.all(28),
                      decoration: BoxDecoration(
                        color: Colors.black.withOpacity(0.85),
                        borderRadius: BorderRadius.circular(24),
                        border: Border.all(
                          color: Colors.white.withOpacity(0.1),
                          width: 1,
                        ),
                      ),
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          // Icon at the top
                          Container(
                            padding: const EdgeInsets.all(20),
                            decoration: BoxDecoration(
                              color: Colors.white.withOpacity(0.1),
                              borderRadius: BorderRadius.circular(20),
                            ),
                            child: Icon(
                              title.contains('Power')
                                  ? Icons.power_settings_new
                                  : title.contains('Restart')
                                      ? Icons.restart_alt
                                      : title.contains('Sleep')
                                          ? Icons.nightlight
                                          : Icons.warning_rounded,
                              color: Colors.white,
                              size: 48,
                            ),
                          ),
                          const SizedBox(height: 24),

                          // Title
                          Text(
                            title,
                            style: const TextStyle(
                              color: Colors.white,
                              fontSize: 28,
                              fontWeight: FontWeight.bold,
                            ),
                            textAlign: TextAlign.center,
                          ),
                          const SizedBox(height: 16),

                          // Message
                          Text(
                            message,
                            style: TextStyle(
                              color: Colors.white.withOpacity(0.8),
                              fontSize: 20,
                            ),
                            textAlign: TextAlign.center,
                          ),
                          const SizedBox(height: 32),

                          // Buttons - Stack them vertically for more touch area
                          Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            crossAxisAlignment: CrossAxisAlignment.stretch,
                            children: [
                              // Confirm button
                              GestureDetector(
                                onTap: () {
                                  Navigator.of(dialogContext).pop();
                                  onConfirm();
                                },
                                child: AnimatedContainer(
                                  duration: const Duration(milliseconds: 150),
                                  padding: const EdgeInsets.symmetric(vertical: 16),
                                  decoration: BoxDecoration(
                                    color: focusedButton == 1
                                        ? Colors.red.withOpacity(0.6)
                                        : Colors.white.withOpacity(0.05),
                                    borderRadius: BorderRadius.circular(20),
                                  ),
                                  child: Center(
                                    child: Text(
                                      'Confirm',
                                      style: TextStyle(
                                        color: focusedButton == 1
                                            ? Colors.white
                                            : Colors.white.withOpacity(0.7),
                                        fontSize: 22,
                                        fontWeight: focusedButton == 1
                                            ? FontWeight.bold
                                            : FontWeight.normal,
                                      ),
                                    ),
                                  ),
                                ),
                              ),

                              const SizedBox(height: 16),

                              // Cancel button
                              GestureDetector(
                                onTap: () => Navigator.of(dialogContext).pop(),
                                child: AnimatedContainer(
                                  duration: const Duration(milliseconds: 150),
                                  padding: const EdgeInsets.symmetric(vertical: 16),
                                  decoration: BoxDecoration(
                                    color: focusedButton == 0
                                        ? Colors.white.withOpacity(0.15)
                                        : Colors.white.withOpacity(0.05),
                                    borderRadius: BorderRadius.circular(20),
                                  ),
                                  child: Center(
                                    child: Text(
                                      'Cancel',
                                      style: TextStyle(
                                        color: focusedButton == 0
                                            ? Colors.white
                                            : Colors.white.withOpacity(0.8),
                                        fontSize: 22,
                                        fontWeight: focusedButton == 0
                                            ? FontWeight.bold
                                            : FontWeight.normal,
                                      ),
                                    ),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            );
          },
        );
      },
    ).then((_) {
      // When dialog is closed, reset the keyboard manager
      KeyboardManager().setDialogState(false);

      // Return focus to the main settings panel
      final mainFocusNode = FocusNode();
      KeyboardManager().setActiveFocusNode(mainFocusNode);
    });
  }

  @override
  void dispose() {
    _scrollController.dispose();
    _refreshTimer?.cancel();
    super.dispose();
  }

  // Get current list of menu items based on panel
  List<SettingsItem> get _currentMenuItems {
    switch (_currentPanel) {
      case 'wifi':
        return _wifiSettings.getWifiMenuItems(_navigateBack);
      case 'performance':
        return _performanceSettings.getPerformanceMenuItems(); // <--- ADD THIS
      /*case 'bluetooth':*/
      default:
        return _mainMenuItems;
    }
  }

  // Handle keyboard events
  void _handleKeyEvent(RawKeyEvent event) {
    // Skip handling if a dialog is open
    if (KeyboardManager().isDialogOpen) return;

    if (event is! RawKeyDownEvent) return;

    if (event.logicalKey == LogicalKeyboardKey.arrowUp) {
      _navigateUp();
    } else if (event.logicalKey == LogicalKeyboardKey.arrowDown) {
      _navigateDown();
    } else if (event.logicalKey == LogicalKeyboardKey.arrowLeft) {
      _adjustCurrentSetting(decrease: true);
    } else if (event.logicalKey == LogicalKeyboardKey.arrowRight) {
      _adjustCurrentSetting(decrease: false);
    } else if (event.logicalKey == LogicalKeyboardKey.enter) {
      _selectCurrentItem();
    }
  }

  // Navigate up, skipping section headers
  void _navigateUp() {
    setState(() {
      int newIndex = _selectedIndex - 1;

      // Skip section headers when navigating up
      while (newIndex >= 0 &&
          _currentMenuItems[newIndex].type == SettingsItemType.section) {
        newIndex--;
      }

      // If we went past the beginning, wrap to the end
      if (newIndex < 0) {
        newIndex = _currentMenuItems.length - 1;
        // Skip section headers at the end too
        while (newIndex >= 0 &&
            _currentMenuItems[newIndex].type == SettingsItemType.section) {
          newIndex--;
        }
      }

      _selectedIndex = newIndex;
      _scrollToSelectedItem();
    });
  }

  // Navigate down, skipping section headers
  void _navigateDown() {
    setState(() {
      int newIndex = _selectedIndex + 1;

      // Skip section headers when navigating down
      while (newIndex < _currentMenuItems.length &&
          _currentMenuItems[newIndex].type == SettingsItemType.section) {
        newIndex++;
      }

      // If we went past the end, wrap to the beginning
      if (newIndex >= _currentMenuItems.length) {
        newIndex = 0;
        // Skip section headers at the beginning too
        while (newIndex < _currentMenuItems.length &&
            _currentMenuItems[newIndex].type == SettingsItemType.section) {
          newIndex++;
        }
      }

      _selectedIndex = newIndex;
      _scrollToSelectedItem();
    });
  }

  // Adjust current setting with left/right arrows
  void _adjustCurrentSetting({required bool decrease}) {
    if (_selectedIndex < 0 || _selectedIndex >= _currentMenuItems.length)
      return;

    final item = _currentMenuItems[_selectedIndex];

    switch (item.type) {
      case SettingsItemType.slider:
        if (item.sliderSetValue != null) {
          final currentValue = item.sliderGetValue?.call() ?? 0.0;
          final step = 0.05; // 5% step
          final newValue = decrease
              ? (currentValue - step).clamp(0.0, 1.0)
              : (currentValue + step).clamp(0.0, 1.0);
          item.sliderSetValue!(newValue);
        }
        break;

      case SettingsItemType.toggle:
        if (item.toggleSetValue != null) {
          final currentValue = item.toggleGetValue?.call() ?? false;
          item.toggleSetValue!(!currentValue);
        }
        break;

      default:
        break;
    }
  }

  // Select current item (Enter key or touch)
  void _selectCurrentItem() {
    if (_selectedIndex < 0 || _selectedIndex >= _currentMenuItems.length)
      return;

    final item = _currentMenuItems[_selectedIndex];

    if (item.onSelect != null) {
      item.onSelect!();
    } else if (item.type == SettingsItemType.toggle &&
        item.toggleSetValue != null) {
      final currentValue = item.toggleGetValue?.call() ?? false;
      item.toggleSetValue!(!currentValue);
    }
  }

  // Navigate to a panel
  void _navigateToPanel(String panel) {
    setState(() {
      _currentPanel = panel;

      // Find the first non-section item to select
      _selectedIndex = 0;
      while (_selectedIndex < _currentMenuItems.length &&
          _currentMenuItems[_selectedIndex].type == SettingsItemType.section) {
        _selectedIndex++;
      }

      // Reset scroll position for the new panel
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_scrollController.hasClients) {
          _scrollController.jumpTo(0);
          // Schedule a scroll to the selected item after layout
          _scrollToSelectedItem();
        }
      });
    });
  }

  // Navigate back to main menu
  void _navigateBack() {
    setState(() {
      _currentPanel = 'main';

      // Find the first non-section item to select
      _selectedIndex = 0;
      while (_selectedIndex < _currentMenuItems.length &&
          _currentMenuItems[_selectedIndex].type == SettingsItemType.section) {
        _selectedIndex++;
      }

      // Reset scroll position
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_scrollController.hasClients) {
          _scrollController.jumpTo(0);
          // Schedule a scroll to the selected item after layout
          _scrollToSelectedItem();
        }
      });
    });
  }

  // Scroll to selected item
  void _scrollToSelectedItem() {
    // Store the current scroll position before any state updates
    final currentScrollPosition =
        _scrollController.hasClients ? _scrollController.offset : 0.0;

    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scrollController.hasClients) {
        // Get the current list of items
        final items = _currentMenuItems;

        // Calculate position based on actual item heights
        double offset = 0;
        double selectedOffset = 0;

        // Calculate the offset of the selected item
        for (int i = 0; i < items.length; i++) {
          // Section headers are shorter, items are taller for touch
          final itemHeight =
              items[i].type == SettingsItemType.section ? 50.0 : 100.0;

          if (i < _selectedIndex) {
            offset += itemHeight;
          } else if (i == _selectedIndex) {
            selectedOffset = offset;
            break;
          }
        }

        // Get viewport height
        final viewportHeight = _scrollController.position.viewportDimension;

        // Calculate ideal position (centered in viewport)
        final targetOffset = selectedOffset - (viewportHeight / 2) + 150;

        // Clamp to valid scroll range
        final clampedOffset =
            targetOffset.clamp(0.0, _scrollController.position.maxScrollExtent);

        // First ensure we're at the current position (prevents jumping)
        _scrollController.jumpTo(currentScrollPosition);

        // Then animate to the target position
        _scrollController.animateTo(
          clampedOffset,
          duration: const Duration(milliseconds: 300),
          curve: Curves.easeOutCubic,
        );
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    // Create a focus node for the main settings panel
    final mainFocusNode = FocusNode();

    // Set it as active if no dialog is open
    if (!KeyboardManager().isDialogOpen) {
      KeyboardManager().setActiveFocusNode(mainFocusNode);
    }

    return RawKeyboardListener(
      focusNode: mainFocusNode,
      onKey: _handleKeyEvent,
      child: ClipRRect(
        child: RepaintBoundary(                    // ← add this
          child: BackdropFilter(
            filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
            child: Container(
              color: Colors.black.withOpacity(0.85),
              child: SafeArea(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    _buildHeader(),
                    Expanded(
                      child: _buildCurrentPanel(),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );

  }

  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.all(32.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Row(
            children: [
              if (_currentPanel != 'main')
                GestureDetector(
                  onTap: _navigateBack,
                  child: Container(
                    padding: const EdgeInsets.all(12),
                    decoration: BoxDecoration(
                      color: Colors.white.withAlpha((0.1 * 255).toInt()),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: const Icon(Icons.arrow_back,
                        color: Colors.white, size: 28),
                  ),
                ),
              if (_currentPanel != 'main')
                const SizedBox(width: 16),
              Text(
                _getPanelTitle(),
                style: const TextStyle(
                  fontSize: 28,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
            ],
          ),
          GestureDetector(
            onTap: widget.onClose,
            child: Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.1),
                borderRadius: BorderRadius.circular(12),
              ),
              child: const Icon(Icons.close, color: Colors.white, size: 28),
            ),
          ),
        ],
      ),
    );
  }

  String _getPanelTitle() {
    switch (_currentPanel) {
      case 'wifi':
        return 'Wi-Fi Networks';
      case 'bluetooth':
        return 'Bluetooth Devices';
      default:
        return 'Settings';
    }
  }

  // Build current panel with touch-friendly UI
  Widget _buildCurrentPanel() {
    final items = _currentMenuItems;

    // Handle WiFi panel
    if (_currentPanel == 'wifi') {
      return _wifiSettings.buildWifiPanel(items, _selectedIndex, (index) {
        setState(() {
          _selectedIndex = index;
          _selectCurrentItem();
        });
      }, _scrollController);
    }

    // ADD THIS for the performance panel:
    if (_currentPanel == 'performance') {
      return _performanceSettings.buildPerformancePanel(
        items,
        _selectedIndex,
        (index) {
          setState(() {
            _selectedIndex = index;
            _selectCurrentItem();
          });
        },
        _scrollController,
      );
    }

    // Handle Bluetooth panel
    /*if (_currentPanel == 'bluetooth') {
      return _bluetoothSettings.buildBluetoothPanel(items, _selectedIndex,
          (index) {
        setState(() {
          _selectedIndex = index;
          _selectCurrentItem();
        });
      }, _scrollController);
    }*/

    // Normal list view for main panel
    return ListView.builder(
      controller: _scrollController,
      padding: const EdgeInsets.symmetric(horizontal: 32),
      itemCount: items.length,
      itemBuilder: (context, index) {
        return GestureDetector(
          onTap: () {
            // Update selected index and trigger selection
            if (items[index].type != SettingsItemType.section) {
              setState(() {
                _selectedIndex = index;
              });
              _selectCurrentItem();
            }
          },
          child: SettingsItemWidget(
            item: items[index],
            isSelected: index == _selectedIndex,
          ),
        );
      },
    );
  }
}

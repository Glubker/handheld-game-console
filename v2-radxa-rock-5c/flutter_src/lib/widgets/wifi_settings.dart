import 'dart:async';
import 'dart:io';

import 'package:flutter/material.dart';
import 'custom_keyboard.dart';
import '/models/settings.dart';
import 'settings_item.dart';

class WifiSettings {
  // State
  bool _isWifiEnabled = false;
  bool _isScanning = false;
  bool _hasError = false;
  String _errorMessage = '';
  bool _isTogglingWifi = false;

  // Callback for state changes
  final Function(VoidCallback) notifyParent;

  // Settings reference for dark mode
  final Settings settings;

  // Context for showing overlays
  final BuildContext context;

  // Password input state
  final TextEditingController _passwordController = TextEditingController();
  bool _passwordVisible = false;
  bool _isConnecting = false;
  String? _connectionStatus;
  String? _connectionError;

  // Selected network for password input
  Map<String, dynamic>? _selectedNetwork;

  // Keyboard state
  bool _isKeyboardVisible = false;
  OverlayEntry? _keyboardOverlay;
  AnimationController? _keyboardAnimController;
  Animation<Offset>? _keyboardSlideAnimation;

  // Network data
  List<Map<String, dynamic>> _wifiNetworks = [];

  // Constructor with required callback
  WifiSettings({
    required this.notifyParent,
    required this.settings,
    required this.context,
  }) {
    // Check if WiFi is enabled on initialization
    _checkWifiStatus();
  }

  // Initialize with ticker provider (for animations)
  void initialize(TickerProvider vsync) {
    // Keyboard animation controller
    _keyboardAnimController = AnimationController(
      vsync: vsync,
      duration: Duration(milliseconds: 200),
    );

    _keyboardSlideAnimation = Tween<Offset>(
      begin: const Offset(0, 1), // Start from bottom
      end: Offset.zero, // End at original position
    ).animate(CurvedAnimation(
      parent: _keyboardAnimController!,
      curve: Curves.easeOutCubic,
    ));
  }

  void dispose() {
    _hideKeyboard();
    _keyboardAnimController?.dispose();
    _passwordController.dispose();
  }

  // Getters
  bool get isWifiEnabled => _isWifiEnabled;
  List<Map<String, dynamic>> get wifiNetworks => _wifiNetworks;
  bool get isScanning => _isScanning;
  bool get hasError => _hasError;
  String get errorMessage => _errorMessage;
  bool get isTogglingWifi => _isTogglingWifi;

  // Check if WiFi is enabled
  Future<void> _checkWifiStatus() async {
    try {
      final result = await Process.run('nmcli', [
        'radio',
        'wifi',
      ]);

      if (result.exitCode == 0) {
        final output = (result.stdout as String).trim();
        final enabled = output == 'enabled';

        // Only update if the state has changed
        if (_isWifiEnabled != enabled) {
          _isWifiEnabled = enabled;
          notifyParent(() {});

          // If WiFi is enabled, scan for networks
          if (_isWifiEnabled) {
            _scanWifiNetworks();
          } else {
            _wifiNetworks = [];
          }
        }
      } else {
        throw Exception('Failed to check WiFi status: ${result.stderr}');
      }
    } catch (e) {
      print('Error checking WiFi status: $e');
      _hasError = true;
      _errorMessage = 'Failed to check WiFi status: $e';
      notifyParent(() {});
    }
  }

  // Toggle WiFi on/off
  void setWifiEnabled(bool value) async {
    if (_isWifiEnabled == value || _isTogglingWifi) return;

    _isTogglingWifi = true;
    notifyParent(() {});

    try {
      final result = await Process.run('nmcli', [
        'radio',
        'wifi',
        value ? 'on' : 'off',
      ]);

      if (result.exitCode == 0) {
        _isWifiEnabled = value;
        _hasError = false;
        _errorMessage = '';

        if (value) {
          // Clear networks and start scanning
          _wifiNetworks = [];
          _scanWifiNetworks();
        } else {
          // Clear networks when WiFi is disabled
          _wifiNetworks = [];
        }
      } else {
        throw Exception('Failed to toggle WiFi: ${result.stderr}');
      }
    } catch (e) {
      print('Error toggling WiFi: $e');
      _hasError = true;
      _errorMessage = 'Failed to toggle WiFi: $e';
    } finally {
      _isTogglingWifi = false;
      notifyParent(() {});
    }
  }

  // Scan for available WiFi networks
  Future<void> _scanWifiNetworks() async {
    if (!_isWifiEnabled || _isScanning) return;

    _isScanning = true;
    _hasError = false;
    _errorMessage = '';
    notifyParent(() {});

    try {
      // First, trigger a rescan
      await Process.run('nmcli', ['device', 'wifi', 'rescan']);

      // Wait a moment for the scan to complete
      await Future.delayed(Duration(seconds: 2));

      final result = await Process.run('nmcli', [
        '-t', // Tabular output
        '-f', // Fields
        'SSID,SIGNAL,SECURITY,IN-USE', // Get these fields
        'device',
        'wifi',
        'list',
      ]);

      if (result.exitCode != 0) {
        throw Exception('Failed to scan WiFi networks: ${result.stderr}');
      }

      final List<String> lines = (result.stdout as String).split('\n');
      final List<Map<String, dynamic>> networks = [];
      final Set<String> uniqueSSIDs = {}; // To filter duplicate SSIDs

      for (final line in lines) {
        if (line.isEmpty) continue;

        final parts = line.split(':');
        if (parts.length >= 4) {
          final ssid = parts[0];
          if (ssid.isEmpty || uniqueSSIDs.contains(ssid))
            continue; // Skip empty or duplicate SSIDs

          uniqueSSIDs.add(ssid);

          final signal = int.tryParse(parts[1]) ?? 0;
          final security = parts[2];
          final inUse = parts[3] == '*';

          // Convert signal strength (0-100) to bars (0-4)
          int strength = 0;
          if (signal > 80)
            strength = 4;
          else if (signal > 60)
            strength = 3;
          else if (signal > 40)
            strength = 2;
          else if (signal > 20) strength = 1;

          networks.add({
            'name': ssid,
            'strength': strength,
            'secured': security != '',
            'connected': inUse,
          });
        }
      }

      // Sort networks by signal strength (highest first)
      networks.sort((a, b) => b['strength'].compareTo(a['strength']));

      _wifiNetworks = networks;
      _hasError = false;
      _errorMessage = '';
    } catch (e) {
      print('Error scanning WiFi networks: $e');
      _hasError = true;
      _errorMessage = 'Failed to scan networks: $e';
      _wifiNetworks = [];
    } finally {
      _isScanning = false;
      notifyParent(() {});
    }
  }

  // Connect to a network
  void connectToNetwork(int index) {
    if (index < 0 || index >= _wifiNetworks.length) return;

    final network = _wifiNetworks[index];

    // If already connected, do nothing
    if (network['connected'] == true) return;

    // If secured, show password screen
    if (network['secured'] == true) {
      _selectedNetwork = network;
      _showPasswordScreen();
    } else {
      // For open networks, connect directly
      _connectToWifi(network['name'], '');
    }
  }

  // Show keyboard with the specified controller
  // Replace the _showKeyboard method with this:
  void _showKeyboard(TextEditingController controller) {
    // Remove any existing keyboard overlay
    if (_keyboardOverlay != null) {
      _keyboardOverlay!.remove();
      _keyboardOverlay = null;
    }

    _isKeyboardVisible = true;
    notifyParent(() {});

    // Make sure the animation controller is initialized
    if (_keyboardAnimController == null) {
      print(
          "Error: Keyboard animation controller is null. Make sure initialize() was called.");
      return;
    }

    // Create and insert the overlay
    _keyboardOverlay = OverlayEntry(
      builder: (context) => Positioned(
        left: 0,
        right: 0,
        bottom: 0,
        child: SlideTransition(
          position: _keyboardSlideAnimation!,
          child: Material(
            elevation: 20,
            color: Colors.transparent,
            child: Container(
              height: 320, // Make sure height is specified
              color: Colors.black.withOpacity(0.9), // Add background color
              child: CustomKeyboard(
                controller: controller,
                isDarkMode: true,
                primaryColor: const Color(0xFF46A0AC),
                onClose: () {
                  _hideKeyboard();
                },
                onSubmit: () {
                  _hideKeyboard();
                  if (_selectedNetwork != null) {
                    _connectToWifi(
                        _selectedNetwork!['name'], _passwordController.text);
                  }
                },
              ),
            ),
          ),
        ),
      ),
    );

    // Make sure we have a valid context
    if (context.mounted) {
      Overlay.of(context).insert(_keyboardOverlay!);

      // Start the animation
      _keyboardAnimController!.forward(from: 0.0);
    } else {
      print("Error: Context is not mounted when trying to show keyboard");
      _keyboardOverlay = null;
      _isKeyboardVisible = false;
    }
  }

  // Hide keyboard with animation
  void _hideKeyboard() {
    if (_keyboardOverlay != null &&
        _keyboardAnimController!.isAnimating == false) {
      // Immediately update the state
      _isKeyboardVisible = false;
      notifyParent(() {});

      // Reverse the animation and then remove the overlay
      _keyboardAnimController!.reverse().then((_) {
        if (_keyboardOverlay != null) {
          _keyboardOverlay!.remove();
          _keyboardOverlay = null;
        }

        notifyParent(() {});
      });
    } else if (_keyboardOverlay == null) {
      _isKeyboardVisible = false;
      notifyParent(() {});
    }
  }

  // Connect to WiFi network
  Future<void> _connectToWifi(String ssid, String password) async {
    _isConnecting = true;
    _connectionStatus = 'Connecting to $ssid...';
    _connectionError = null;
    notifyParent(() {});

    try {
      _hideKeyboard();

      final result = await Process.run(
          'nmcli',
          [
            'device',
            'wifi',
            'connect',
            ssid,
            password.isNotEmpty ? 'password' : '',
            password.isNotEmpty ? password : '',
          ].where((arg) => arg.isNotEmpty).toList());

      if (result.exitCode == 0) {
        _isConnecting = false;
        _connectionStatus = 'Connected to $ssid';
        _connectionError = null;

        // Update network list to show the new connection
        await Future.delayed(Duration(seconds: 1));
        _scanWifiNetworks();

        // Close password screen after a delay
        Timer(Duration(seconds: 2), () {
          _selectedNetwork = null;
          notifyParent(() {});
        });
      } else {
        throw Exception(result.stderr ?? 'Unknown connection error.');
      }
    } catch (e) {
      _isConnecting = false;
      _connectionStatus = 'Connection failed';
      _connectionError = e.toString();
    }

    notifyParent(() {});
  }

  // Show fullscreen password input
// Update the _showPasswordScreen method in wifi_settings.dart:
  void _showPasswordScreen() {
    _passwordController.clear();
    _passwordVisible = false;
    _connectionStatus = null;
    _connectionError = null;
    _isConnecting = false;

    // Use the new fullscreen keyboard instead of an overlay
    showGeneralDialog(
      context: context,
      barrierDismissible: false,
      pageBuilder: (context, animation, secondaryAnimation) {
        return CustomKeyboard(
          controller: _passwordController,
          onClose: () {
            Navigator.of(context).pop();
            _selectedNetwork = null;
            notifyParent(() {});
          },
          onSubmit: () {
            Navigator.of(context).pop();
            if (_selectedNetwork != null) {
              _connectToWifi(
                  _selectedNetwork!['name'], _passwordController.text);
            }
          },
          isDarkMode: true,
          primaryColor: const Color(0xFF46A0AC),
          obscureText: true,
        );
      },
    );

    notifyParent(() {});
  }

  // Close password screen
  void _closePasswordScreen() {
    _hideKeyboard();
    _selectedNetwork = null;
    _connectionStatus = null;
    _connectionError = null;
    notifyParent(() {});
  }

  // Get WiFi strength icon
  IconData getWifiStrengthIcon(int strength) {
    switch (strength) {
      case 4:
        return Icons.signal_wifi_4_bar;
      case 3:
        return Icons.network_wifi_3_bar;
      case 2:
        return Icons.network_wifi_2_bar;
      case 1:
        return Icons.network_wifi_1_bar;
      default:
        return Icons.signal_wifi_0_bar;
    }
  }

  // Generate menu items
  List<SettingsItem> getWifiMenuItems(VoidCallback navigateBack) {
    List<SettingsItem> items = [
      SettingsItem.toggle(
        id: 'wifi_toggle',
        title: 'Wi-Fi',
        icon: Icons.wifi,
        getValue: () => _isWifiEnabled,
        setValue: (bool value) {
          if (!_isTogglingWifi) {
            setWifiEnabled(value);
          }
        },
      ),
      SettingsItem.action(
        id: 'back',
        title: 'Back to Settings',
        icon: Icons.arrow_back,
        onSelect: navigateBack,
      ),
    ];

    // Add refresh action if WiFi is enabled
    if (_isWifiEnabled) {
      items.add(
        SettingsItem.action(
          id: 'refresh',
          title: _isScanning ? 'Scanning...' : 'Refresh Networks',
          icon: _isScanning ? Icons.sync : Icons.refresh,
          onSelect: _isScanning ? null : () => _scanWifiNetworks(),
        ),
      );
    }

    // Add WiFi networks section if we have networks
    if (_wifiNetworks.isNotEmpty) {
      items.add(SettingsItem.section(title: 'Available Networks'));

      // Add networks
      // In the getWifiMenuItems method, modify how network items are added:
      // Add networks
      for (int i = 0; i < _wifiNetworks.length; i++) {
        final network = _wifiNetworks[i];
        final int networkIndex = i;
        items.add(
          SettingsItem.network(
            id: 'network_$i',
            title: network['name'],
            icon: getWifiStrengthIcon(network['strength']),
            subtitle: () => network['connected'] ? 'Connected' : '',
            getValue: () => network['connected'] as bool,
            onSelect: () => connectToNetwork(networkIndex),
            isSecured: network['secured'] as bool,
            // Add this line to pass the connected status to the widget
            isConnected: network['connected'] as bool,
          ),
        );
      }
    }

    return items;
  }

  // Build WiFi panel with empty state
  Widget buildWifiPanel(List<SettingsItem> items, int selectedIndex,
      Function(int) onItemSelected, ScrollController scrollController) {
    // If password screen is active, show it
    if (_selectedNetwork != null) {
      return _buildPasswordScreen();
    }

    // Always show the WiFi toggle and back button
    final headerItems = <Widget>[
      Padding(
        padding: const EdgeInsets.symmetric(horizontal: 24),
        child: SettingsItemWidget(
          item: items[0],
          isSelected: selectedIndex == 0,
          onTap: () => onItemSelected(0),
        ), // WiFi toggle
      ),
      Padding(
        padding: const EdgeInsets.symmetric(horizontal: 24),
        child: SettingsItemWidget(
          item: items[1],
          isSelected: selectedIndex == 1,
          onTap: () => onItemSelected(1),
        ), // Back button
      ),
    ];

    // If WiFi is being toggled, show loading
    if (_isTogglingWifi) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          ...headerItems,
          Expanded(
            child: Center(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  SizedBox(
                    width: 50,
                    height: 50,
                    child: CircularProgressIndicator(
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(height: 24),
                  Text(
                    _isWifiEnabled
                        ? 'Turning WiFi off...'
                        : 'Turning WiFi on...',
                    style: TextStyle(
                      fontSize: 22,
                      color: Colors.white.withOpacity(0.7),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      );
    }

    // If WiFi is disabled, show empty state
    if (!_isWifiEnabled) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          ...headerItems,
          Expanded(
            child: Center(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    Icons.wifi_off,
                    size: 64,
                    color: Colors.white.withOpacity(0.5),
                  ),
                  const SizedBox(height: 24),
                  Text(
                    'Wi-Fi is turned off',
                    style: TextStyle(
                      fontSize: 22,
                      color: Colors.white.withOpacity(0.7),
                    ),
                  ),
                  const SizedBox(height: 12),
                  Text(
                    'Turn on Wi-Fi to see available networks',
                    style: TextStyle(
                      fontSize: 18,
                      color: Colors.white.withOpacity(0.5),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      );
    }

    // If scanning, show loading
    if (_isScanning) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          ...headerItems,
          // Add refresh button
          if (items.length > 2)
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: SettingsItemWidget(
                item: items[2],
                isSelected: selectedIndex == 2,
                onTap: () => onItemSelected(2),
              ),
            ),
          Expanded(
            child: Center(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  SizedBox(
                    width: 50,
                    height: 50,
                    child: CircularProgressIndicator(
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(height: 24),
                  Text(
                    'Scanning for networks...',
                    style: TextStyle(
                      fontSize: 22,
                      color: Colors.white.withOpacity(0.7),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      );
    }

    // If there's an error, show error state
    if (_hasError) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          ...headerItems,
          // Add refresh button
          if (items.length > 2)
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: SettingsItemWidget(
                item: items[2],
                isSelected: selectedIndex == 2,
                onTap: () => onItemSelected(2),
              ),
            ),
          Expanded(
            child: Center(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    Icons.error_outline,
                    size: 64,
                    color: Colors.red.withOpacity(0.7),
                  ),
                  const SizedBox(height: 24),
                  Text(
                    'Error scanning networks',
                    style: TextStyle(
                      fontSize: 22,
                      color: Colors.white.withOpacity(0.7),
                    ),
                  ),
                  const SizedBox(height: 12),
                  Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 32),
                    child: Text(
                      _errorMessage,
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        fontSize: 16,
                        color: Colors.red.withOpacity(0.7),
                      ),
                    ),
                  ),
                  const SizedBox(height: 24),
                  ElevatedButton(
                    onPressed: _scanWifiNetworks,
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.white.withOpacity(0.1),
                      padding:
                          EdgeInsets.symmetric(horizontal: 24, vertical: 12),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(16),
                      ),
                    ),
                    child: Text(
                      'Try Again',
                      style: TextStyle(
                        fontSize: 18,
                        color: Colors.white,
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      );
    }

    // If no networks found, show empty state
    if (_wifiNetworks.isEmpty) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          ...headerItems,
          // Add refresh button
          if (items.length > 2)
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: SettingsItemWidget(
                item: items[2],
                isSelected: selectedIndex == 2,
                onTap: () => onItemSelected(2),
              ),
            ),
          Expanded(
            child: Center(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    Icons.wifi_find,
                    size: 64,
                    color: Colors.white.withOpacity(0.5),
                  ),
                  const SizedBox(height: 24),
                  Text(
                    'No networks found',
                    style: TextStyle(
                      fontSize: 22,
                      color: Colors.white.withOpacity(0.7),
                    ),
                  ),
                  const SizedBox(height: 12),
                  Text(
                    'Try refreshing or check your WiFi adapter',
                    style: TextStyle(
                      fontSize: 18,
                      color: Colors.white.withOpacity(0.5),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      );
    }

    // Normal list view for networks
    return ListView.builder(
      controller: scrollController,
      padding: const EdgeInsets.symmetric(horizontal: 24),
      itemCount: items.length,
      itemBuilder: (context, index) {
        return SettingsItemWidget(
          item: items[index],
          isSelected: index == selectedIndex,
          onTap: () {
            if (items[index].type != SettingsItemType.section) {
              onItemSelected(index);
            }
          },
        );
      },
    );
  }

  // Build password input screen
  Widget _buildPasswordScreen() {
    if (_selectedNetwork == null) return SizedBox();

    return Container(
      color: Colors.black.withOpacity(0.85),
      child: Padding(
        padding: EdgeInsets.only(
          bottom: _isKeyboardVisible ? 320 : 0,
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Header
            Padding(
              padding: const EdgeInsets.all(24.0),
              child: Row(
                children: [
                  GestureDetector(
                    onTap: _isConnecting ? null : _closePasswordScreen,
                    child: Container(
                      padding: const EdgeInsets.all(12),
                      decoration: BoxDecoration(
                        color: Colors.white
                            .withOpacity(_isConnecting ? 0.05 : 0.1),
                        borderRadius: BorderRadius.circular(12),
                      ),
                      child: Icon(Icons.arrow_back,
                          color: Colors.white
                              .withOpacity(_isConnecting ? 0.3 : 1.0),
                          size: 28),
                    ),
                  ),
                  const SizedBox(width: 16),
                  Expanded(
                    child: Text(
                      'Connect to ${_selectedNetwork!['name']}',
                      style: TextStyle(
                        fontSize: 28,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                      ),
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                ],
              ),
            ),

            // Network info
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24.0),
              child: Container(
                padding: EdgeInsets.all(20),
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(20),
                ),
                child: Row(
                  children: [
                    Icon(
                      getWifiStrengthIcon(_selectedNetwork!['strength']),
                      color: Colors.white,
                      size: 32,
                    ),
                    SizedBox(width: 16),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            _selectedNetwork!['name'],
                            style: TextStyle(
                              fontSize: 20,
                              fontWeight: FontWeight.w600,
                              color: Colors.white,
                            ),
                          ),
                          Text(
                            _selectedNetwork!['secured']
                                ? 'Secured network'
                                : 'Open network',
                            style: TextStyle(
                              fontSize: 16,
                              color: Colors.white.withOpacity(0.7),
                            ),
                          ),
                        ],
                      ),
                    ),
                    if (_selectedNetwork!['secured'])
                      Icon(
                        Icons.lock_outline,
                        color: Colors.white.withOpacity(0.7),
                        size: 24,
                      ),
                  ],
                ),
              ),
            ),

            SizedBox(height: 32),

            // Password field
            if (_selectedNetwork!['secured']) ...[
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 24.0),
                child: Text(
                  'Password',
                  style: TextStyle(
                    fontSize: 20,
                    fontWeight: FontWeight.w600,
                    color: Colors.white,
                  ),
                ),
              ),
              SizedBox(height: 12),
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 24.0),
                child: Container(
                  height: 70,
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(16),
                    color: Colors.white.withOpacity(0.1),
                    border: Border.all(
                      color: _connectionError != null
                          ? Colors.red.withOpacity(0.5)
                          : Colors.white.withOpacity(0.2),
                      width: 1,
                    ),
                  ),
                  child: Center(
                    child: TextField(
                      controller: _passwordController,
                      readOnly: true, // Prevent system keyboard
                      enabled: !_isConnecting,
                      obscureText: !_passwordVisible,
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 20,
                      ),
                      decoration: InputDecoration(
                        prefixIcon: Padding(
                          padding: EdgeInsets.all(12),
                          child: Icon(
                            Icons.lock_outline,
                            size: 28,
                            color: Colors.white
                                .withOpacity(_isConnecting ? 0.3 : 0.7),
                          ),
                        ),
                        suffixIcon: IconButton(
                          iconSize: 28,
                          padding: EdgeInsets.all(12),
                          icon: Icon(
                            _passwordVisible
                                ? Icons.visibility_off
                                : Icons.visibility,
                            color: Colors.white
                                .withOpacity(_isConnecting ? 0.3 : 0.7),
                          ),
                          onPressed: _isConnecting
                              ? null
                              : () {
                                  _passwordVisible = !_passwordVisible;
                                  notifyParent(() {});
                                },
                        ),
                        hintText: 'Enter Password',
                        hintStyle: TextStyle(
                          color: Colors.white.withOpacity(0.4),
                          fontSize: 20,
                        ),
                        border: InputBorder.none,
                        contentPadding:
                            EdgeInsets.symmetric(vertical: 20, horizontal: 20),
                      ),
                      onTap: _isConnecting
                          ? null
                          : () {
                              _showKeyboard(_passwordController);
                            },
                    ),
                  ),
                ),
              ),
            ],

            // Status message
            if (_connectionStatus != null)
              Padding(
                padding: const EdgeInsets.only(top: 24, left: 24, right: 24),
                child: Container(
                  padding: EdgeInsets.all(16),
                  decoration: BoxDecoration(
                    color: _connectionError != null
                        ? Colors.red.withOpacity(0.1)
                        : Colors.green.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(16),
                    border: Border.all(
                      color: _connectionError != null
                          ? Colors.red.withOpacity(0.3)
                          : Colors.green.withOpacity(0.3),
                      width: 1.5,
                    ),
                  ),
                  child: Row(
                    children: [
                      _isConnecting
                          ? SizedBox(
                              width: 24,
                              height: 24,
                              child: CircularProgressIndicator(
                                strokeWidth: 2,
                                valueColor: AlwaysStoppedAnimation<Color>(
                                  _connectionError != null
                                      ? Colors.red
                                      : Colors.green,
                                ),
                              ),
                            )
                          : Icon(
                              _connectionError != null
                                  ? Icons.error_outline
                                  : Icons.check_circle_outline,
                              color: _connectionError != null
                                  ? Colors.red
                                  : Colors.green,
                              size: 28,
                            ),
                      SizedBox(width: 12),
                      Expanded(
                        child: Text(
                          _connectionStatus!,
                          style: TextStyle(
                            fontSize: 18,
                            color: _connectionError != null
                                ? Colors.red
                                : Colors.green,
                            fontWeight: FontWeight.w500,
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
              ),

            // Error details
            if (_connectionError != null)
              Padding(
                padding: const EdgeInsets.only(top: 12, left: 24, right: 24),
                child: Text(
                  _connectionError!,
                  style: TextStyle(
                    fontSize: 14,
                    color: Colors.red.withOpacity(0.8),
                  ),
                ),
              ),

            Spacer(),

            // Connect button
            Padding(
              padding: const EdgeInsets.all(24.0),
              child: Row(
                children: [
                  // Cancel button
                  Expanded(
                    child: SizedBox(
                      height: 60,
                      child: TextButton(
                        onPressed: _isConnecting ? null : _closePasswordScreen,
                        style: TextButton.styleFrom(
                          backgroundColor: Colors.white.withOpacity(0.1),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(16),
                          ),
                          padding: EdgeInsets.zero,
                        ),
                        child: Text(
                          'Cancel',
                          style: TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.w600,
                            color: _isConnecting
                                ? Colors.white.withOpacity(0.3)
                                : Colors.white.withOpacity(0.7),
                          ),
                        ),
                      ),
                    ),
                  ),
                  SizedBox(width: 16),
                  // Connect button
                  Expanded(
                    child: SizedBox(
                      height: 60,
                      child: ElevatedButton(
                        onPressed: _isConnecting
                            ? null
                            : () {
                                final ssid = _selectedNetwork!['name'];
                                final password = _passwordController.text;

                                // For open networks, we can connect without a password
                                if (!_selectedNetwork!['secured'] ||
                                    password.isNotEmpty) {
                                  _connectToWifi(ssid, password);
                                } else {
                                  _connectionStatus = 'Connection failed';
                                  _connectionError = 'Please enter password';
                                  notifyParent(() {});
                                }
                              },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: const Color(0xFF46A0AC),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(16),
                          ),
                          elevation: 0,
                          padding: EdgeInsets.zero,
                        ),
                        child: _isConnecting
                            ? SizedBox(
                                width: 24,
                                height: 24,
                                child: CircularProgressIndicator(
                                  strokeWidth: 3,
                                  valueColor: AlwaysStoppedAnimation<Color>(
                                    Colors.white,
                                  ),
                                ),
                              )
                            : Text(
                                'Connect',
                                style: TextStyle(
                                  fontSize: 20,
                                  fontWeight: FontWeight.w600,
                                  color: Colors.white,
                                ),
                              ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

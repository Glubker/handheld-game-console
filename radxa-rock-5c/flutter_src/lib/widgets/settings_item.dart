import 'package:flutter/material.dart';

// Settings item type enum
enum SettingsItemType {
  section,
  slider,
  toggle,
  info,
  action,
  network,
  device,
}

// Settings item class
class SettingsItem {
  final String title;
  final IconData icon;
  final SettingsItemType type;
  final String? id;
  final String Function()? subtitle;
  final double Function()? sliderGetValue;
  final Function(double)? sliderSetValue;
  final bool Function()? toggleGetValue;
  final Function(bool)? toggleSetValue;
  final VoidCallback? onSelect;
  final VoidCallback? onLongPress;
  final bool hasSubmenu;
  final bool? isSecured;
  final bool? isConnected;

  // Private constructor
  SettingsItem._({
    required this.title,
    required this.icon,
    required this.type,
    this.id,
    this.subtitle,
    this.sliderGetValue,
    this.sliderSetValue,
    this.toggleGetValue,
    this.toggleSetValue,
    this.onSelect,
    this.onLongPress,
    this.hasSubmenu = false,
    this.isSecured,
    this.isConnected, // Add this line
  });

  // Factory constructor for section headers
  factory SettingsItem.section({
    required String title,
  }) {
    return SettingsItem._(
      id: 'section_${title.toLowerCase().replaceAll(' ', '_')}',
      title: title,
      icon: Icons.circle, // Not used for sections
      type: SettingsItemType.section,
    );
  }

  // Factory constructor for slider items
  factory SettingsItem.slider({
    required String title,
    required IconData icon,
    required double Function() getValue,
    required Function(double) setValue,
    String Function()? subtitle,
    String? id,
  }) {
    return SettingsItem._(
      title: title,
      icon: icon,
      type: SettingsItemType.slider,
      subtitle: subtitle,
      sliderGetValue: getValue,
      sliderSetValue: setValue,
      id: id,
    );
  }

  // Factory constructor for toggle items
  factory SettingsItem.toggle({
    required String title,
    required IconData icon,
    required bool Function() getValue,
    required Function(bool) setValue,
    String Function()? subtitle,
    bool hasSubmenu = false,
    VoidCallback? onSelect,
    String? id,
  }) {
    return SettingsItem._(
      title: title,
      icon: icon,
      type: SettingsItemType.toggle,
      subtitle: subtitle,
      toggleGetValue: getValue,
      toggleSetValue: setValue,
      hasSubmenu: hasSubmenu,
      onSelect: onSelect,
      id: id,
    );
  }

  // Factory constructor for info items
  factory SettingsItem.info({
    required String title,
    required IconData icon,
    String Function()? subtitle,
    String? id,
  }) {
    return SettingsItem._(
      title: title,
      icon: icon,
      type: SettingsItemType.info,
      subtitle: subtitle,
      id: id,
    );
  }

  // Factory constructor for action items
  factory SettingsItem.action({
    required String title,
    required IconData icon,
    String Function()? subtitle,
    VoidCallback? onSelect,
    String? id,
  }) {
    return SettingsItem._(
      title: title,
      icon: icon,
      type: SettingsItemType.action,
      subtitle: subtitle,
      onSelect: onSelect,
      id: id,
    );
  }

  // Factory constructor for network items
  // In the SettingsItem class:
  factory SettingsItem.network({
    required String title,
    required IconData icon,
    required bool Function() getValue,
    String Function()? subtitle,
    VoidCallback? onSelect,
    bool? isSecured,
    bool isConnected = false, // Add this parameter
    String? id,
  }) {
    return SettingsItem._(
      title: title,
      icon: icon,
      type: SettingsItemType.network,
      subtitle: subtitle,
      toggleGetValue: getValue,
      onSelect: onSelect,
      isSecured: isSecured,
      isConnected: isConnected, // Store the connected status
      id: id,
    );
  }

  // Factory constructor for device items
  factory SettingsItem.device({
    required String title,
    required IconData icon,
    required bool Function() getValue,
    String Function()? subtitle,
    VoidCallback? onSelect,
    VoidCallback? onLongPress,
    String? id,
  }) {
    return SettingsItem._(
      title: title,
      icon: icon,
      type: SettingsItemType.device,
      subtitle: subtitle,
      toggleGetValue: getValue,
      onSelect: onSelect,
      onLongPress: onLongPress,
      id: id,
    );
  }
}

// Shared widget to build different types of settings items
class SettingsItemWidget extends StatelessWidget {
  final SettingsItem item;
  final bool isSelected;
  final VoidCallback? onTap;
  final VoidCallback? onLongPress;

  const SettingsItemWidget({
    Key? key,
    required this.item,
    required this.isSelected,
    this.onTap,
    this.onLongPress,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    // Different rendering based on item type
    switch (item.type) {
      case SettingsItemType.section:
        return _buildSectionHeader();
      case SettingsItemType.slider:
        return _buildSliderItem();
      case SettingsItemType.toggle:
        return _buildToggleItem();
      case SettingsItemType.info:
        return _buildInfoItem();
      case SettingsItemType.action:
        return _buildActionItem();
      case SettingsItemType.network:
        return _buildNetworkItem();
      case SettingsItemType.device:
        return _buildDeviceItem();
    }
  }

  // Build section header with larger text
  Widget _buildSectionHeader() {
    return Padding(
      padding: const EdgeInsets.only(top: 32, bottom: 12),
      child: Text(
        item.title,
        style: TextStyle(
          fontSize: 20,
          fontWeight: FontWeight.bold,
          color: isSelected ? Colors.white : Colors.white.withOpacity(0.5),
        ),
      ),
    );
  }

  // Build slider item with larger touch targets
  Widget _buildSliderItem() {
    final value = item.sliderGetValue?.call() ?? 0.0;

    return GestureDetector(
      onTap: onTap,
      onLongPress: onLongPress,
      child: Container(
        margin: const EdgeInsets.only(bottom: 20),
        decoration: BoxDecoration(
          color: isSelected
              ? Colors.white.withOpacity(0.15)
              : Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(20),
        ),
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Container(
                    width: 48,
                    height: 48,
                    decoration: BoxDecoration(
                      color: Colors.white.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(14),
                    ),
                    child: Icon(
                      item.icon,
                      color: Colors.white,
                      size: 26,
                    ),
                  ),
                  const SizedBox(width: 16),
                  Text(
                    item.title,
                    style: const TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.w600,
                      color: Colors.white,
                    ),
                  ),
                  const Spacer(),
                  Text(
                    '${(value * 100).round()}%',
                    style: const TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.w600,
                      color: Colors.white70,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 24),
              SliderTheme(
                data: SliderThemeData(
                  trackHeight: 10,
                  activeTrackColor: Colors.white,
                  inactiveTrackColor: Colors.white24,
                  thumbColor: Colors.white,
                  overlayColor: Colors.white.withOpacity(0.2),
                  thumbShape:
                      const RoundSliderThumbShape(enabledThumbRadius: 16),
                  overlayShape:
                      const RoundSliderOverlayShape(overlayRadius: 28),
                ),
                child: Slider(
                  value: value,
                  onChanged: item.sliderSetValue,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // Build toggle item with larger touch targets
  Widget _buildToggleItem() {
    final value = item.toggleGetValue?.call() ?? false;

    return GestureDetector(
      onTap: onTap,
      child: Container(
        margin: const EdgeInsets.only(bottom: 20),
        decoration: BoxDecoration(
          color: isSelected
              ? Colors.white.withOpacity(0.15)
              : Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(20),
        ),
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Row(
            children: [
              Container(
                width: 48,
                height: 48,
                decoration: BoxDecoration(
                  color: value
                      ? Colors.white.withOpacity(0.15)
                      : Colors.white.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(14),
                ),
                child: Icon(
                  item.icon,
                  color: value ? Colors.white : Colors.white54,
                  size: 26,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      item.title,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.w600,
                        color: Colors.white,
                      ),
                    ),
                    if (item.subtitle != null)
                      Text(
                        item.subtitle!(),
                        style: const TextStyle(
                          fontSize: 16,
                          color: Colors.white54,
                        ),
                      ),
                  ],
                ),
              ),
              Transform.scale(
                scale: 1.3,
                child: Switch(
                  value: value,
                  onChanged: item.toggleSetValue,
                  activeColor: Colors.white,
                  activeTrackColor:
                      const Color.fromARGB(255, 0, 106, 255).withOpacity(0.9),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // Build info item with larger touch targets
  Widget _buildInfoItem() {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        margin: const EdgeInsets.only(bottom: 20),
        decoration: BoxDecoration(
          color: isSelected
              ? Colors.white.withOpacity(0.15)
              : Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(20),
        ),
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Row(
            children: [
              Container(
                width: 48,
                height: 48,
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(14),
                ),
                child: Icon(
                  item.icon,
                  color: Colors.white70,
                  size: 26,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      item.title,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.w600,
                        color: Colors.white,
                      ),
                    ),
                    if (item.subtitle != null)
                      Text(
                        item.subtitle!(),
                        style: const TextStyle(
                          fontSize: 16,
                          color: Colors.white54,
                        ),
                      ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // Build action item with larger touch targets
  Widget _buildActionItem() {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        margin: const EdgeInsets.only(bottom: 20),
        decoration: BoxDecoration(
          color: isSelected
              ? Colors.white.withOpacity(0.15)
              : Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(20),
        ),
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Row(
            children: [
              Container(
                width: 48,
                height: 48,
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(14),
                ),
                child: Icon(
                  item.icon,
                  color: Colors.white70,
                  size: 26,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      item.title,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.w600,
                        color: Colors.white,
                      ),
                    ),
                    if (item.subtitle != null)
                      Text(
                        item.subtitle!(),
                        style: const TextStyle(
                          fontSize: 16,
                          color: Colors.white54,
                        ),
                      ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // Build network item with larger touch targets
  Widget _buildNetworkItem() {
    final bool isConnected = item.toggleGetValue?.call() ?? false;

    return GestureDetector(
      onTap: onTap,
      child: Container(
        margin: const EdgeInsets.only(bottom: 20),
        decoration: BoxDecoration(
          color: isConnected
              ? const Color(0xFF46A0AC)
                  .withOpacity(0.3) // Highlight connected network
              : (isSelected
                  ? Colors.white.withOpacity(0.15)
                  : Colors.white.withOpacity(0.05)),
          borderRadius: BorderRadius.circular(20),
          border: isConnected
              ? Border.all(color: const Color(0xFF46A0AC), width: 2)
              : null,
        ),
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Row(
            children: [
              Icon(
                item.icon,
                color: Colors.white,
                size: 28,
              ),
              const SizedBox(width: 20),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      item.title,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.w600,
                        color: Colors.white,
                      ),
                    ),
                    if (isConnected && item.subtitle != null)
                      Text(
                        item.subtitle!(),
                        style: TextStyle(
                          fontSize: 16,
                          color: Colors.white.withOpacity(0.7),
                        ),
                      ),
                  ],
                ),
              ),
              if (item.isSecured ?? false)
                Icon(
                  Icons.lock_outline,
                  color: Colors.white.withOpacity(0.5),
                  size: 20,
                ),
              const SizedBox(width: 12),
              if (isConnected)
                Container(
                  width: 12,
                  height: 12,
                  decoration: const BoxDecoration(
                    color: Colors.white,
                    shape: BoxShape.circle,
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }

  // Build device item with larger touch targets
  Widget _buildDeviceItem() {
    final bool isConnected = item.toggleGetValue?.call() ?? false;

    return GestureDetector(
      onTap: onTap,
      onLongPress: onLongPress,
      child: Container(
        margin: const EdgeInsets.only(bottom: 20),
        decoration: BoxDecoration(
          color: isSelected
              ? Colors.white.withOpacity(0.15)
              : Colors.white.withOpacity(0.05),
          borderRadius: BorderRadius.circular(20),
          border: isConnected
              ? Border.all(color: const Color(0xFF46A0AC), width: 2)
              : null,
        ),
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Row(
            children: [
              Container(
                width: 48,
                height: 48,
                decoration: BoxDecoration(
                  color: isConnected
                      ? const Color(0xFF46A0AC).withOpacity(0.3)
                      : Colors.white.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(14),
                ),
                child: Icon(
                  item.icon,
                  color: isConnected ? Colors.white : Colors.white70,
                  size: 26,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      item.title,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.w600,
                        color: Colors.white,
                      ),
                    ),
                    if (item.subtitle != null)
                      Text(
                        item.subtitle!(),
                        style: TextStyle(
                          fontSize: 16,
                          color: isConnected
                              ? Colors.white.withOpacity(0.9)
                              : Colors.white.withOpacity(0.5),
                        ),
                      ),
                  ],
                ),
              ),
              if (isConnected)
                Container(
                  width: 12,
                  height: 12,
                  decoration: const BoxDecoration(
                    color: Color(0xFF46A0AC),
                    shape: BoxShape.circle,
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}

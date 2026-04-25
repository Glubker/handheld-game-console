// Create a central keyboard manager class
import 'package:flutter/material.dart';

class KeyboardManager {
  static final KeyboardManager _instance = KeyboardManager._internal();
  factory KeyboardManager() => _instance;
  KeyboardManager._internal();

  // Track the current active focus node
  FocusNode? _activeFocusNode;

  // Track if a dialog is currently open
  bool _isDialogOpen = false;

  // Get the current active focus node
  FocusNode? get activeFocusNode => _activeFocusNode;

  // Check if a dialog is open
  bool get isDialogOpen => _isDialogOpen;

  // Set the active focus node
  void setActiveFocusNode(FocusNode? node) {
    if (_activeFocusNode != node) {
      // Unfocus the previous node if it exists
      if (_activeFocusNode != null && _activeFocusNode!.hasFocus) {
        _activeFocusNode!.unfocus();
      }

      _activeFocusNode = node;

      // Focus the new node if it exists
      if (_activeFocusNode != null) {
        _activeFocusNode!.requestFocus();
      }
    }
  }

  // Set dialog state
  void setDialogState(bool isOpen) {
    _isDialogOpen = isOpen;
  }
}

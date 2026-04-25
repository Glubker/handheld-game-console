import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// A custom keyboard widget that can be easily integrated with text fields.
/// Enhanced with Danish characters (Æ, Ø, Å), improved visual design, and keyboard navigation.
class CustomKeyboard extends StatefulWidget {
  /// The controller to update when keys are pressed
  final TextEditingController controller;

  /// Callback when the keyboard should be closed
  final VoidCallback? onClose;

  /// Callback when the return/enter key is pressed
  final VoidCallback? onSubmit;

  /// Whether the keyboard is in dark mode
  final bool isDarkMode;

  /// Primary color for the keyboard
  final Color? primaryColor;

  /// Whether to obscure text (for passwords)
  final bool obscureText;

  /// Title to display at the top of the keyboard
  final String title;

  const CustomKeyboard({
    Key? key,
    required this.controller,
    this.onClose,
    this.onSubmit,
    this.isDarkMode = false,
    this.primaryColor,
    this.obscureText = false,
    this.title = 'Enter Password',
  }) : super(key: key);

  @override
  _CustomKeyboardState createState() => _CustomKeyboardState();
}

class _CustomKeyboardState extends State<CustomKeyboard>
    with SingleTickerProviderStateMixin {
  bool _isShiftEnabled = false;
  bool _isSymbolsEnabled = false;
  bool _isCapsLockEnabled = false;
  String? _pressedKey;
  bool _isKeyboardNavigationActive = false;
  int _focusedRowIndex = 0;
  int _focusedKeyIndex = 0;
  bool _passwordVisible = false;
  late AnimationController _animController;

  final FocusNode _keyboardFocusNode = FocusNode();

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 100),
    );

    // Request focus for keyboard navigation
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _keyboardFocusNode.requestFocus();
    });
  }

  @override
  void dispose() {
    _animController.dispose();
    _keyboardFocusNode.dispose();
    super.dispose();
  }

  // Get colors based on theme
  Color get _backgroundColor =>
      widget.isDarkMode ? const Color(0xFF1A1A1A) : const Color(0xFFE8E8E8);

  Color get _keyColor =>
      widget.isDarkMode ? const Color(0xFF2D2D2D) : const Color(0xFFF5F5F5);

  Color get _specialKeyColor =>
      widget.isDarkMode ? const Color(0xFF3D3D3D) : const Color(0xFFDDDDDD);

  Color get _textColor => widget.isDarkMode ? Colors.white : Colors.black;

  Color get _primaryColor =>
      widget.primaryColor ??
      (widget.isDarkMode ? const Color(0xFF46A0AC) : const Color(0xFF2196F3));

  // Danish keyboard layouts
  final List<List<String>> _lowerCaseLayout = [
    ['q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 'å'],
    ['a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'æ', 'ø'],
    ['shift', 'z', 'x', 'c', 'v', 'b', 'n', 'm', 'backspace'],
    ['symbols', 'space', 'return'],
  ];

  final List<List<String>> _upperCaseLayout = [
    ['Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 'Å'],
    ['A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'Æ', 'Ø'],
    ['shift', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', 'backspace'],
    ['symbols', 'space', 'return'],
  ];

  final List<List<String>> _symbolsLayout = [
    ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0'],
    ['-', '/', ':', ';', '(', ')', '\$', '&', '@', '"'],
    ['symbols2', '.', ',', '?', '!', '\'', '#', '%', 'backspace'],
    ['abc', 'space', 'return'],
  ];

  final List<List<String>> _symbolsLayout2 = [
    ['~', '`', '|', '•', '√', 'π', '÷', '×', '¶', '∆'],
    ['£', '¢', '€', '¥', '^', '°', '=', '{', '}', '\\'],
    ['symbols', '[', ']', '<', '>', '+', '_', '*', 'backspace'],
    ['abc', 'space', 'return'],
  ];

  // Get current layout based on state
  List<List<String>> get _currentLayout {
    if (_isSymbolsEnabled) {
      return _isShiftEnabled ? _symbolsLayout2 : _symbolsLayout;
    } else {
      return _isShiftEnabled || _isCapsLockEnabled
          ? _upperCaseLayout
          : _lowerCaseLayout;
    }
  }

  // Handle key press
  void _onKeyPressed(String key) {
    HapticFeedback.lightImpact();

    // Special handling for return and close button to ensure immediate callback
    if (key == 'return') {
      _handleReturn();
      return;
    }

    setState(() {
      _pressedKey = key;
    });

    _animController.forward().then((_) {
      _animController.reverse();
    });

    Future.delayed(const Duration(milliseconds: 100), () {
      if (mounted) {
        setState(() {
          _pressedKey = null;
        });
      }
    });

    switch (key) {
      case 'backspace':
        _handleBackspace();
        break;
      case 'shift':
        _handleShift();
        break;
      case 'symbols':
        _handleSymbols();
        break;
      case 'symbols2':
        _handleSymbols2();
        break;
      case 'abc':
        _handleAbc();
        break;
      case 'space':
        _handleSpace();
        break;
      // 'return' is handled above
      default:
        _handleCharacter(key);
        break;
    }
  }

  void _handleBackspace() {
    final text = widget.controller.text;
    if (text.isNotEmpty) {
      final selection = widget.controller.selection;
      final int selectionStart = selection.start;
      final int selectionEnd = selection.end;

      if (selectionStart == selectionEnd) {
        // No selection, just delete the previous character
        if (selectionStart > 0) {
          final newText = text.substring(0, selectionStart - 1) +
              text.substring(selectionStart);
          widget.controller.text = newText;
          widget.controller.selection = TextSelection.fromPosition(
            TextPosition(offset: selectionStart - 1),
          );
        }
      } else {
        // Delete the selected text
        final newText =
            text.substring(0, selectionStart) + text.substring(selectionEnd);
        widget.controller.text = newText;
        widget.controller.selection = TextSelection.fromPosition(
          TextPosition(offset: selectionStart),
        );
      }
    }
  }

  void _handleShift() {
    setState(() {
      if (_isShiftEnabled && !_isCapsLockEnabled) {
        // Double tap for caps lock
        _isCapsLockEnabled = true;
      } else if (_isCapsLockEnabled) {
        // Turn off caps lock
        _isCapsLockEnabled = false;
        _isShiftEnabled = false;
      } else {
        // Single tap for shift
        _isShiftEnabled = true;
      }
    });
  }

  void _handleSymbols() {
    setState(() {
      _isSymbolsEnabled = true;
      _isShiftEnabled = false;

      // Reset focus position for new layout
      _focusedRowIndex = 0;
      _focusedKeyIndex = 0;
    });
  }

  void _handleSymbols2() {
    setState(() {
      _isShiftEnabled = true;
    });
  }

  void _handleAbc() {
    setState(() {
      _isSymbolsEnabled = false;
      _isShiftEnabled = false;

      // Reset focus position for new layout
      _focusedRowIndex = 0;
      _focusedKeyIndex = 0;
    });
  }

  void _handleSpace() {
    _insertText(' ');
  }

  void _handleReturn() {
    // Immediately call the onSubmit callback if provided
    if (widget.onSubmit != null) {
      widget.onSubmit!();
    } else {
      _insertText('\n');
    }
  }

  void _handleCharacter(String character) {
    _insertText(character);

    // If shift is enabled but not caps lock, disable shift after character
    if (_isShiftEnabled && !_isCapsLockEnabled) {
      setState(() {
        _isShiftEnabled = false;
      });
    }
  }

  void _insertText(String text) {
    final currentText = widget.controller.text;
    final selection = widget.controller.selection;
    final int selectionStart = selection.start;
    final int selectionEnd = selection.end;

    String newText;
    int newCursorPosition;

    if (selectionStart == selectionEnd) {
      // No selection, just insert text at cursor
      newText = currentText.substring(0, selectionStart) +
          text +
          currentText.substring(selectionStart);
      newCursorPosition = selectionStart + text.length;
    } else {
      // Replace selected text with new text
      newText = currentText.substring(0, selectionStart) +
          text +
          currentText.substring(selectionEnd);
      newCursorPosition = selectionStart + text.length;
    }

    widget.controller.text = newText;
    widget.controller.selection = TextSelection.fromPosition(
      TextPosition(offset: newCursorPosition),
    );
  }

  // Handle keyboard navigation
  void _handleKeyboardNavigation(RawKeyEvent event) {
    if (event is! RawKeyDownEvent) return;

    setState(() {
      _isKeyboardNavigationActive = true;
    });

    final layout = _currentLayout;

    if (event.logicalKey == LogicalKeyboardKey.escape) {
      if (widget.onClose != null) {
        widget.onClose!();
      }
      return;
    }

    if (event.logicalKey == LogicalKeyboardKey.enter ||
        event.logicalKey == LogicalKeyboardKey.select) {
      if (_focusedRowIndex < layout.length &&
          _focusedKeyIndex < layout[_focusedRowIndex].length) {
        _onKeyPressed(layout[_focusedRowIndex][_focusedKeyIndex]);
      }
      return;
    }

    if (event.logicalKey == LogicalKeyboardKey.arrowUp) {
      setState(() {
        _focusedRowIndex = (_focusedRowIndex - 1) % layout.length;
        if (_focusedRowIndex < 0) _focusedRowIndex = layout.length - 1;

        // Make sure key index is valid for the new row
        _focusedKeyIndex =
            _focusedKeyIndex.clamp(0, layout[_focusedRowIndex].length - 1);
      });
    } else if (event.logicalKey == LogicalKeyboardKey.arrowDown) {
      setState(() {
        _focusedRowIndex = (_focusedRowIndex + 1) % layout.length;

        // Make sure key index is valid for the new row
        _focusedKeyIndex =
            _focusedKeyIndex.clamp(0, layout[_focusedRowIndex].length - 1);
      });
    } else if (event.logicalKey == LogicalKeyboardKey.arrowLeft) {
      setState(() {
        _focusedKeyIndex--;
        if (_focusedKeyIndex < 0) {
          _focusedKeyIndex = layout[_focusedRowIndex].length - 1;
        }
      });
    } else if (event.logicalKey == LogicalKeyboardKey.arrowRight) {
      setState(() {
        _focusedKeyIndex =
            (_focusedKeyIndex + 1) % layout[_focusedRowIndex].length;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return RawKeyboardListener(
      focusNode: _keyboardFocusNode,
      onKey: _handleKeyboardNavigation,
      child: Scaffold(
        backgroundColor: _backgroundColor,
        body: SafeArea(
          child: Column(
            children: [
              // Input field
              Container(
                padding: EdgeInsets.all(24), // Increased padding
                color: widget.isDarkMode ? Colors.black : Colors.white,
                child: Container(
                  height: 80, // Increased height
                  padding:
                      EdgeInsets.symmetric(horizontal: 24), // Increased padding
                  decoration: BoxDecoration(
                    color: widget.isDarkMode
                        ? Color(0xFF2D2D2D)
                        : Color(0xFFF5F5F5),
                    borderRadius:
                        BorderRadius.circular(20), // Increased border radius
                    border: Border.all(
                      color: _primaryColor.withOpacity(0.3),
                      width: 2, // Increased border width
                    ),
                  ),
                  child: Row(
                    children: [
                      Expanded(
                        child: Text(
                          widget.obscureText && !_passwordVisible
                              ? '•' * widget.controller.text.length
                              : widget.controller.text,
                          style: TextStyle(
                            color: _textColor,
                            fontSize: 28, // Increased font size
                          ),
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                      if (widget.obscureText)
                        IconButton(
                          icon: Icon(
                            _passwordVisible
                                ? Icons.visibility_off
                                : Icons.visibility,
                            color: _textColor.withOpacity(0.7),
                            size: 32, // Increased icon size
                          ),
                          onPressed: () {
                            setState(() {
                              _passwordVisible = !_passwordVisible;
                            });
                          },
                        ),
                    ],
                  ),
                ),
              ),

              // Status bar
              Container(
                height: 80, // Increased height
                padding: const EdgeInsets.symmetric(horizontal: 24),
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.only(
                    topLeft: Radius.circular(20),
                    topRight: Radius.circular(20),
                  ),
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    // Status indicators
                    Row(
                      children: [
                        if (_isShiftEnabled || _isCapsLockEnabled)
                          Container(
                            padding: const EdgeInsets.symmetric(
                                horizontal: 16,
                                vertical: 8), // Increased padding
                            decoration: BoxDecoration(
                              color: _isCapsLockEnabled
                                  ? _primaryColor
                                  : _primaryColor.withOpacity(0.7),
                              borderRadius: BorderRadius.circular(
                                  16), // Increased border radius
                            ),
                            child: Text(
                              _isCapsLockEnabled ? 'CAPS' : 'SHIFT',
                              style: TextStyle(
                                color: Colors.white,
                                fontSize: 18, // Increased font size
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                        if (_isSymbolsEnabled)
                          Container(
                            margin: const EdgeInsets.only(
                                left: 12), // Increased margin
                            padding: const EdgeInsets.symmetric(
                                horizontal: 16,
                                vertical: 8), // Increased padding
                            decoration: BoxDecoration(
                              color: _primaryColor.withOpacity(0.7),
                              borderRadius: BorderRadius.circular(
                                  16), // Increased border radius
                            ),
                            child: Text(
                              _isShiftEnabled ? 'SYM 2' : 'SYM 1',
                              style: TextStyle(
                                color: Colors.white,
                                fontSize: 18, // Increased font size
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                        if (_isKeyboardNavigationActive)
                          Container(
                            margin: const EdgeInsets.only(
                                left: 12), // Increased margin
                            padding: const EdgeInsets.symmetric(
                                horizontal: 16,
                                vertical: 8), // Increased padding
                            decoration: BoxDecoration(
                              color: Colors.blue.withOpacity(0.7),
                              borderRadius: BorderRadius.circular(
                                  16), // Increased border radius
                            ),
                            child: Text(
                              'NAV',
                              style: TextStyle(
                                color: Colors.white,
                                fontSize: 18, // Increased font size
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                      ],
                    ),

                    // Close button
                    if (widget.onClose != null)
                      Material(
                        color: Colors.transparent,
                        child: InkWell(
                          onTap: widget.onClose,
                          borderRadius: BorderRadius.circular(
                              20), // Increased border radius
                          child: Container(
                            padding:
                                const EdgeInsets.all(12), // Increased padding
                            decoration: BoxDecoration(
                              color: _specialKeyColor,
                              borderRadius: BorderRadius.circular(
                                  20), // Increased border radius
                              boxShadow: [
                                BoxShadow(
                                  color: Colors.black.withOpacity(0.1),
                                  blurRadius: 2,
                                  offset: Offset(0, 1),
                                ),
                              ],
                            ),
                            child: Icon(
                              Icons.keyboard_hide,
                              color: _textColor,
                              size: 32, // Increased icon size
                            ),
                          ),
                        ),
                      ),
                  ],
                ),
              ),

              // Keyboard rows
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.all(12), // Increased padding
                  child: Column(
                    children: _buildKeyboardRows(),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  List<Widget> _buildKeyboardRows() {
    final layout = _currentLayout;
    return List.generate(layout.length, (rowIndex) {
      return Expanded(
        child: Row(
          children: _buildKeyboardRow(layout[rowIndex], rowIndex),
        ),
      );
    });
  }

  List<Widget> _buildKeyboardRow(List<String> row, int rowIndex) {
    return List.generate(row.length, (keyIndex) {
      final key = row[keyIndex];
      return _buildKey(key, rowIndex, keyIndex);
    });
  }

  Widget _buildKey(String keyLabel, int rowIndex, int keyIndex) {
    // Determine key width based on special keys
    double flex = 1;
    if (keyLabel == 'space') {
      flex = 4;
    } else if (keyLabel == 'return') {
      flex = 2;
    } else if (keyLabel == 'backspace' ||
        keyLabel == 'shift' ||
        keyLabel == 'symbols' ||
        keyLabel == 'symbols2' ||
        keyLabel == 'abc') {
      flex = 1.5;
    }

    // Determine if this is a special key
    final isSpecialKey = keyLabel == 'shift' ||
        keyLabel == 'backspace' ||
        keyLabel == 'symbols' ||
        keyLabel == 'symbols2' ||
        keyLabel == 'abc' ||
        keyLabel == 'return';

    // Determine if this key is currently pressed
    final isPressed = _pressedKey == keyLabel;

    // Determine if this key is focused for keyboard navigation
    final isFocused = _isKeyboardNavigationActive &&
        rowIndex == _focusedRowIndex &&
        keyIndex == _focusedKeyIndex;

    // Determine if shift is active for coloring
    final isShiftActive =
        keyLabel == 'shift' && (_isShiftEnabled || _isCapsLockEnabled);

    // Determine if symbols is active for coloring
    final isSymbolsActive =
        (keyLabel == 'symbols' || keyLabel == 'symbols2') && _isSymbolsEnabled;

    // Determine background color
    Color backgroundColor;
    if (isPressed) {
      backgroundColor = _primaryColor;
    } else if (isShiftActive || isSymbolsActive) {
      backgroundColor = _primaryColor.withOpacity(0.7);
    } else if (isSpecialKey) {
      backgroundColor = _specialKeyColor;
    } else {
      backgroundColor = _keyColor;
    }

    // Determine text/icon color
    final textIconColor = isPressed ? Colors.white : _textColor;

    return Expanded(
      flex: flex.toInt(),
      child: Padding(
        padding: const EdgeInsets.all(6), // Increased padding
        child: Material(
          color: Colors.transparent,
          child: InkWell(
            onTap: () => _onKeyPressed(keyLabel),
            borderRadius: BorderRadius.circular(16), // Increased border radius
            child: Ink(
              decoration: BoxDecoration(
                color: backgroundColor,
                borderRadius:
                    BorderRadius.circular(16), // Increased border radius
                border: isFocused
                    ? Border.all(
                        color: Colors.white, width: 3) // Increased border width
                    : null,
                boxShadow: [
                  BoxShadow(
                    color: widget.isDarkMode
                        ? Colors.black.withOpacity(0.3)
                        : Colors.grey.withOpacity(0.3),
                    blurRadius: 3, // Increased blur
                    offset: Offset(0, 2), // Increased offset
                  ),
                ],
              ),
              child: Center(
                child: _buildKeyContent(keyLabel, textIconColor),
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildKeyContent(String keyLabel, Color color) {
    switch (keyLabel) {
      case 'shift':
        return Icon(
          _isCapsLockEnabled ? Icons.lock : Icons.arrow_upward,
          color: color,
          size: 32, // Increased icon size
        );
      case 'backspace':
        return Icon(
          Icons.backspace_outlined,
          color: color,
          size: 32, // Increased icon size
        );
      case 'symbols':
      case 'symbols2':
        return Container(
          width: 50, // Fixed width for consistent sizing
          child: Center(
            child: Text(
              '!#1',
              style: TextStyle(
                color: color,
                fontSize: 24, // Increased font size
                fontWeight: FontWeight.w600,
              ),
            ),
          ),
        );
      case 'abc':
        return Container(
          width: 50, // Fixed width for consistent sizing
          child: Center(
            child: Text(
              'ABC',
              style: TextStyle(
                color: color,
                fontSize: 24, // Increased font size
                fontWeight: FontWeight.w600,
              ),
            ),
          ),
        );
      case 'space':
        return Icon(
          Icons.space_bar,
          color: color,
          size: 32, // Increased icon size
        );
      case 'return':
        return Icon(
          Icons.keyboard_return,
          color: color,
          size: 32, // Increased icon size
        );
      default:
        return Container(
          width: 40, // Fixed width for consistent sizing
          height: 40, // Fixed height for consistent sizing
          child: Center(
            child: Text(
              keyLabel,
              style: TextStyle(
                color: color,
                fontSize: 28, // Increased font size
                fontWeight: FontWeight.w500,
              ),
            ),
          ),
        );
    }
  }
}

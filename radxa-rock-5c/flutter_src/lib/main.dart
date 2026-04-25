import 'dart:async';
import 'dart:io'; // for Platform check
import 'package:window_manager/window_manager.dart';

import 'package:control_panel/models/settings.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'screens/console_home_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  if (Platform.isLinux) {
    await windowManager.ensureInitialized();
    WindowOptions windowOptions = const WindowOptions(
      fullScreen: true,
      backgroundColor: Colors.transparent,
    );
    windowManager.waitUntilReadyToShow(windowOptions, () async {
      await windowManager.show();
      await windowManager.setFullScreen(true);
    });
  }

  SystemChrome.setPreferredOrientations([
    DeviceOrientation.landscapeLeft,
    DeviceOrientation.landscapeRight,
  ]);
  SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);

  runApp(const GameConsoleOS());
}


class GameConsoleOS extends StatefulWidget {
  const GameConsoleOS({super.key});

  @override
  State<GameConsoleOS> createState() => _GameConsoleOSState();
}

class _GameConsoleOSState extends State<GameConsoleOS> {
  final settings = Settings();
  Timer? _inactivityTimer;

  @override
  void initState() {
    super.initState();
    _resetInactivityTimer();
  }

  void _resetInactivityTimer() {
    _inactivityTimer?.cancel();
    _inactivityTimer = Timer(const Duration(seconds: 300), _turnOffScreen);
  }

  void _turnOffScreen() {
    setState(() {
      settings.isScreenBlank = true;
      settings.turnScreenOff();
    });
  }

  void _turnOnScreen() {
    if (settings.isScreenBlank) {
      setState(() {
        settings.isScreenBlank = false;
        settings.turnScreenOn();
      });
    }
    _resetInactivityTimer();
  }

  @override
  void dispose() {
    _inactivityTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Game Console OS',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.dark,
        scaffoldBackgroundColor: Colors.black,
        fontFamily: 'Montserrat',
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFF6200EA),
          secondary: Color(0xFF00BFA5),
          background: Colors.black,
          surface: Color(0xFF121212),
        ),
      ),
      home: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTap: _turnOnScreen,
        onPanDown: (_) => _turnOnScreen(),
        child: Stack(
          children: [
            Scaffold(
              body: MouseRegion(
                //cursor: SystemMouseCursors.none, // Hides the cursor
                onHover: (_) => _turnOnScreen(),
                child: const Center(
                  child: ConsoleHomeScreen(),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

import 'dart:io';
import 'package:path/path.dart' as p;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:ui';
import '../models/game.dart';
import '../utils/painters.dart';
import '../widgets/settings_panel.dart';

class ConsoleHomeScreen extends StatefulWidget {
  const ConsoleHomeScreen({super.key});

  @override
  State<ConsoleHomeScreen> createState() => _ConsoleHomeScreenState();
}

class _ConsoleHomeScreenState extends State<ConsoleHomeScreen>
    with TickerProviderStateMixin, WidgetsBindingObserver {
  List<Game> _games = [];

  // Controllers
  late AnimationController _backgroundAnimController;
  late AnimationController _gameSelectAnimController;
  late AnimationController _settingsPanelController;
  late PageController _gamePageController;

  // Animations
  late Animation<double> _backgroundAnim;
  late Animation<double> _gameSelectAnim;
  late Animation<double> _settingsPanelAnimation;

  // State
  int _selectedGameIndex = 0;
  bool _isSettingsPanelOpen = false;
  DateTime _currentTime = DateTime.now();
  bool _isGameLoading = false;
  int? _loadingGameIndex;

  // Focus nodes for keyboard navigation
  final FocusNode _globalFocusNode = FocusNode();

  @override
  void initState() {
    super.initState();
  WidgetsBinding.instance.addObserver(this);


    // Background animation
    _backgroundAnimController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 30),
    )..repeat(reverse: true);

    _backgroundAnim = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(_backgroundAnimController);

    // Game selection animation
    _gameSelectAnimController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 400),
    );

    _gameSelectAnim = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _gameSelectAnimController,
      curve: Curves.easeOutCubic,
    ));

    // Settings panel animation
    _settingsPanelController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 300),
    );

    _settingsPanelController.addStatusListener((status) {
      if (status == AnimationStatus.dismissed) {
        setState(() {
          _isSettingsPanelOpen = false;
        });
      }
    });

    _settingsPanelAnimation = CurvedAnimation(
      parent: _settingsPanelController,
      curve: Curves.easeInOut,
    );

    // Page controller for games
    _gamePageController = PageController(
      viewportFraction: 0.6,
      initialPage: _selectedGameIndex,
    );

    WidgetsBinding.instance.addPostFrameCallback((_) {
        _globalFocusNode.requestFocus();

    });

    // load games from disk
    Game.loadGames().then((list) {
      setState(() {
        _games = list;
      });
    });

    // Update time every minute
    _updateTimeAndDate();

  }

  @override
  void dispose() {
    _backgroundAnimController.dispose();
    _gameSelectAnimController.dispose();
    _settingsPanelController.dispose();
    _gamePageController.dispose();
    _globalFocusNode.dispose();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
        _globalFocusNode.requestFocus();
    }
  }


  void _updateTimeAndDate() {
    if (mounted) {
      setState(() {
        _currentTime = DateTime.now();
      });
      Future.delayed(
          Duration(seconds: 60 - _currentTime.second), _updateTimeAndDate);
    }
  }

  void _onGamePageChanged(int index) {
    setState(() {
      _selectedGameIndex = index;
      _gameSelectAnimController.reset();
      _gameSelectAnimController.forward();
    });
  }

  void _toggleSettingsPanel() {
    if (_settingsPanelController.status == AnimationStatus.dismissed) {
      setState(() {
        _isSettingsPanelOpen = true;
      });
      _settingsPanelController.forward();
    } else {
      _settingsPanelController.reverse();
    }
  }

  void _closeSettingsPanel() {
    if (_settingsPanelController.status != AnimationStatus.dismissed) {
      _settingsPanelController.reverse();
      // Re-focus global key listener
      _globalFocusNode.requestFocus();
    }
  }

  void _launchGame(int index) async {
    final game = _games[index];

    setState(() {
      _isGameLoading = true;
      _loadingGameIndex = index;
    });

    // Base paths
    const launcher = '/home/rock/console/bin/launcher.sh';
    const gamesBase = '/home/rock/console/games';

    // Build the directory & exec paths using path.join
    final gameDir = p.join(gamesBase, game.id);
    final execName =
        game.exec.startsWith('./') ? game.exec.substring(2) : game.exec;
    final gamePath = p.join(gameDir, execName);

    // Debug logging
    print('> _launchGame called for index $index');
    print('> Game ID: "${game.id}"');
    print('> Game directory: $gameDir');
    print('> Exec name: $execName');
    print('> Full gamePath: $gamePath');
    print('> launcher script: $launcher');

    // Existence checks
    final dirExists = await Directory(gameDir).exists();
    final fileExists = await File(gamePath).exists();
    print('> Directory exists? $dirExists');
    print('> Exec file exists? $fileExists');

    if (!dirExists) {
      print('ERROR: Game folder not found: $gameDir');
      return;
    }
    if (!fileExists) {
      print('ERROR: Game executable not found: $gamePath');
      return;
    }

    try {
      final proc = await Process.start(
        launcher,
        ['game', gamePath],
        mode: ProcessStartMode.detached,
        runInShell: true,
      );
      print('> _launchGame: Launched PID ${proc.pid}');
    } catch (e, st) {
      print('> _launchGame ERROR: $e\n$st');
    }
  }

  void _handleKeyEvent(RawKeyEvent event) {
  if (event is RawKeyDownEvent) {
    // Escape should always work
    if (event.logicalKey == LogicalKeyboardKey.escape) {
      if (_isSettingsPanelOpen) {
        _closeSettingsPanel();
        print('Escape pressed, closing settings panel');
      } else {
        _toggleSettingsPanel();
        print('Escape pressed, toggling settings panel');
      }
      return; // stop further handling
    }

    // Main screen navigation
    if (!_isSettingsPanelOpen) {
      if (event.logicalKey == LogicalKeyboardKey.arrowLeft) {
        _navigateToGame(_selectedGameIndex - 1);
      } else if (event.logicalKey == LogicalKeyboardKey.arrowRight) {
        _navigateToGame(_selectedGameIndex + 1);
      } else if (event.logicalKey == LogicalKeyboardKey.enter ||
          event.logicalKey == LogicalKeyboardKey.space) {
        if (!_isGameLoading) {
          _launchGame(_selectedGameIndex);
        }
      }
    }
  }
}


  void _navigateToGame(int index) {
    if (index >= 0 && index < _games.length) {
      _gamePageController.animateToPage(
        index,
        duration: const Duration(milliseconds: 300),
        curve: Curves.easeOutCubic,
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          // Animated background
          _buildAnimatedBackground(),

          // Main content with keyboard focus
          RawKeyboardListener(
            focusNode: _globalFocusNode,
            onKey: _handleKeyEvent,
            autofocus: true,
            child: Stack(
              children: [
                _buildAnimatedBackground(),
                SafeArea(
                  child: Column(
                    children: [
                      _buildTopBar(),
                      Expanded(child: _buildGameCarousel()),
                      const SizedBox(height: 20),
                    ],
                  ),
                ),
                if (_isSettingsPanelOpen || _settingsPanelController.isAnimating)
                  Positioned.fill(
                    child: AbsorbPointer(
                      absorbing: _settingsPanelAnimation.value > 0.0,
                      child: Container(), // Transparent layer to block taps
                    ),
                  ),
                if (_isSettingsPanelOpen || _settingsPanelController.isAnimating)
                  Positioned.fill(
                    child: AnimatedBuilder(
                      animation: _settingsPanelAnimation,
                      builder: (context, child) {
                        final animationValue = _settingsPanelAnimation.value;
                        return Opacity(
                          opacity: animationValue,
                          child: Stack(
                            children: [
                              GestureDetector(
                                onTap: _closeSettingsPanel,
                                child: BackdropFilter(
                                  filter: ImageFilter.blur(
                                    sigmaX: 4.0 * animationValue,
                                    sigmaY: 4.0 * animationValue,
                                  ),
                                  child: Container(
                                    color: Colors.black.withOpacity(1 * animationValue),
                                  ),
                                ),
                              ),
                              Center(
                                child: Transform.scale(
                                  scale: 0.95 + 0.05 * animationValue,
                                  child: FocusScope(
                                    autofocus: true,
                                    child: SettingsPanel(
                                      onClose: _closeSettingsPanel,
                                    ),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        );
                      },
                    ),
                  ),

              ],
            ),
          )

        ],
      ),
    );
  }

  Widget _buildAnimatedBackground() {
    return AnimatedBuilder(
      animation: _backgroundAnim,
      builder: (context, child) {
        return Container(
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: [
                const Color(0xFF0A0A0A),
                const Color(0xFF1A1A1A),
                const Color(0xFF0A0A0A),
              ],
              stops: [
                0.0,
                _backgroundAnim.value,
                1.0,
              ],
            ),
          ),
          child: CustomPaint(
            painter: GridPainter(
              progress: _backgroundAnim.value,
              gridColor: Colors.white.withOpacity(0.05),
            ),
            child: Container(),
          ),
        );
      },
    );
  }

  Widget _buildTopBar() {
    return Padding(
      padding: const EdgeInsets.fromLTRB(40, 20, 40, 20),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          // Time and date
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                '${_currentTime.hour.toString().padLeft(2, '0')}:${_currentTime.minute.toString().padLeft(2, '0')}',
                style: const TextStyle(
                  fontSize: 36,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              Text(
                '${_getDayOfWeek(_currentTime.weekday)}, ${_currentTime.day} ${_getMonth(_currentTime.month)}',
                style: TextStyle(
                  fontSize: 24,
                  color: Colors.white.withOpacity(0.7),
                ),
              ),
            ],
          ),

          // Settings button
          GestureDetector(
            onTap: _toggleSettingsPanel,
            child: Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: _isSettingsPanelOpen
                    ? Colors.white.withOpacity(0.15)
                    : Colors.white.withOpacity(0.1),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(
                Icons.settings,
                color: _isSettingsPanelOpen
                    ? Colors.white
                    : Colors.white.withOpacity(0.8),
                size: 36,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildGameCarousel() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // Game carousel
        Expanded(
          child: PageView.builder(
            controller: _gamePageController,
            itemCount: _games.length,
            onPageChanged: _onGamePageChanged,
            itemBuilder: (context, index) {
              final bool isSelected = index == _selectedGameIndex;
              final Game game = _games[index];
              final bool isLoading =
                  _isGameLoading && _loadingGameIndex == index;

              return AnimatedScale(
                scale: isSelected ? 1.0 : 0.85,
                duration: const Duration(milliseconds: 300),
                curve: Curves.easeOutQuint,
                child: AnimatedOpacity(
                  opacity: isSelected ? 1.0 : 0.6,
                  duration: const Duration(milliseconds: 300),
                  child: GestureDetector(
                    onTap: () {
                      if (!isSelected) {
                        _gamePageController.animateToPage(
                          index,
                          duration: const Duration(milliseconds: 500),
                          curve: Curves.easeOutCubic,
                        );
                      } else if (!isLoading) {
                        _launchGame(index);
                      }
                    },
                    child: _buildGameCard(game, isSelected, isLoading),
                  ),
                ),
              );
            },
          ),
        ),

        SizedBox(
          height: 48,
        ),
      ],
    );
  }

  Widget _buildGameCard(Game game, bool isSelected, bool isLoading) {
    return Container(
      margin: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            game.accentColor.withOpacity(0.8),
            game.accentColor.withOpacity(0.4),
          ],
        ),
        borderRadius: BorderRadius.circular(30),
        boxShadow: isSelected
            ? [
                BoxShadow(
                  color: game.accentColor.withOpacity(0.5),
                  blurRadius: 16,
                  spreadRadius: 0,
                  offset: const Offset(0, 5),
                )
              ]
            : [],
      ),
      child: ClipRRect(
        borderRadius: BorderRadius.circular(30),
        child: Stack(
          children: [
            // Background pattern
            Positioned.fill(
              child: CustomPaint(
                painter: HexagonPatternPainter(
                  color: Colors.white.withOpacity(0.05),
                  size: 40,
                ),
              ),
            ),

            // Game content
            Padding(
              padding: const EdgeInsets.all(30),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Spacer(),

                  // Game title
                  Text(
                    game.title,
                    style: const TextStyle(
                      fontSize: 32,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(height: 8),

                  // Game genre
                  Container(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 16,
                      vertical: 8,
                    ),
                    decoration: BoxDecoration(
                      color: Colors.black.withOpacity(0.3),
                      borderRadius: BorderRadius.circular(20),
                    ),
                    child: Text(
                      game.genre.toUpperCase(),
                      style: const TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                      ),
                    ),
                  ),
                ],
              ),
            ),

            // Play button or loading indicator
            if (isSelected)
              Positioned(
                right: 30,
                bottom: 30,
                child: isLoading
                    ? _buildLoadingIndicator(game.accentColor)
                    : _buildPlayButton(game.accentColor),
              ),

            // Loading overlay
            if (isLoading)
              Positioned.fill(
                child: Container(
                  decoration: BoxDecoration(
                    color: Colors.black.withOpacity(0.5),
                    borderRadius: BorderRadius.circular(30),
                  ),
                  child: Center(
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        SizedBox(
                          width: 200,
                          child: ClipRRect(
                            borderRadius: BorderRadius.circular(10),
                            child: LinearProgressIndicator(
                              backgroundColor: Colors.white10,
                              valueColor:
                                  AlwaysStoppedAnimation<Color>(Colors.white),
                              minHeight: 6,
                            ),
                          ),
                        ),
                        const SizedBox(height: 16),
                        const Text(
                          'LAUNCHING GAME...',
                          style: TextStyle(
                            fontSize: 14,
                            fontWeight: FontWeight.w600,
                            letterSpacing: 2,
                            color: Colors.white,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }

  Widget _buildPlayButton(Color accentColor) {
    return Container(
      width: 60,
      height: 60,
      decoration: BoxDecoration(
        color: Colors.white,
        shape: BoxShape.circle,
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.3),
            blurRadius: 10,
            spreadRadius: 0,
            offset: const Offset(0, 5),
          ),
        ],
      ),
      child: Icon(
        Icons.play_arrow,
        color: accentColor,
        size: 30,
      ),
    );
  }

  Widget _buildLoadingIndicator(Color accentColor) {
    return Container(
      width: 60,
      height: 60,
      decoration: BoxDecoration(
        color: Colors.white,
        shape: BoxShape.circle,
      ),
      child: Padding(
        padding: const EdgeInsets.all(15.0),
        child: CircularProgressIndicator(
          strokeWidth: 3,
          valueColor: AlwaysStoppedAnimation<Color>(accentColor),
        ),
      ),
    );
  }

  String _getDayOfWeek(int day) {
    switch (day) {
      case 1:
        return 'Monday';
      case 2:
        return 'Tuesday';
      case 3:
        return 'Wednesday';
      case 4:
        return 'Thursday';
      case 5:
        return 'Friday';
      case 6:
        return 'Saturday';
      case 7:
        return 'Sunday';
      default:
        return '';
    }
  }

  String _getMonth(int month) {
    switch (month) {
      case 1:
        return 'Jan';
      case 2:
        return 'Feb';
      case 3:
        return 'Mar';
      case 4:
        return 'Apr';
      case 5:
        return 'May';
      case 6:
        return 'Jun';
      case 7:
        return 'Jul';
      case 8:
        return 'Aug';
      case 9:
        return 'Sep';
      case 10:
        return 'Oct';
      case 11:
        return 'Nov';
      case 12:
        return 'Dec';
      default:
        return '';
    }
  }
}

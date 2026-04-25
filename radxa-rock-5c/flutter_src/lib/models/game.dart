import 'dart:io';
import 'dart:convert';
import 'package:path/path.dart' as p;
import 'package:flutter/material.dart';

class Game {
  final String id;
  final String title;
  final String genre;
  final Color accentColor;
  final String exec;

  Game({
    required this.id,
    required this.title,
    required this.genre,
    required this.accentColor,
    required this.exec,
  });

  static Future<List<Game>> loadGames() async {
    final gamesDir = Directory('/home/rock/console/games');
    if (!await gamesDir.exists()) return [];

    final games = <Game>[];

    for (final entry in gamesDir.listSync()) {
      if (entry is Directory) {
        // Use basename() to strip any trailing slash
        final id = p.basename(entry.path);
        final manifestFile = File(p.join(entry.path, 'game.json'));
        if (!await manifestFile.exists()) continue;

        final Map<String, dynamic> jsonData =
            jsonDecode(await manifestFile.readAsString());

        final title = jsonData['name'] as String;
        final genre = jsonData['genre'] as String;
        final colorHex = jsonData['color'] as String;
        final exec = jsonData['exec'] as String;

        // Parse hex "#RRGGBB" into a Color
        final hex = colorHex.replaceFirst('#', '');
        final color = Color(int.parse('FF$hex', radix: 16));

        games.add(Game(
          id: id,
          title: title,
          genre: genre,
          accentColor: color,
          exec: exec,
        ));
      }
    }

    return games;
  }
}

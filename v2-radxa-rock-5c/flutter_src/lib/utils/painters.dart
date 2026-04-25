import 'package:flutter/material.dart';
import 'dart:math' as math;

class GridPainter extends CustomPainter {
  final double progress;
  final Color gridColor;

  GridPainter({
    required this.progress,
    required this.gridColor,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = gridColor
      ..strokeWidth = 1;

    final cellSize = 50.0;
    final xCount = (size.width / cellSize).ceil() + 1;
    final yCount = (size.height / cellSize).ceil() + 1;

    // Draw horizontal lines
    for (int i = 0; i < yCount; i++) {
      final y = i * cellSize;
      canvas.drawLine(
        Offset(0, y),
        Offset(size.width, y),
        paint,
      );
    }

    // Draw vertical lines
    for (int i = 0; i < xCount; i++) {
      final x = i * cellSize;
      canvas.drawLine(
        Offset(x, 0),
        Offset(x, size.height),
        paint,
      );
    }
  }

  @override
  bool shouldRepaint(covariant GridPainter oldDelegate) {
    return oldDelegate.progress != progress;
  }
}

class HexagonPatternPainter extends CustomPainter {
  final Color color;
  final double size;

  HexagonPatternPainter({
    required this.color,
    required this.size,
  });

  @override
  void paint(Canvas canvas, Size canvasSize) {
    final paint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;

    final double hexHeight = size * math.sqrt(3);
    final double hexWidth = size * 2;
    
    final int rows = (canvasSize.height / hexHeight).ceil() + 1;
    final int cols = (canvasSize.width / (hexWidth * 0.75)).ceil() + 1;

    for (int r = -1; r < rows; r++) {
      for (int c = -1; c < cols; c++) {
        final double centerX = c * hexWidth * 0.75;
        final double centerY = r * hexHeight + (c % 2 == 0 ? 0 : hexHeight / 2);
        
        final Path hexPath = Path();
        for (int i = 0; i < 6; i++) {
          final double angle = (i * 60) * math.pi / 180;
          final double x = centerX + size * math.cos(angle);
          final double y = centerY + size * math.sin(angle);
          
          if (i == 0) {
            hexPath.moveTo(x, y);
          } else {
            hexPath.lineTo(x, y);
          }
        }
        hexPath.close();
        
        canvas.drawPath(hexPath, paint);
      }
    }
  }

  @override
  bool shouldRepaint(covariant HexagonPatternPainter oldDelegate) {
    return oldDelegate.color != color || oldDelegate.size != size;
  }
}

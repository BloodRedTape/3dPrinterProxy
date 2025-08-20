import 'package:flutter_material_design_icons/flutter_material_design_icons.dart';
import 'package:frontend_flutter/device.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';

class Preview extends StatelessWidget {
  final double maxHeight;
  final String filenameOrHash;
  final IconData fallbackIcon;
  final Color backgroundColor;

  const Preview(
    this.maxHeight,
    this.filenameOrHash, {
    this.fallbackIcon = MdiIcons.printer3d, // default icon if not provided
    this.backgroundColor = const Color.fromARGB(255, 214, 214, 214), // default grey background
    super.key,
  });

  @override
  Widget build(BuildContext context) {
    final String device = 'ttb_1';
    final String url = '${getOrigin()}/api/v1/printers/$device/files/$filenameOrHash/preview';

    final image = Image.network(
      url,
      fit: BoxFit.contain,
      // Placeholder while loading
      loadingBuilder: (BuildContext context, Widget child, ImageChunkEvent? loadingProgress) {
        if (loadingProgress == null) {
          return child; // fully loaded
        }
        return _buildPlaceholder(icon: Icons.image_search, context: context);
      },
      // Fallback if failed
      errorBuilder: (BuildContext context, Object error, StackTrace? stackTrace) {
        return _buildPlaceholder(icon: fallbackIcon, context: context);
      },
    );

    return Card(
      padding: EdgeInsets.all(0),
      child: SizedBox(height: maxHeight, child: filenameOrHash.length != 0 ? image : _buildPlaceholder(icon: fallbackIcon, context: context)),
    );
  }

  Widget _buildPlaceholder({required IconData icon, required BuildContext context}) {
    return Container(width: maxHeight, height: maxHeight, color: backgroundColor, child: Center(child: Icon(icon, size: maxHeight * 0.5, color: Colors.white)));
  }
}

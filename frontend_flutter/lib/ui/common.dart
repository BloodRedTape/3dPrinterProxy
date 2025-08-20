import 'package:flutter/cupertino.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';

class MessageCardContent extends StatelessWidget {
  final String message;
  const MessageCardContent(this.message, {super.key});

  @override
  Widget build(BuildContext context) {
    return Padding(padding: EdgeInsets.all(16), child: Center(child: Text(message)));
  }
}

class PrinterCard extends StatelessWidget {
  final Widget child;
  final String? title;

  const PrinterCard({super.key, this.title, required this.child});

  @override
  Widget build(BuildContext context) {
    return Card(
      padding: EdgeInsets.all(0),
      child: Column(
        children: [title != null ? AppBar(title: Text(title!), backgroundColor: Theme.of(context).colorScheme.card.withLuminance(0.98)) : SizedBox(), child],
      ),
    );
  }
}

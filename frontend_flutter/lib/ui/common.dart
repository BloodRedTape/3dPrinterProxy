import 'package:flutter/cupertino.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';

class ExpandedColumn extends StatelessWidget {
  final List<Widget> children;
  final MainAxisAlignment mainAxisAlignment;
  final CrossAxisAlignment crossAxisAlignment;
  final MainAxisSize mainAxisSize;

  const ExpandedColumn({
    Key? key,
    required this.children,
    this.mainAxisAlignment = MainAxisAlignment.start,
    this.crossAxisAlignment = CrossAxisAlignment.center,
    this.mainAxisSize = MainAxisSize.max,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisAlignment: mainAxisAlignment,
      crossAxisAlignment: crossAxisAlignment,
      mainAxisSize: mainAxisSize,
      children: children.map((child) => Expanded(child: child, flex: 2)).toList(),
    );
  }
}

class ExpandedRow extends StatelessWidget {
  final List<Widget> children;
  final MainAxisAlignment mainAxisAlignment;
  final CrossAxisAlignment crossAxisAlignment;
  final MainAxisSize mainAxisSize;

  const ExpandedRow({
    Key? key,
    required this.children,
    this.mainAxisAlignment = MainAxisAlignment.start,
    this.crossAxisAlignment = CrossAxisAlignment.center,
    this.mainAxisSize = MainAxisSize.max,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: mainAxisAlignment,
      crossAxisAlignment: crossAxisAlignment,
      mainAxisSize: mainAxisSize,
      children: children.map((child) => Expanded(child: child, flex: 2)).toList(),
    );
  }
}

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
      child: Flex(
        direction: Axis.vertical,
        children: [title != null ? AppBar(title: Text(title!), backgroundColor: Theme.of(context).colorScheme.card.withLuminance(0.98)) : SizedBox(), child],
      ),
    );
  }
}

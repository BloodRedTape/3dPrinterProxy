import 'package:flutter/cupertino.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
export 'package:flutter_bloc/flutter_bloc.dart';

class Cubit2<StateType> extends Cubit<StateType> {
  Cubit2(super.initial);
}

abstract class BlocWidget<StateType> extends StatelessWidget {
  final Cubit<StateType> cubit;

  const BlocWidget(this.cubit, {super.key});

  @override
  Widget build(BuildContext context) {
    return BlocProvider(
      create: (context) {
        return cubit;
      },
      child: BlocBuilder<Cubit<StateType>, StateType>(builder: (context, state) => buildFromState(context, state)),
    );
  }

  Widget buildFromState(BuildContext context, StateType state);

  void emit(StateType state) {
    // ignore: invalid_use_of_protected_member
    cubit.emit(state);
  }
}

abstract class CubitWidget<CubitType extends Cubit<StateType>, StateType> extends BlocWidget<StateType> {
  const CubitWidget(CubitType cubit, {super.key}) : super(cubit);

  CubitType getCubit() {
    return cubit as CubitType;
  }
}

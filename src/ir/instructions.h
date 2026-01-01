enum Opcode {
    // Stack
    PushConst,
    Pop,

    // Locals
    LoadLocal,
    StoreLocal,

    // Arithmetic
    AddI32,
    SubI32,
    MulI32,
    DivI32,

    // Comparison
    CmpEqlI32,
    CmpLtI32,

    // Control flow
    Jump,
    JumpIfFalse,

    // Side effects
    PrintString,

    //Structural
    Nop
};
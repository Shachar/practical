#include "ast.h"

PracticalSemanticAnalyzer::ExpressionId::Allocator<> expressionIdAllocator;
PracticalSemanticAnalyzer::ExpressionId voidExpressionId;

void AST::prepare()
{
    // Register the built-in types
    globalCtx.registerBuiltInType("Void", BuiltInType::Type::Void, 0);
    voidExpressionId = expressionIdAllocator.allocate();

    globalCtx.registerBuiltInType("S8", BuiltInType::Type::SignedInt, 8);
    globalCtx.registerBuiltInType("S16", BuiltInType::Type::SignedInt, 16);
    globalCtx.registerBuiltInType("S32", BuiltInType::Type::SignedInt, 32);
    globalCtx.registerBuiltInType("S64", BuiltInType::Type::SignedInt, 64);
    globalCtx.registerBuiltInType("S128", BuiltInType::Type::SignedInt, 128);

    globalCtx.registerBuiltInType("Bool", BuiltInType::Type::Boolean, 1);

    globalCtx.registerBuiltInType("U8", BuiltInType::Type::UnsignedInt, 8);
    globalCtx.registerBuiltInType("U16", BuiltInType::Type::UnsignedInt, 16);
    globalCtx.registerBuiltInType("U32", BuiltInType::Type::UnsignedInt, 32);
    globalCtx.registerBuiltInType("U64", BuiltInType::Type::UnsignedInt, 64);
    globalCtx.registerBuiltInType("U128", BuiltInType::Type::UnsignedInt, 128);

    // Types for internal use only
    globalCtx.registerBuiltInType("__U7", BuiltInType::Type::InternalUnsignedInt, 7);
    globalCtx.registerBuiltInType("__U15", BuiltInType::Type::InternalUnsignedInt, 15);
    globalCtx.registerBuiltInType("__U31", BuiltInType::Type::InternalUnsignedInt, 31);
    globalCtx.registerBuiltInType("__U63", BuiltInType::Type::InternalUnsignedInt, 63);
    globalCtx.registerBuiltInType("__U127", BuiltInType::Type::InternalUnsignedInt, 127);
}

void AST::parseModule(String moduleSource) {
    auto module = safenew<NonTerminals::Module>(&globalCtx);
    module->parse(moduleSource);

    module->symbolsPass1(&globalCtx);
    module->symbolsPass2(&globalCtx);

    modules.emplace(toSlice("Dummy module"), std::move(module));
}

void AST::codeGen(PracticalSemanticAnalyzer::ModuleGen *codeGen) {
    // XXX We should only code-gen some of the modules?
    for(auto &module: modules) {
        module.second->codeGen(codeGen);
    }
}
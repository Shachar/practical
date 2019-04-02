/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * This file is file is copyright (C) 2018-2019 by its authors.
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include "ast_nodes.h"

#include "ast.h"
#include "casts.h"
#include "practical-errors.h"

#include "asserts.h"

using PracticalSemanticAnalyzer::CannotTakeValueOfFunction;

namespace AST {

Type::Type(const NonTerminals::Type *nt) : parseType(nt) {
}

void Type::symbolsPass2(const LookupContext *ctx) {
    auto type = ctx->lookupType(parseType->type.getName());
    if( type==nullptr )
        throw pass2_error("Type name expected", parseType->getLine(), parseType->getCol());

    staticType.setId( type->id() );
}

CompoundExpression::CompoundExpression(LookupContext *parentCtx, const NonTerminals::CompoundExpression *_parserExpresison)
    : ctx(parentCtx), parserExpression(_parserExpresison)
{
}

void CompoundExpression::symbolsPass1() {
    // TODO implement
}

void CompoundExpression::symbolsPass2() {
    // TODO implement
}

ExpressionId CompoundExpression::codeGen(FunctionGen *codeGen, ExpectedType expectedResult) {
    for( auto &statement: parserExpression->statementList.statements ) {
        codeGenStatement( codeGen, &statement );
    }

    return codeGenExpression( codeGen, expectedResult, &parserExpression->expression );
}

void CompoundExpression::codeGenStatement(FunctionGen *codeGen, const NonTerminals::Statement *statement) {

    struct Visitor {
        CompoundExpression *_this;
        FunctionGen *codeGen;

        void operator()(const NonTerminals::Expression &expression) {
            _this->codeGenExpression(codeGen, ExpectedType(), &expression); // Disregard return value
        }

        void operator()(const NonTerminals::VariableDefinition &definition) {
            _this->codeGenVarDef(codeGen, &definition);
        }

        void operator()(std::monostate mono) {
            ABORT() << "Codegen called on unparsed expression";
        }
    } visitor = { this, codeGen };

    std::visit( visitor, statement->content );
}

ExpressionId CompoundExpression::codeGenExpression(
        FunctionGen *codeGen, ExpectedType expectedResult, const NonTerminals::Expression *expression)
{
    using namespace NonTerminals;

    struct Visitor {
        CompoundExpression *_this;
        FunctionGen *codeGen;
        ExpectedType expectedResult;

        ExpressionId operator()( std::monostate none ) const {
            return voidExpressionId;
        }

        ExpressionId operator()( const std::unique_ptr<NonTerminals::CompoundExpression> &compound ) const {
            ABORT() << "TODO implement";
            /*
            return std::get< static_cast<unsigned int>(Expression::ExpressionType::CompoundExpression) >(expression->value)->
                    codeGen(functionGen);
            */
        }

        ExpressionId operator()( const NonTerminals::Literal &literal ) const {
            return _this->codeGenLiteral( codeGen, expectedResult, &literal );
        }

        ExpressionId operator()( const NonTerminals::Identifier &identifier ) const {
            return _this->codeGenIdentifierLookup(codeGen, expectedResult, identifier.identifier);
        }

        ExpressionId operator()( const NonTerminals::Expression::UnaryOperator &op ) const {
            return _this->codeGenUnaryOperator(codeGen, expectedResult, op );
        }

        ExpressionId operator()( const NonTerminals::Expression::BinaryOperator &op ) const {
            return _this->codeGenBinaryOperator(codeGen, expectedResult, op );
        }

        ExpressionId operator()( const NonTerminals::Expression::FunctionCall &functionCall ) const {
            return _this->codeGenFunctionCall(codeGen, expectedResult, &functionCall);
        }
    };

    Visitor visitor = { this, codeGen, expectedResult };
    return std::visit( visitor, expression->value );
}

void CompoundExpression::codeGenVarDef(FunctionGen *codeGen, const NonTerminals::VariableDefinition *definition) {
    Type varType( &definition->body.type );
    varType.symbolsPass2( &ctx );
    const LookupContext::LocalVariable *namedVar = ctx.registerVariable( LookupContext::LocalVariable(
			    definition->body.name.identifier, std::move(varType).removeType(), expressionIdAllocator.allocate() ) );
    codeGen->allocateStackVar( namedVar->lvalueId, namedVar->type, namedVar->name->text );

    if( definition->initValue ) {
        ExpressionId initValueId = codeGenExpression(codeGen, ExpectedType( &namedVar->type ), definition->initValue.get());
        codeGen->assign( namedVar->lvalueId, initValueId );
    } else {
        ABORT() << "TODO Type default values not yet implemented";
    }
}

ExpressionId CompoundExpression::codeGenLiteral(
        FunctionGen *codeGen, ExpectedType expectedResult, const NonTerminals::Literal *literal)
{
    String text = literal->token.text;

    ExpressionId id = expressionIdAllocator.allocate();

    LongEnoughInt res = 0;

    switch( literal->token.token ) {
    case Tokenizer::Tokens::LITERAL_INT_10:
        for( char c: text ) {
            // TODO check range and assign type
            if( c=='_' )
                continue;

            ASSERT( c>='0' && c<='9' ) << "Decimal literal has character '"<<c<<"' out of allowed range";
            res *= 10;
            res += c-'0';
        }
        break;
    default:
        ABORT() << "TODO implement";
    }

    // XXX use value range propagation instead
    StaticType resType( AST::AST::deductLiteralRange(res) );
    bool useExpected = false;
    if( expectedResult ) {
        useExpected = checkImplicitCastAllowed(id, resType, expectedResult, literal->token);
    }

    codeGen->setLiteral(id, res, useExpected ? *expectedResult.type : resType);

    return id;
}

ExpressionId CompoundExpression::codeGenIdentifierLookup(
        FunctionGen *codeGen, ExpectedType expectedResult, const Tokenizer::Token *identifier)
{
    const LookupContext::NamedObject *referencedObject = ctx.lookupIdentifier( identifier->text );
    if( referencedObject==nullptr )
        throw SymbolNotFound(identifier->text, identifier->line, identifier->col);

    struct Visitor {
        const Tokenizer::Token *identifier;
        FunctionGen *codeGen;
        ExpectedType expectedResult;

        ExpressionId operator()(const LookupContext::LocalVariable &localVar) const {
            ExpressionId expId = expressionIdAllocator.allocate();
            codeGen->dereferencePointer( expId, localVar.type, localVar.lvalueId );

            if( expectedResult )
                expId = codeGenCast( codeGen, expId, localVar.type, expectedResult, *identifier, true );

            return expId;
        }

        ExpressionId operator()(const LookupContext::Function &function) const {
            throw CannotTakeValueOfFunction(identifier);
        }
    };

    return std::visit(
            Visitor{ .identifier=identifier, .codeGen=codeGen, .expectedResult=expectedResult }, *referencedObject );
}

ExpressionId CompoundExpression::codeGenUnaryOperator(
        FunctionGen *codeGen, ExpectedType expectedResult, const NonTerminals::Expression::UnaryOperator &op)
{
    ASSERT( op.op!=nullptr ) << "Operator codegen called with no operator";

    using namespace Tokenizer;
    switch( op.op->token ) {
    case Tokens::OP_PLUS:
        return codeGenExpression(codeGen, expectedResult, op.operand.get());
    default:
        ABORT() << "Code generation for unimplemented unary operator "<<op.op->token<<" at "<<op.op->line<<":"<<op.op->col<<"\n";
    }
}

ExpressionId CompoundExpression::codeGenBinaryOperator(
        FunctionGen *codeGen, ExpectedType expectedResult, const NonTerminals::Expression::BinaryOperator &op)
{
    ASSERT( op.op!=nullptr ) << "Operator codegen called with no operator";

    using namespace Tokenizer;
    switch( op.op->token ) {
    default:
        ABORT() << "Code generation for unimplemented binary operator "<<op.op->token<<" at "<<op.op->line<<":"<<op.op->col<<"\n";
    }
}

ExpressionId CompoundExpression::codeGenFunctionCall(
        FunctionGen *codeGen, ExpectedType expectedResult, const NonTerminals::Expression::FunctionCall *functionCall)
{
    const NonTerminals::Identifier *functionName = std::get_if<NonTerminals::Identifier>( & functionCall->expression->value );
    ASSERT( functionName != nullptr ) << "TODO support function calls not by means of direct call";

    const LookupContext::NamedObject *functionObject = ctx.lookupIdentifier( functionName->identifier->text );
    if( functionObject==nullptr ) {
        throw SymbolNotFound(
                functionName->identifier->text, functionName->identifier->line, functionName->identifier->col);
    }

    const LookupContext::Function *function = std::get_if< LookupContext::Function >( functionObject );
    if( function==nullptr ) {
        throw TryToCallNonCallable( functionName->identifier );
    }

    std::vector<ExpressionId> arguments;
    arguments.reserve( function->arguments.size() );

    for( unsigned int argNum=0; argNum<function->arguments.size(); ++argNum ) {
        arguments.emplace_back( codeGenExpression(
                    codeGen, ExpectedType( &function->arguments[argNum].type ), &functionCall->arguments.arguments[argNum] ) );
    }

    ExpressionId functionRet = expressionIdAllocator.allocate();
    codeGen->callFunctionDirect(
            functionRet, functionName->identifier->text, Slice<const ExpressionId>(arguments), function->returnType );

    if( !expectedResult )
        return functionRet;

    return codeGenCast( codeGen, functionRet, function->returnType, expectedResult, *functionName->identifier, true );
}

FuncDecl::FuncDecl(const NonTerminals::FuncDeclBody *nt, LookupContext::Function *function)
    : parserFuncDecl(nt), ctxFunction(function)
{
}

void FuncDecl::symbolsPass1(LookupContext *ctx) {
    // TODO implement ?
}

void FuncDecl::symbolsPass2(LookupContext *ctx) {
    Type retType(&parserFuncDecl->returnType.type);
    retType.symbolsPass2(ctx);
    ctxFunction->returnType = std::move(retType).removeType();

    ctxFunction->arguments.reserve( parserFuncDecl->arguments.arguments.size() );
    for( const auto &parserArg : parserFuncDecl->arguments.arguments ) {
        Type argumentType(&parserArg.type);
        // Function argument types are looked up in the context of the function's parent
        argumentType.symbolsPass2( ctx->getParent() );
        ExpressionId lvalueExpression = expressionIdAllocator.allocate();
        const LookupContext::LocalVariable *localVariable = ctx->registerVariable(
                LookupContext::LocalVariable(
                    parserArg.name.identifier, std::move(argumentType).removeType(), lvalueExpression ) );
        ctxFunction->arguments.emplace_back( localVariable->type, localVariable->name->text, localVariable->lvalueId );
    }
}

FuncDef::FuncDef(const NonTerminals::FuncDef *nt, LookupContext *ctx, LookupContext::Function *ctxFunction)
    : parserFuncDef(nt), declaration(&nt->decl, ctxFunction), body(ctx, &nt->body)
{
}

String FuncDef::getName() const {
    return parserFuncDef->getName();
}

IdentifierId FuncDef::getId() const {
    return id;
}

void FuncDef::setId(IdentifierId id) {
    this->id = id;
}

void FuncDef::symbolsPass1(LookupContext *ctx) {
    declaration.symbolsPass1(ctx);
    body.symbolsPass1();
}

void FuncDef::symbolsPass2(LookupContext *ctx) {
    declaration.symbolsPass2(ctx);
    body.symbolsPass2();
}

void FuncDef::codeGen(PracticalSemanticAnalyzer::ModuleGen *moduleGen) {
    std::shared_ptr<FunctionGen> functionGen = moduleGen->handleFunction(id);

    if( functionGen==nullptr )
        return;

    functionGen->functionEnter(
            id,
            declaration.getName(),
            declaration.getRetType(),
            declaration.getArguments(),
            toSlice("No file"), declaration.getLine(), declaration.getCol());

    ExpressionId result = body.codeGen( functionGen.get(), ExpectedType( &declaration.getRetType() ) );
    functionGen->returnValue(result);

    functionGen->functionLeave(id);
}

Module::Module( NonTerminals::Module *parseModule, LookupContext *parentSymbolsTable )
    : parseModule(parseModule), ctx(parentSymbolsTable), id(moduleIdAllocator.allocate())
{}

void Module::symbolsPass1() {
    for( auto &symbol : parseModule->functionDefinitions ) {
        FuncDef *funcDef = nullptr;

        auto previousDefinition = ctx.lookupIdentifier(symbol.getName());
        if( previousDefinition == nullptr ) {
            funcDef = &functionDefinitions.emplace_back(
                    &symbol, &ctx, ctx.registerFunctionPass1( symbol.decl.name.identifier ));
        } else {
            ABORT() << "TODO Overloads not yet implemented";
        }

        funcDef->symbolsPass1(&ctx);
    }
}

void Module::symbolsPass2() {
    for( auto &funcDef: functionDefinitions ) {
        funcDef.symbolsPass2(&ctx);
    }
}

void Module::codeGen(PracticalSemanticAnalyzer::ModuleGen *codeGenCB) {
    codeGenCB->moduleEnter(id, toSlice("OnlyModule"), toSlice("No file"), 1, 1);

    for(auto &function: functionDefinitions) {
        function.codeGen(codeGenCB);
    }

    codeGenCB->moduleLeave(id);
}

} // Namespace AST

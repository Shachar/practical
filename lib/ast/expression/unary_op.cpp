/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include "ast/expression/unary_op.h"

#include "ast/operators/boolean.h"

#include <experimental/array>

namespace AST::ExpressionImpl {

// Non member private helpers
static std::unordered_map< Tokenizer::Tokens, std::string > operatorNames;

static String opToFuncName( Tokenizer::Tokens token ) {
    return operatorNames.at(token);
}

// Static methods
void UnaryOp::init(LookupContext &builtinCtx) {
    auto unsignedTypes = std::experimental::make_array<const StaticTypeImpl::CPtr>(
        builtinCtx.lookupType( "U8" ),
        builtinCtx.lookupType( "U16" ),
        builtinCtx.lookupType( "U32" ),
        builtinCtx.lookupType( "U64" )
    );
    auto signedTypes = std::experimental::make_array<const StaticTypeImpl::CPtr>(
        builtinCtx.lookupType( "S8" ),
        builtinCtx.lookupType( "S16" ),
        builtinCtx.lookupType( "S32" ),
        builtinCtx.lookupType( "S64" )
    );
    const StaticTypeImpl::CPtr boolType = builtinCtx.lookupType( "Bool" );

    auto inserter = operatorNames.emplace( Tokenizer::Tokens::OP_ARROW, "__opArrow" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_AMPERSAND, "__opAmpersand" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_PTR, "__opDereference" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_BIT_NOT, "__opOneComplement" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_MINUS, "__opMinus" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_MINUS_MINUS, "__opMinusMinus" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_PLUS, "__opPlus" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_PLUS_PLUS, "__opPlusPlus" );
    inserter = operatorNames.emplace( Tokenizer::Tokens::OP_LOGIC_NOT, "__opNot" );
    builtinCtx.addBuiltinFunction( inserter.first->second, boolType, { boolType }, Operators::logicalNot, Operators::logicalNotVrp );
}

UnaryOp::UnaryOp( const NonTerminals::Expression::UnaryOperator &parserOp ) :
    parserOp(parserOp)
{}

SourceLocation UnaryOp::getLocation() const {
    return parserOp.op->location;
}

// Protected methods
void UnaryOp::buildASTImpl(
        LookupContext &lookupContext, ExpectedResult expectedResult, Weight &weight, Weight weightLimit )
{
    bool defaultHandling = true;

    // The special cases
    switch( parserOp.op->token ) {
    case Tokenizer::Tokens::OP_AMPERSAND:
        defaultHandling = false;
        body.emplace<AddressOf>( *parserOp.operand ).
                buildASTImpl(lookupContext, expectedResult, metadata, weight, weightLimit);
        break;
    case Tokenizer::Tokens::OP_PTR:
        defaultHandling = false;
        body.emplace<Dereference>( *parserOp.operand ).
                buildASTImpl(lookupContext, expectedResult, metadata, weight, weightLimit);
        break;
    default:
        break;
    }

    if( defaultHandling ) {
        buildASTFromTemplate(body.emplace<OverloadResolver>(), lookupContext, expectedResult, weight, weightLimit);
    }
}

void UnaryOp::buildASTFromTemplate(
        OverloadResolver &resolver, LookupContext &lookupContext, ExpectedResult expectedResult,
        Weight &weight, Weight weightLimit )
{
    String baseName = opToFuncName( parserOp.op->token );
    auto identifier = lookupContext.lookupIdentifier( baseName );
    ASSERT( identifier )<<"Unary operator "<<parserOp.op->token<<" is not yet implemented by the compiler";
    const LookupContext::Function &function =
            std::get<LookupContext::Function>(*identifier);

    resolver.resolveOverloads( lookupContext, expectedResult, function.overloads, weight, weightLimit, metadata,
            { parserOp.operand.get() }, parserOp.op );
}

ExpressionId UnaryOp::codeGenImpl( PracticalSemanticAnalyzer::FunctionGen *functionGen ) const {
    struct Visitor {
        PracticalSemanticAnalyzer::FunctionGen *functionGen;

        ExpressionId operator()( std::monostate ) {
            ABORT()<<"Trying to codegen over uninitialized unary operator";
        }

        ExpressionId operator()( const OverloadResolver &resolver ) {
            return resolver.codeGen( functionGen );
        }

        ExpressionId operator()( const AddressOf &addressOf ) {
            return addressOf.codeGen( functionGen );
        }

        ExpressionId operator()( const Dereference &dereference ) {
            return dereference.codeGen( functionGen );
        }
    };

    return std::visit( Visitor{ .functionGen = functionGen }, body );
}

} // namespace AST::ExpressionImpl

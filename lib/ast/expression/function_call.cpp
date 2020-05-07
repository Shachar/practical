/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2019 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include "function_call.h"

#include "ast/expression/identifier.h"
#include "ast/mangle.h"

using namespace PracticalSemanticAnalyzer;

namespace AST::ExpressionImpl {

FunctionCall::FunctionCall( const NonTerminals::Expression::FunctionCall &parserFunctionCall ) :
    parserFunctionCall( parserFunctionCall )
{
}

// protected methods
void FunctionCall::buildASTImpl( LookupContext &lookupContext, ExpectedResult expectedResult ) {
    functionId.emplace( *parserFunctionCall.expression );
    functionId->buildAST( lookupContext, ExpectedResult() );

    const Identifier *identifier = functionId->tryGetActualExpression<Identifier>();
    ASSERT(identifier)<<"TODO calling function through generic pointer expression not yet implemented";

    StaticType::Types functionIdType = functionId->getType()->getType();
    auto functionType = std::get_if<const StaticType::Function *>( &functionIdType );

    ASSERT( functionType!=nullptr )<<"TODO implement calling non function identifiers";

    ASSERT( arguments.size()==0 )<<"buildAST called twice";
    arguments.reserve( (*functionType)->getNumArguments() );

    for( unsigned argumentNum=0; argumentNum<(*functionType)->getNumArguments(); ++argumentNum ) {
        Expression &argument = arguments.emplace_back( parserFunctionCall.arguments.arguments[argumentNum] );
        argument.buildAST( lookupContext, ExpectedResult( (*functionType)->getArgumentType(argumentNum) ) );
    }

    StaticTypeImpl::CPtr returnType = static_cast<const StaticTypeImpl *>( (*functionType)->getReturnType().get() );
    metadata.valueRange = returnType->defaultRange();
    metadata.type = std::move(returnType);

    functionName = getFunctionMangledName( identifier->getName(), functionId->getType() );
}

ExpressionId FunctionCall::codeGenImpl( PracticalSemanticAnalyzer::FunctionGen *functionGen ) {
    ASSERT( !functionName.empty() )<<"TODO implement overloads and callable expressions";

    ExpressionId resultId = Expression::allocateId();
    ExpressionId argumentExpressionIds[arguments.size()];
    for( unsigned argIdx=0; argIdx<arguments.size(); ++argIdx ) {
        argumentExpressionIds[argIdx] = arguments[argIdx].codeGen( functionGen );
    }

    functionGen->callFunctionDirect(
            resultId, String(functionName), Slice(argumentExpressionIds, arguments.size()), metadata.type );

    return resultId;
}

} // namespace AST::ExpressionImpl

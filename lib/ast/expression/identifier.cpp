/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2019 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include "ast/expression/identifier.h"

#include <practical-errors.h>

using namespace PracticalSemanticAnalyzer;

namespace AST::ExpressionImpl {

Identifier::Identifier( const NonTerminals::Identifier &parserIdentifier ) :
    parserIdentifier( parserIdentifier )
{
}

String Identifier::getName() const {
    return parserIdentifier.identifier->text;
}

void Identifier::buildASTImpl( LookupContext &lookupContext, ExpectedResult expectedResult ) {
    symbol = lookupContext.lookupSymbol( parserIdentifier.identifier->text );

    if( symbol==nullptr ) {
        throw SymbolNotFound(
                parserIdentifier.identifier->text, parserIdentifier.identifier->line, parserIdentifier.identifier->col );
    }

    metadata.type = symbol->type;
    metadata.valueRange = symbol->type->defaultRange();
}

ExpressionId Identifier::codeGenImpl( PracticalSemanticAnalyzer::FunctionGen *functionGen ) {
    if( symbol->lvalueId!=ExpressionId() ) {
        ExpressionId resultId = allocateId();

        functionGen->dereferencePointer( resultId, symbol->type, symbol->lvalueId );

        return resultId;
    }

    ABORT()<<"TODO implement by name identifier lookup";
}

} // namespace AST::ExpressionImpl

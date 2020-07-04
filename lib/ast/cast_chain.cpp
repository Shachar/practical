/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include "ast/cast_chain.h"

namespace AST {

CastChain::CastChain(
        std::unique_ptr<CastChain> &&previousCast,
        const LookupContext::CastDescriptor &cast,
        ExpressionImpl::ExpressionMetadata metadata
    ) :
        previousCast( std::move(previousCast) ),
        cast(cast),
        metadata(metadata)
{}

std::unique_ptr<CastChain> CastChain::allocate(
        const LookupContext &lookupContext,
        StaticTypeImpl::CPtr destinationType,
        const ExpressionImpl::ExpressionMetadata &srcMetadata,
        unsigned &weight,
        unsigned weightLimit )
{
    ASSERT( destinationType != srcMetadata.type );

    {
        // Fastpath: direct cast from source to destination
        auto castDescriptor = lookupContext.lookupCast( srcMetadata.type, destinationType );

        if(
                castDescriptor!=nullptr &&
                castDescriptor->whenPossible!=LookupContext::CastDescriptor::ImplicitCastAllowed::Never )
        {
            return fastPathAllocate( castDescriptor, srcMetadata );
        }
    }

    LookupContext::CastsList upperHook = lookupContext.allCastsTo( destinationType );
    if( !upperHook )
        return nullptr;

    LookupContext::CastsList lowerHook = lookupContext.allCastsFrom( srcMetadata.type );
    if( !upperHook )
        return nullptr;

    ABORT()<<"TODO implement";
}

ExpressionId CastChain::codeGen(
            PracticalSemanticAnalyzer::StaticType::CPtr sourceType, ExpressionId sourceExpression,
            PracticalSemanticAnalyzer::FunctionGen *functionGen
        ) const
{
    ExpressionId previousResult = sourceExpression;
    if( previousCast ) {
        previousResult = previousCast->codeGen( sourceType, sourceExpression, functionGen );
        sourceType = previousCast->getMetadata().type;
    }

    return cast.codeGen( sourceType, previousResult, metadata.type, functionGen );
}

std::unique_ptr<CastChain> CastChain::fastPathAllocate(
            const LookupContext::CastDescriptor *castDescriptor,
            const ExpressionImpl::ExpressionMetadata &srcMetadata )
{
    ExpressionImpl::ExpressionMetadata metadata;
    metadata.type = castDescriptor->destType;

    if( castDescriptor->calcVrp ) {
        metadata.valueRange = castDescriptor->calcVrp(
                srcMetadata.type.get(),
                castDescriptor->destType.get(),
                srcMetadata.valueRange,
                true );
    } else {
        ASSERT( castDescriptor->whenPossible == LookupContext::CastDescriptor::ImplicitCastAllowed::Always );

        metadata.valueRange = castDescriptor->destType->defaultRange();
    }

    return std::unique_ptr<CastChain>( new CastChain(
                nullptr,
                *castDescriptor,
                std::move( metadata ) )
            );
}

} // namespace AST
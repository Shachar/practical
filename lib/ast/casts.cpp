/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include "ast/casts.h"

#include "ast/expression.h"
#include "ast/signed_int_value_range.h"
#include "ast/unsigned_int_value_range.h"

namespace AST {

ExpressionId signedExpansionCast( 
            PracticalSemanticAnalyzer::StaticType::CPtr sourceType, ExpressionId sourceExpression,
            PracticalSemanticAnalyzer::StaticType::CPtr destType,
            PracticalSemanticAnalyzer::FunctionGen *functionGen)
{
    ExpressionId id = Expression::allocateId();
    functionGen->expandIntegerSigned( id, sourceExpression, sourceType, destType );

    return id;
}

ExpressionId unsignedExpansionCast( 
            PracticalSemanticAnalyzer::StaticType::CPtr sourceType, ExpressionId sourceExpression,
            PracticalSemanticAnalyzer::StaticType::CPtr destType,
            PracticalSemanticAnalyzer::FunctionGen *functionGen)
{
    ExpressionId id = Expression::allocateId();
    functionGen->expandIntegerUnsigned( id, sourceExpression, sourceType, destType );

    return id;
}

ValueRangeBase::CPtr identityVrp(
            const StaticTypeImpl *sourceType,
            const StaticTypeImpl *destType,
            ValueRangeBase::CPtr inputRange,
            bool isImplicit )
{
    return inputRange;
}

ValueRangeBase::CPtr unsignedToSignedIdentityVrp(
            const StaticTypeImpl *sourceType,
            const StaticTypeImpl *destType,
            ValueRangeBase::CPtr inputRangeBase,
            bool isImplicit )
{
    ASSERT(
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType() ==
            StaticType::Scalar::Type::UnsignedInt
          ) <<
            "VRP for unsigned->signed called on input of type "<<
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType();

    ASSERT( dynamic_cast<const UnsignedIntValueRange *>(inputRangeBase.get())!=nullptr );

    auto inputRange = static_cast<const UnsignedIntValueRange *>(inputRangeBase.get());

    auto maximalRange = destType->defaultRange();
    ASSERT( dynamic_cast<const SignedIntValueRange *>( maximalRange.get() )!=nullptr );

    ASSERT(
            inputRange->maximum <=
            static_cast<LongEnoughInt>( static_cast<const SignedIntValueRange *>(maximalRange.get())->maximum ) );

    return SignedIntValueRange::allocate( inputRange->minimum, inputRange->maximum );
}

ExpressionId integerReductionCast(
            PracticalSemanticAnalyzer::StaticType::CPtr sourceType, ExpressionId sourceExpression,
            PracticalSemanticAnalyzer::StaticType::CPtr destType,
            PracticalSemanticAnalyzer::FunctionGen *functionGen)
{
    ExpressionId id = Expression::allocateId();
    functionGen->truncateInteger( id, sourceExpression, sourceType, destType );

    return id;
}

ValueRangeBase::CPtr unsignedReductionVrp(
            const StaticTypeImpl *sourceType,
            const StaticTypeImpl *destType,
            ValueRangeBase::CPtr inputRangeBase,
            bool isImplicit )
{
    ASSERT(
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType() ==
            StaticType::Scalar::Type::UnsignedInt
          ) <<
            "VRP for unsigned->signed called on input of type "<<
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType();

    ASSERT( dynamic_cast<const UnsignedIntValueRange *>(inputRangeBase.get())!=nullptr );

    auto inputRange = static_cast<const UnsignedIntValueRange *>(inputRangeBase.get());

    auto maximalRange = destType->defaultRange();
    ASSERT( dynamic_cast<const UnsignedIntValueRange *>( maximalRange.get() )!=nullptr );

    if( inputRange->maximum > static_cast<const UnsignedIntValueRange *>(maximalRange.get())->maximum ) {
        // Values out of range
        if( ! isImplicit )
            return maximalRange;

        return ValueRangeBase::CPtr(); // Cannot implicit cast
    } else {
        return UnsignedIntValueRange::allocate( inputRange->minimum, inputRange->maximum );
    }
}

ValueRangeBase::CPtr signedReductionVrp(
            const StaticTypeImpl *sourceType,
            const StaticTypeImpl *destType,
            ValueRangeBase::CPtr inputRangeBase,
            bool isImplicit )
{
    ASSERT(
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType() ==
            StaticType::Scalar::Type::SignedInt
          ) <<
            "VRP for signed->signed called on input of type "<<
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType();

    ASSERT( dynamic_cast<const SignedIntValueRange *>(inputRangeBase.get())!=nullptr );

    auto inputRange = static_cast<const SignedIntValueRange *>(inputRangeBase.get());

    auto maximalRangeBase = destType->defaultRange();
    ASSERT( dynamic_cast<const SignedIntValueRange *>( maximalRangeBase.get() )!=nullptr );
    auto maximalRange = static_cast<const SignedIntValueRange *>(maximalRangeBase.get());

    if(
            inputRange->maximum > maximalRange->maximum ||
            inputRange->minimum < maximalRange->minimum
      )
    {
        // Values out of range
        if( ! isImplicit )
            return maximalRange;

        return ValueRangeBase::CPtr(); // Cannot implicit cast
    } else {
        return SignedIntValueRange::allocate( inputRange->minimum, inputRange->maximum );
    }
}

ValueRangeBase::CPtr signed2UnsignedVrp(
            const StaticTypeImpl *sourceType,
            const StaticTypeImpl *destType,
            ValueRangeBase::CPtr inputRangeBase,
            bool isImplicit )
{
    ASSERT(
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType() ==
            StaticType::Scalar::Type::SignedInt
          ) <<
            "VRP for signed->unsigned called on input of type "<<
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType();

    ASSERT( dynamic_cast<const SignedIntValueRange *>(inputRangeBase.get())!=nullptr );

    auto inputRange = static_cast<const SignedIntValueRange *>(inputRangeBase.get());

    auto maximalRangeBase = destType->defaultRange();
    ASSERT( dynamic_cast<const UnsignedIntValueRange *>( maximalRangeBase.get() )!=nullptr );
    auto maximalRange = static_cast<const UnsignedIntValueRange *>(maximalRangeBase.get());

    if(
            inputRange->minimum<0
      )
    {
        // Values out of range
        if( ! isImplicit )
            return maximalRange;

        return ValueRangeBase::CPtr(); // Cannot implicit cast
    } else {
        return UnsignedIntValueRange::allocate( inputRange->minimum, inputRange->maximum );
    }
}

ValueRangeBase::CPtr unsigned2SignedVrp(
            const StaticTypeImpl *sourceType,
            const StaticTypeImpl *destType,
            ValueRangeBase::CPtr inputRangeBase,
            bool isImplicit )
{
    ASSERT(
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType() ==
            StaticType::Scalar::Type::UnsignedInt
          ) <<
            "VRP for unsigned->signed ("<<(*sourceType)<<" to "<<(*destType)<<") called on input of type "<<
            std::get<const StaticType::Scalar *>(sourceType->getType())->getType();

    ASSERT( dynamic_cast<const UnsignedIntValueRange *>(inputRangeBase.get())!=nullptr );

    auto inputRange = static_cast<const UnsignedIntValueRange *>(inputRangeBase.get());

    auto maximalRangeBase = destType->defaultRange();
    ASSERT( dynamic_cast<const SignedIntValueRange *>( maximalRangeBase.get() )!=nullptr );
    auto maximalRange = static_cast<const SignedIntValueRange *>(maximalRangeBase.get());

    if(
            inputRange->maximum > maximalRange->maximum
      )
    {
        // Values out of range
        if( ! isImplicit )
            return maximalRange;

        return ValueRangeBase::CPtr(); // Cannot implicit cast
    } else {
        return SignedIntValueRange::allocate( inputRange->minimum, inputRange->maximum );
    }
}

ExpressionId changeSignCast(
            PracticalSemanticAnalyzer::StaticType::CPtr sourceType, ExpressionId sourceExpression,
            PracticalSemanticAnalyzer::StaticType::CPtr destType,
            PracticalSemanticAnalyzer::FunctionGen *functionGen)
{
    ExpressionId id = Expression::allocateId();
    functionGen->changeIntegerSign( id, sourceExpression, sourceType, destType );

    return id;
}

} // namespace AST

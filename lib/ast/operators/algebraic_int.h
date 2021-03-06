/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef AST_OPERATORS_ALGEBRAIC_INT_H
#define AST_OPERATORS_ALGEBRAIC_INT_H

#include "ast/expression.h"

#include <practical/practical.h>

namespace AST::Operators {

using PracticalSemanticAnalyzer::ExpressionId;

ExpressionId bPlusCodegenUnsigned(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);
ValueRangeBase::CPtr bPlusUnsignedVrp(StaticTypeImpl::CPtr functType, Slice<ValueRangeBase::CPtr> inputRanges);

ExpressionId bPlusCodegenSigned(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);
ValueRangeBase::CPtr bPlusSignedVrp(StaticTypeImpl::CPtr functType, Slice<ValueRangeBase::CPtr> inputRanges);



ExpressionId bMinusCodegenUnsigned(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);
ValueRangeBase::CPtr bMinusUnsignedVrp(StaticTypeImpl::CPtr functType, Slice<ValueRangeBase::CPtr> inputRanges);

ExpressionId bMinusCodegenSigned(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);
ValueRangeBase::CPtr bMinusSignedVrp(StaticTypeImpl::CPtr functType, Slice<ValueRangeBase::CPtr> inputRanges);



ExpressionId bMultiplyCodegenUnsigned(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);
ValueRangeBase::CPtr bMultiplyUnsignedVrp(StaticTypeImpl::CPtr functType, Slice<ValueRangeBase::CPtr> inputRanges);

ExpressionId bMultiplyCodegenSigned(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);
ValueRangeBase::CPtr bMultiplySignedVrp(StaticTypeImpl::CPtr functType, Slice<ValueRangeBase::CPtr> inputRanges);



ExpressionId bDivideCodegenUnsigned(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);
ValueRangeBase::CPtr bDivideUnsignedVrp(StaticTypeImpl::CPtr functType, Slice<ValueRangeBase::CPtr> inputRanges);

} // namespace AST::Operators

#endif // AST_OPERATORS_ALGEBRAIC_INT_H

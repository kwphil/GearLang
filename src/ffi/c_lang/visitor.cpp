/*
   _____                 _                       
  / ____|               | |                      
 | |  __  ___  __ _ _ __| |     __ _ _ __   __ _ 
 | | |_ |/ _ \/ _` | '__| |    / _` | '_ \ / _` | Clean, Clear and Fast Code
 | |__| |  __/ (_| | |  | |___| (_| | | | | (_| | https://github.com/kwphil/gearlang
  \_____|\___|\__,_|_|  |______\__,_|_| |_|\__, |
                                            __/ |
                                           |___/ 

Licensed under the MIT License <https://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <gearlang/ast/func.hpp>
#include <gearlang/etc.hpp>
#include <clang/AST/Type.h>

#include "visitor.hpp"

bool CAstVisitor::VisitFunctionDecl(clang::FunctionDecl* func) {
    using namespace Ast::Nodes;
    using clang::QualType;

    if (func->isThisDeclarationADefinition() || func->isExternC()) {
        std::deque<unique_ptr<Argument>> args;
        bool is_variadic = func->isVariadic();

        for (unsigned i = 0; i < func->getNumParams(); ++i) {
            QualType qt = func->getParamDecl(i)->getType();

            args.push_back(std::make_unique<Argument>(Argument(
                "",
                *manager.c_to_gear_ty(&qt),
                { "", 0, 0, 0, 0 }
            )));
        }

        QualType ret = func->getReturnType();
        ExternFn fn(
            func->getNameAsString(), 
            *manager.c_to_gear_ty(&ret),
            std::move(args),
            is_variadic,
            ManglingScheme::None,
            { "", 0, 0, 0, 0 }   
        );

        manager.add_node(std::make_unique<ExternFn>(std::move(fn)));
    }
    return true;
}

bool CAstVisitor::VisitRecordDecl(clang::RecordDecl* record) {
    if (record->isCompleteDefinition()) {
        manager.c_record_to_gear(record);
    }

    return true;
}

bool CAstVisitor::VisitEnumDecl(clang::EnumDecl* enm) {
    // if (enm->isComplete())
    //     std::cout << "Enum: " << enm->getNameAsString() << "\n";
    return true;
}
#include <iostream>
bool CAstVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl* td) {
    auto underlying = td->getUnderlyingType();
    Sem::Type::add_alias(td->getNameAsString(), *manager.c_to_gear_ty(&underlying));
    // std::cout << "Typedef: " << td->getNameAsString() << "\n";
    return true;
}
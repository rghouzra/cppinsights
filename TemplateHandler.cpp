/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#include "TemplateHandler.h"
#include <type_traits>
#include "ClangCompat.h"
#include "CodeGenerator.h"
#include "DPrint.h"
#include "InsightsHelpers.h"
#include "InsightsMatchers.h"
#include "OutputFormatHelper.h"

#include "llvm/Support/Path.h"
//-----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
//-----------------------------------------------------------------------------

namespace clang::ast_matchers {
const internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateDecl> varTemplateDecl;  // NOLINT
}

namespace clang::insights {
/// \brief Inserts the instantiation point of a template.
//
// This reveals at which place the template is first used.
static void
InsertInstantiationPoint(OutputFormatHelper& outputFormatHelper, const SourceManager& sm, const SourceLocation& instLoc)
{
    const auto  lineNo = sm.getSpellingLineNumber(instLoc);
    const auto& fileId = sm.getFileID(instLoc);
    const auto* file   = sm.getFileEntryForID(fileId);
    if(file) {
        const auto fileWithDirName = file->getName();
        const auto fileName        = llvm::sys::path::filename(fileWithDirName);

        outputFormatHelper.AppendNewLine("/* First instantiated from: ", fileName, ":", lineNo, " */");
    }
}
//-----------------------------------------------------------------------------

// Workaround to keep clang 6 Linux build alive
template<class T, class U>
inline constexpr bool is_same_v = std::is_same<T, U>::value;  // NOLINT
//-----------------------------------------------------------------------------

/// \brief Insert the instantiated template with the resulting code.
template<typename T>
static OutputFormatHelper InsertInstantiatedTemplate(const T& decl, const MatchFinder::MatchResult&)
{
    OutputFormatHelper outputFormatHelper{};
    outputFormatHelper.AppendNewLine();
    outputFormatHelper.AppendNewLine();

    //    const auto& sm = GetSM(result);

    /*if constexpr(not is_same_v<VarTemplateDecl, T>) {  // NOLINT
        InsertInstantiationPoint(outputFormatHelper, sm, decl.getPointOfInstantiation());
    }*/

    // outputFormatHelper.AppendNewLine("#ifdef INSIGHTS_USE_TEMPLATE");
    CodeGenerator codeGenerator{outputFormatHelper};

    if constexpr(is_same_v<VarTemplateDecl, T>) {
        for(const auto& spec : decl.specializations()) {
            // InsertInstantiationPoint(outputFormatHelper, sm, spec->getPointOfInstantiation());
            codeGenerator.InsertArg(spec);
        }
    } else {
        codeGenerator.InsertArg(&decl);
    }

    // outputFormatHelper.AppendNewLine("#endif");

    return outputFormatHelper;
}
//-----------------------------------------------------------------------------

TemplateHandler::TemplateHandler(Rewriter& rewrite, MatchFinder& matcher)
: InsightsBase(rewrite)
{
    matcher.addMatcher(
        functionTemplateDecl(
            unless(anyOf(isExpansionInSystemHeader(), isMacroOrInvalidLocation(), hasAncestor(cxxRecordDecl()))))
            .bind("func"),
        this);

#if 0
        functionDecl(allOf(unless(isExpansionInSystemHeader()),
                           unless(isMacroOrInvalidLocation()),
                           hasParent(functionTemplateDecl(unless(hasParent(classTemplateSpecializationDecl())),
                                                          unless(hasParent(cxxRecordDecl())))),
                           isTemplateInstantiationPlain()),
                     unless(hasAncestor(cxxRecordDecl())))
            .bind("func"),
        this);
#endif

    // match typical use where a class template is defined and it is used later.
    matcher.addMatcher(classTemplateSpecializationDecl(unless(anyOf(isExpansionInSystemHeader(),
                                                                    hasAncestor(cxxRecordDecl()),
                                                                    hasAncestor(classTemplateSpecializationDecl()))),
                                                       hasParent(classTemplateDecl().bind("decl")))
                           .bind("class"),
                       this);

    // special case, where a class template is defined and somewhere else we request an explicit instantiation
    matcher.addMatcher(classTemplateSpecializationDecl(unless(anyOf(isExpansionInSystemHeader(),
                                                                    hasParent(classTemplateDecl()),
                                                                    isExplicitTemplateSpecialization())))
                           .bind("class"),
                       this);

    matcher.addMatcher(
        varTemplateDecl(unless(isExpansionInSystemHeader()), unless(hasParent(classTemplateDecl()))).bind("vd"), this);
}
//-----------------------------------------------------------------------------

void TemplateHandler::run(const MatchFinder::MatchResult& result)
{
    DPrint("asdfasdfsd\n");
    if(const auto* functionDecl = result.Nodes.getNodeAs<FunctionTemplateDecl>("func")) {
        DPrint("functionTmpl\n");
        if(not functionDecl->getBody()) {
            // DPrint("exit\n");
            // return;
        }

        // skip implicit function template decls. One reason are deduction guides which bring implicit declarations with
        // them but at the same source location as the class template.
        if(functionDecl->isImplicit()) {
            return;
        }

        functionDecl->dump();

        OutputFormatHelper outputFormatHelper{};
        outputFormatHelper.AppendNewLine();
        outputFormatHelper.AppendNewLine();

        CodeGenerator codeGenerator{outputFormatHelper};

        for(const auto* spec : functionDecl->specializations()) {
            codeGenerator.InsertArg(spec);
            outputFormatHelper.AppendNewLine();
        }

        //        OutputFormatHelper outputFormatHelper = InsertInstantiatedTemplate(*functionDecl, result);
        const auto endOfCond = FindLocationAfterSemi(GetEndLoc(*functionDecl), result);

        InsertIndentedText(endOfCond.getLocWithOffset(1), outputFormatHelper);
        // mRewrite.ReplaceText(functionDecl->getSourceRange(), outputFormatHelper.GetString());

    } else if(const auto* clsTmplSpecDecl = result.Nodes.getNodeAs<ClassTemplateSpecializationDecl>("class")) {
        // skip classes/struct's without a definition
        if(not clsTmplSpecDecl->hasDefinition()) {
            return;
        }

        OutputFormatHelper outputFormatHelper = InsertInstantiatedTemplate(*clsTmplSpecDecl, result);

        if(const auto* clsTmplDecl = result.Nodes.getNodeAs<ClassTemplateDecl>("decl")) {
            const auto endOfCond = FindLocationAfterSemi(GetEndLoc(clsTmplDecl), result);
            InsertIndentedText(endOfCond, outputFormatHelper);

        } else {  // explicit specialization, we have to remove the specialization
            mRewrite.ReplaceText(clsTmplSpecDecl->getSourceRange(), outputFormatHelper.GetString());
        }

    } else if(const auto* vd = result.Nodes.getNodeAs<VarTemplateDecl>("vd")) {
        OutputFormatHelper outputFormatHelper = InsertInstantiatedTemplate(*vd, result);

        const auto endOfCond = FindLocationAfterSemi(GetEndLoc(vd), result);
        InsertIndentedText(endOfCond, outputFormatHelper);
    }
}
//-----------------------------------------------------------------------------

}  // namespace clang::insights

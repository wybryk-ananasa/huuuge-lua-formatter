#include "LuaCodeFormat.h"

#include "CodeFormatCore/Config/LuaEditorConfig.h"
#include "CodeFormatCore/RangeFormat/RangeFormatBuilder.h"
#include "LuaParser/Parse/LuaParser.h"
#include "Util/StringUtil.h"

LuaCodeFormat &LuaCodeFormat::GetInstance() {
    static LuaCodeFormat instance;
    return instance;
}

LuaCodeFormat::LuaCodeFormat()
    : _supportNonStandardSymbol(false) {
}

void LuaCodeFormat::UpdateCodeStyle(const std::string &workspaceUri, const std::string &configPath) {
    for (auto &config: _configs) {
        if (config.Workspace == workspaceUri) {
            config.Editorconfig = LuaEditorConfig::LoadFromFile(configPath);
            config.Editorconfig->Parse();
            return;
        }
    }

    auto &config = _configs.emplace_back(
            workspaceUri);
    config.Editorconfig = LuaEditorConfig::LoadFromFile(configPath);
    config.Editorconfig->Parse();
}

void LuaCodeFormat::UpdateDiagnosticStyle(InfoTree &tree) {
    _diagnosticStyle = LuaDiagnosticStyle();
    _diagnosticStyle.ParseTree(tree);
    _diagnosticStyle.code_style_check = true;
    _diagnosticStyle.name_style_check = true;
    _diagnosticStyle.spell_check = true;
}

void LuaCodeFormat::RemoveCodeStyle(const std::string &workspaceUri) {
    for (auto it = _configs.begin(); it != _configs.end(); it++) {
        if (it->Workspace == workspaceUri) {
            _configs.erase(it);
            break;
        }
    }
}

void LuaCodeFormat::SetDefaultCodeStyle(ConfigMap &configMap) {
    if (!configMap.empty()) {
        LuaStyle style;
        style.Parse(configMap);
        _defaultStyle = style;
    }
}

void LuaCodeFormat::SupportNonStandardSymbol() {
    _supportNonStandardSymbol = true;
}

void LuaCodeFormat::LoadSpellDictionary(const std::string &path) {
    _spellChecker.LoadDictionary(path);
}

void LuaCodeFormat::LoadSpellDictionaryFromBuffer(const std::string &buffer) {
    _spellChecker.LoadDictionaryFromBuffer(buffer);
}

Result<std::string> LuaCodeFormat::Reformat(const std::string &uri, std::string &&text, ConfigMap &configMap) {
    auto file = std::make_shared<LuaSource>(std::move(text));
    LuaLexer luaLexer(file);
    if (_supportNonStandardSymbol) {
        luaLexer.SupportNonStandardSymbol();
    }
    luaLexer.Parse();

    LuaParser p(file, std::move(luaLexer.GetTokens()));
    p.Parse();

    if (p.HasError()) {
        return ResultType::Err;
    }

    LuaSyntaxTree t;
    t.BuildTree(p);

    LuaStyle style = GetStyle(uri);
    CalculateTempStyle(style, configMap);

    FormatBuilder f(style);

    return f.GetFormatResult(t);
}

Result<std::string> LuaCodeFormat::RangeFormat(const std::string &uri, FormatRange &range,
                                               std::string &&text,
                                               ConfigMap &configMap) {
    auto file = std::make_shared<LuaSource>(std::move(text));
    LuaLexer luaLexer(file);
    if (_supportNonStandardSymbol) {
        luaLexer.SupportNonStandardSymbol();
    }
    luaLexer.Parse();

    LuaParser p(file, std::move(luaLexer.GetTokens()));
    p.Parse();

    if (p.HasError()) {
        return ResultType::Err;
    }

    LuaSyntaxTree t;
    t.BuildTree(p);

    LuaStyle style = GetStyle(uri);
    CalculateTempStyle(style, configMap);

    RangeFormatBuilder f(style, range);

    auto formattedText = f.GetFormatResult(t);
    range = f.GetReplaceRange();
    return formattedText;
}

Result<std::vector<LuaTypeFormat::Result>>
LuaCodeFormat::TypeFormat(const std::string &uri, std::size_t line, std::size_t character, std::string &&text,
                          ConfigMap &configMap, ConfigMap &stringTypeOptions) {
    auto file = std::make_shared<LuaSource>(std::move(text));
    LuaLexer luaLexer(file);
    if (_supportNonStandardSymbol) {
        luaLexer.SupportNonStandardSymbol();
    }
    luaLexer.Parse();

    LuaParser p(file, std::move(luaLexer.GetTokens()));
    p.Parse();

    if (p.HasError()) {
        return ResultType::Err;
    }

    LuaSyntaxTree t;
    t.BuildTree(p);

    LuaStyle style = GetStyle(uri);
    CalculateTempStyle(style, configMap);

    LuaTypeFormatFeatures typeFormatOptions = LuaTypeFormatFeatures::From(stringTypeOptions);

    LuaTypeFormat tf(typeFormatOptions);
    tf.Analyze("\n", line, character, t, style);
    return tf.GetResult();
}

Result<std::vector<LuaDiagnosticInfo>> LuaCodeFormat::Diagnostic(const std::string &uri, std::string &&text) {
    auto file = std::make_shared<LuaSource>(std::move(text));
    LuaLexer luaLexer(file);
    if (_supportNonStandardSymbol) {
        luaLexer.SupportNonStandardSymbol();
    }
    luaLexer.Parse();

    LuaParser p(file, std::move(luaLexer.GetTokens()));
    p.Parse();

    if (p.HasError()) {
        return ResultType::Err;
    }

    LuaSyntaxTree t;
    t.BuildTree(p);

    LuaStyle style = GetStyle(uri);

    DiagnosticBuilder diagnosticBuilder(style, _diagnosticStyle);
    diagnosticBuilder.CodeStyleCheck(t);
    return MakeDiagnosticInfo(diagnosticBuilder.GetDiagnosticResults(t), file);
}

Result<std::vector<LuaDiagnosticInfo>> LuaCodeFormat::SpellCheck(const std::string &uri, std::string &&text,
                                                                 const CodeSpellChecker::CustomDictionary &tempDict) {
    auto file = LuaSource::From(std::move(text));
    LuaLexer luaLexer(file);
    if (_supportNonStandardSymbol) {
        luaLexer.SupportNonStandardSymbol();
    }
    luaLexer.Parse();

    LuaParser p(file, std::move(luaLexer.GetTokens()));
    p.Parse();

    if (p.HasError()) {
        return ResultType::Err;
    }

    LuaSyntaxTree t;
    t.BuildTree(p);

    LuaStyle style = GetStyle(uri);

    DiagnosticBuilder diagnosticBuilder(style, _diagnosticStyle);
    _spellChecker.SetCustomDictionary(tempDict);
    diagnosticBuilder.SpellCheck(t, _spellChecker);
    return MakeDiagnosticInfo(diagnosticBuilder.GetDiagnosticResults(t), file);
}

Result<std::vector<LuaDiagnosticInfo>> LuaCodeFormat::NameStyleCheck(const std::string &uri, std::string &&text) {
    auto file = LuaSource::From(std::move(text));
    LuaLexer luaLexer(file);
    if (_supportNonStandardSymbol) {
        luaLexer.SupportNonStandardSymbol();
    }
    luaLexer.Parse();

    LuaParser p(file, std::move(luaLexer.GetTokens()));
    p.Parse();

    if (p.HasError()) {
        return ResultType::Err;
    }

    LuaSyntaxTree t;
    t.BuildTree(p);

    LuaStyle style = GetStyle(uri);

    DiagnosticBuilder diagnosticBuilder(style, _diagnosticStyle);

    diagnosticBuilder.NameStyleCheck(t);
    return MakeDiagnosticInfo(diagnosticBuilder.GetDiagnosticResults(t), file);
}

std::vector<SuggestItem> LuaCodeFormat::SpellCorrect(const std::string &word) {
    std::string letterWord = word;
    for (auto &c: letterWord) {
        c = std::tolower(c);
    }
    bool upperFirst = false;
    if (std::isupper(word.front())) {
        upperFirst = true;
    }

    auto suggests = _spellChecker.GetSuggests(letterWord);

    for (auto &suggest: suggests) {
        if (!suggest.Term.empty()) {
            if (upperFirst) {
                suggest.Term[0] = std::toupper(suggest.Term[0]);
            }
        }
    }
    return suggests;
}

LuaStyle &LuaCodeFormat::GetStyle(const std::string &uri) {
    std::shared_ptr<LuaEditorConfig> editorConfig = nullptr;
    std::size_t matchProcess = 0;
    for (auto &config: _configs) {
        if (string_util::StartWith(uri, config.Workspace)) {
            if (config.Workspace.size() > matchProcess) {
                matchProcess = config.Workspace.size();
                editorConfig = config.Editorconfig;
            }
        }
    }

    if (editorConfig) {
        return editorConfig->Generate(uri);
    }
    return _defaultStyle;
}

std::vector<LuaDiagnosticInfo> LuaCodeFormat::MakeDiagnosticInfo(const std::vector<LuaDiagnostic> &diagnostics,
                                                                 std::shared_ptr<LuaSource> file) {
    std::vector<LuaDiagnosticInfo> results;
    for (auto &diagnostic: diagnostics) {
        auto &result = results.emplace_back();
        result.Type = diagnostic.Type;
        result.Message = diagnostic.Message;
        result.Data = diagnostic.Data;
        result.Start.Line = file->GetLine(diagnostic.Range.StartOffset);
        result.Start.Col = file->GetColumn(diagnostic.Range.StartOffset);
        result.End.Line = file->GetLine(diagnostic.Range.GetEndOffset());
        result.End.Col = file->GetColumn(diagnostic.Range.GetEndOffset()) + 1;
    }

    return results;
}

void LuaCodeFormat::CalculateTempStyle(LuaStyle &style, ConfigMap &configMap) {
    if (configMap.empty()) {
        return;
    }

    if (configMap.count("insertSpaces")) {
        style.indent_style = configMap.at("insertSpaces") == "true"
                                     ? IndentStyle::Space
                                     : IndentStyle::Tab;
    }
    if (configMap.count("tabSize")) {
        if (style.indent_style == IndentStyle::Tab) {
            style.tab_width = std::stoi(configMap.at("tabSize"));
        } else if (style.indent_style == IndentStyle::Space) {
            style.indent_size = std::stoi(configMap.at("tabSize"));
        }
    }
}

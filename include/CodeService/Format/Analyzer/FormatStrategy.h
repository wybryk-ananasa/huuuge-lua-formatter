#pragma once

#include <cstdlib>
#include <vector>

enum class NextSpaceStrategy {
    None,
    Space,
    LineBreak
};

enum class PrevSpaceStrategy {
    None,
    AlignPos,
    AlignRelativeIndent,
    LineBreak
};

enum class TokenStrategy {
    Origin,
    Remove,
    StringSingleQuote,
    StringDoubleQuote,

    TableSepSemicolon,
    TableSepComma,
    TableAddSep,

    RemoveCommentTrailSpace
};

enum class IndentStrategy {
    None,
    Standard,
    InvertIndentation,
    WhenLineBreak,
    WhenPrevIndent
};

struct IndentData {
    IndentData(IndentStrategy strategy = IndentStrategy::Standard, std::size_t size = 0)
            : Strategy(strategy), Data(size) {}

    IndentStrategy Strategy;
    std::size_t Data;
};

enum class LineBreakStrategy {
    Standard,
    WhenMayExceed,
};

struct LineBreakData {
    LineBreakData() : Strategy(LineBreakStrategy::Standard) {
        Data.Line = 1;
    }

    explicit LineBreakData(std::size_t line)
            : Strategy(LineBreakStrategy::Standard) {
        Data.Line = line;
    }

    LineBreakData(LineBreakStrategy strategy, std::size_t index)
            : Strategy(strategy) {
        Data.Index = index;
    }


    LineBreakStrategy Strategy;
    union BreakData {
        std::size_t Line;
        std::size_t Index;
    } Data;
};

enum class AlignStrategy {
    AlignToEq,
    AlignToFirst
};

struct AlignGroup {
    AlignGroup(AlignStrategy strategy, std::vector<std::size_t> &group)
            : Strategy(strategy), SyntaxGroup(group), Resolve(false), AlignPos(0) {}

    AlignStrategy Strategy;
    std::vector<std::size_t> SyntaxGroup;
    bool Resolve;
    std::size_t AlignPos;
};

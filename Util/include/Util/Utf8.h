﻿#pragma once

#include <string>
#include <string_view>

namespace utf8 {
/*
 * 算法来自3rd/utf8库
 */
std::size_t Utf8nLen(const char *source, std::size_t byteNum);

std::size_t Utf8nByteNum(const char *source, std::size_t maxByteNum, std::size_t utf8Position);

std::size_t Utf8nLenAtFirstLine(const char *source, std::size_t byteNum);

std::size_t Utf8OneCharLen(const char *source);

uint32_t Utf8ToUnicode(const char *source, std::size_t maxNum, std::size_t &byteNum);
}// namespace utf8

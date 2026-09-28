#ifndef TEXTBLINDER_VERSION_H
#define TEXTBLINDER_VERSION_H

// 버전의 기준 파일. ReleaseTool 이 릴리스 때 아래 3 줄을 고쳐 쓴다.
// rc.exe 도 include 하므로 #pragma once 같은 C++ 전용 문법을 쓰지 않는다.
#define TB_VERSION_MAJOR 0
#define TB_VERSION_MINOR 1
#define TB_VERSION_PATCH 0

// 문자열은 위 3 줄에서 만든다. 숫자와 문자열이 어긋나지 않는다.
#define TB_STRINGIZE_(x) #x
#define TB_STRINGIZE(x) TB_STRINGIZE_(x)
#define TB_VERSION_STRING TB_STRINGIZE(TB_VERSION_MAJOR) "." TB_STRINGIZE(TB_VERSION_MINOR) "." TB_STRINGIZE(TB_VERSION_PATCH)

#endif  // TEXTBLINDER_VERSION_H

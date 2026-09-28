# TextBlinder

**TextBlinder 는 Notepad++ 에서 지정한 글자를 화면에서만 가리는 플러그인이다.** 문서의 글자는 바꾸지 않는다.

화면 공유, 녹화, 옆 사람 시선에서 비밀번호와 키를 숨길 때 쓴다. 저장하거나 복사하면 원문이 그대로 나온다.

*English: TextBlinder is a Notepad++ plugin that hides selected text on screen only. It draws an opaque box over the text. The document does not change. Save and copy give the original text.*

## 기능

- **선택 가림** — 선택한 범위를 가린다. 파일을 닫았다가 다시 열어도 가림이 남는다.
- **규칙 가림** — 설정 파일의 정규식에 맞는 곳을 자동으로 가린다.

## 설치

필요한 것: Notepad++ 8.5.6 이상, 64 비트 (x64).

1. Notepad++ 를 닫는다.
2. 릴리스 zip 을 받아서 푼다.
3. `TextBlinder.dll` 을 `C:\Program Files\Notepad++\plugins\TextBlinder\` 폴더에 넣는다. 폴더가 없으면 만든다.
4. Notepad++ 를 연다. `플러그인 > TextBlinder` 메뉴가 생긴다.

**참고:** 폴더 이름과 DLL 이름이 둘 다 `TextBlinder` 여야 Notepad++ 가 플러그인을 읽는다.

## 메뉴와 단축키

`플러그인 > TextBlinder` 메뉴:

| 명령 | 기본 단축키 | 하는 일 |
| --- | --- | --- |
| 선택 영역 가리기 / 풀기 | `Ctrl+Shift+C` | 선택 영역을 가리거나 푼다 |
| 가림 보이기 / 숨기기 | `Ctrl+Shift+V` | 모든 가림을 잠깐 투명하게 하거나 다시 보이게 한다 |
| 모든 가림 풀기 | 없음 | 현재 문서의 선택 가림과 그 기록을 모두 지운다 |
| 설정 파일 열기 | 없음 | `TextBlinder.ini` 를 연다 |
| 단축키 설정... | 없음 | Notepad++ 단축키 설정 창을 연다 |

`Ctrl+Shift+C` 는 이렇게 동작한다:

1. 선택 영역이 모두 가려져 있으면, 가림을 푼다.
2. 선택 영역이 가려져 있지 않거나 일부만 가려져 있으면, 선택 영역 전체를 가린다.
3. 선택한 글이 없고 커서가 가린 범위 안에 있으면, 그 범위를 푼다.
4. 다중 선택이면, 선택 영역마다 1 ~ 2 번을 따로 적용한다.

단축키는 `설정 > 단축키 설정...` 의 `플러그인 명령어` 탭에서 바꾼다. 이 창에는 기본값으로 되돌리는 버튼이 없다. 되돌리려면 위 표의 기본값을 직접 다시 넣는다.

**주의:** 다른 프로그램에서 `Ctrl+Shift+V` 는 서식 없이 붙여넣기 키로 흔히 쓰인다. 습관으로 누르면 모든 가림이 투명해진다. 화면 공유 중이면 숨긴 글이 드러난다.

## 가림 유지

선택 가림의 범위는 `TextBlinder.masks` 파일에 파일마다 기록한다. 원문은 기록하지 않는다. 한 줄에 범위 하나를 쓴다:

```
<파일 전체 경로> TAB <시작 위치> TAB <길이> TAB <가린 글의 SHA-256>
```

- 가린 글이 저장한 파일과 같을 때만 기록한다. 저장하지 않은 변경이 있으면 저장할 때 기록한다.
- 파일을 다시 열면 해시가 맞는 범위만 다시 가린다. 파일이 밖에서 바뀌어 맞지 않는 범위는 건너뛰고 상태 표시줄에 개수를 알린다.
- 저장한 적 없는 새 문서 (`new 1` 등) 는 기록하지 않는다.
- 파일 이름을 바꾸면 기록의 경로도 바꾼다.

**참고:** 짧은 글 (예: 4 자리 PIN) 은 해시만으로도 추측할 수 있다. 기록 파일은 사용자 설정 폴더에 있다.

## 설정 파일

설정 파일은 Notepad++ 플러그인 설정 폴더의 `TextBlinder.ini` 다. 보통 `%APPDATA%\Notepad++\plugins\config\` 에 있다. `설정 파일 열기` 메뉴로 연다. Notepad++ 에서 저장하면 바로 다시 읽는다.

```ini
[Display]
MaskColor=#808080

[General]
Language=auto

[Rules]
Rule1=password\s*=\s*\K.+
```

| 키 | 값 |
| --- | --- |
| `MaskColor` | 가림 색. `#RRGGBB` 형식. 형식이 틀리면 기본값 `#808080` 을 쓴다 |
| `Language` | 메뉴 언어. `auto` 는 Notepad++ 언어를 따른다 (한국어면 한국어, 그 밖이면 영어). `ko`, `en` 으로 고정할 수 있다 |
| `DetectedLanguage` | TextBlinder 가 쓰는 값이다. 지난번에 확인한 Notepad++ 언어를 기억한다 |

**참고:** `auto` 는 Notepad++ 가 다 뜬 뒤에 언어를 확인한다. 그래서 처음 설치한 날에는 단축키 설정 창에 영어 명령 이름이 보일 수 있다.

## 규칙 가림

`[Rules]` 섹션에서 이름이 `Rule` 로 시작하는 키를 모두 규칙으로 읽는다. 정규식이 맞은 글 전체를 가린다.

```ini
[Rules]
Rule1=password\s*=\s*\K.+
Rule2=api_key\s*=\s*\K.+
```

- 앞부분을 보이게 두려면 `\K` 를 쓴다. `\K` 앞은 맞은 글에 들지 않는다. 위 예에서는 `password = ` 는 보이고 값만 가려진다.
- 정규식은 Notepad++ 찾기 창과 같은 엔진 (Boost) 을 쓴다. 찾기 창에서 시험한 정규식을 그대로 옮겨 쓸 수 있다.
- 대소문자를 가린다. 대소문자를 무시하려면 맨 앞에 `(?i)` 를 쓴다.
- `.` 는 줄바꿈에 맞지 않는다.
- 규칙은 모든 파일에 적용한다.
- 정규식이 틀린 규칙은 건너뛰고 상태 표시줄에 알린다.
- 글을 고치면 잠시 뒤 (0.3 초) 다시 찾는다.

규칙 가림은 `Ctrl+Shift+C` 로 풀어도 규칙이 다시 가린다. 내용을 보려면 `Ctrl+Shift+V` 를 쓴다. `모든 가림 풀기` 는 선택 가림만 푼다.

## 알려진 한계

TextBlinder 는 편집 화면과, 같은 문서를 보여 주는 창 (문서 구조 창 등) 에만 상자를 그린다. 그래서 아래 경우에는 가린 글이 보인다.

- **찾기 결과 창:** `현재 문서에서 모두 찾기` 등의 결과 창에 찾은 줄이 원문 그대로 나온다. 결과 창은 다른 문서라서 가리지 않는다.
- **가림 경계에 입력한 글자:** 가림 안에 입력한 글자는 가려진다. 가림이 시작하는 자리나 끝난 바로 뒤에 입력한 글자는 가려지지 않는다. 다시 가리려면 그 부분을 선택하고 `Ctrl+Shift+C` 를 누른다.
- **밖에서 바뀐 파일:** 다시 열 때 해시가 맞지 않는 가림은 건너뛴다. 건너뛴 자리의 글은 보인다. 상태 표시줄에 건너뛴 개수를 알린다.
- **인쇄:** 인쇄물에 가림이 나오는지는 아직 확인하지 않았다.

## 빌드

필요한 것: Visual Studio 2022 (C++ 데스크톱 개발), 플랫폼 도구 집합 `v143`.

```
MSBuild.exe TextBlinder.sln /p:Configuration=Release /p:Platform=x64
```

결과: `x64\Release\TextBlinder.dll`

단위 테스트:

```
vstest.console.exe x64\Release\TextBlinder.UnitTests.dll
```

빌드한 DLL 설치 (관리자 PowerShell, Notepad++ 를 닫은 상태):

```
.\Tools\Install-TextBlinder.ps1
```

## 라이선스

GPL v3 만 적용한다 (`GPL-3.0-only`). [LICENSE](LICENSE) 를 본다. 이 플러그인은 [Notepad++ 플러그인 템플릿](https://github.com/npp-plugins/plugintemplate) 을 바탕으로 만들었다.

`TextBlinder\src\Scintilla.h` 와 `Sci_Position.h` 는 Scintilla 의 헤더이고 Scintilla 라이선스를 따른다. [Scintilla-License.txt](TextBlinder/src/Scintilla-License.txt) 를 본다.

# GUI 자동 검증 (GuiTest)

TextBlinder 를 실제 Notepad++ 에서 자동으로 검증한다. 사람이 화면을 보던 수동 검증 항목을 대신한다.

## 안전장치

- **설치본을 건드리지 않는다.** 설치된 Notepad++ 를 `x64\GuiTest\npp-portable` 에 복사해서 쓴다. 복사본의 `doLocalConf.xml` 때문에 설정도 복사본 안에만 쓴다.
- **사용자 화면을 건드리지 않는다.** 복사본을 격리 데스크톱 (`TextBlinderGuiTest`) 에 띄운다. 창이 튀어나오지 않고 포커스도 빼앗지 않는다.
- **사용자 입력과 클립보드를 쓰지 않는다.** 창 메시지로만 조작한다.
- 끝나면 테스트용 복사본 Notepad++ 만 정리한다. 설치본 프로세스는 경로가 달라서 건드리지 않는다.

## 준비물

- Windows 10 이상, Notepad++ x64 설치본 (`C:\Program Files\Notepad++`)
- .NET Framework 4.x 의 C# 컴파일러 (`%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe`)
- 빌드한 `x64\Release\TextBlinder.dll` (`TextBlinder.sln` x64 Release)

## 실행

```powershell
.\Tools\GuiTest\Run-GuiTest.ps1                  # 전체 검증
.\Tools\GuiTest\Run-GuiTest.ps1 -Mode stress     # 닫기·다시 열기 60 회
.\Tools\GuiTest\Run-GuiTest.ps1 -Mode showcase   # 장면 캡처
.\Tools\GuiTest\Run-GuiTest.ps1 -FreshCopy       # 휴대용 복사본을 새로 만들고 검증
```

- 결과는 콘솔과 `x64\GuiTest\work\log-<모드>.txt` 에 남는다. 줄 머리는 `PASS`, `FAIL`, `OBSERVE` 다.
- 종료 코드: 성공 `0`, 실패 있음 `1`, 오류 `2` 이상
- 캡처는 `x64\GuiTest\work\*.png` 에 남는다.

## 모드

| 모드 | 하는 일 |
| --- | --- |
| `full` | `설정 > 가져오기 > 플러그인 가져오기...` 로 DLL 을 설치한다. 그다음 메뉴와 한국어, 가리기 / 풀기 규칙 1 ~ 4, 원문 보존, 가림 유지, 보이기 / 숨기기, 설정 색, 규칙 가림, 문서 구조 창, 편집과 단축키를 검증한다 (판정 40 개) |
| `stress` | 파일 닫기와 다시 열기를 60 회 반복한다. 버퍼 ID 가 다시 쓰여도 가림이 복원되는지 본다 |
| `showcase` | 설치, 메뉴, 선택 가림, 규칙 가림, 상태 표시줄 알림, 문서 구조 창 장면을 캡처한다 |

`stress` 와 `showcase` 는 `full` 이 설치한 DLL 을 쓴다. 그래서 `full` 을 먼저 돌린다.

## 한계

- **캡처는 GDI 로 그린다.** 격리 데스크톱에서는 DirectWrite 로 그린 편집 화면이 캡처되지 않는다. 그래서 테스트용 복사본만 GDI 로 그리게 바꾼다. 실제 설치본의 글자 모양과 조금 다를 수 있다.
- **실제 키보드 누름은 시험하지 않는다.** 메뉴에 붙은 단축키 글로 연결을 확인한다.
- **인쇄는 시험하지 않는다.**
- 격리 데스크톱이라 창틀이 Windows 기본 테마로 보인다.

## 파일

| 파일 | 내용 |
| --- | --- |
| `Run-GuiTest.ps1` | 휴대용 복사본 준비, 드라이버 빌드, 실행 |
| `Program.cs` | 격리 데스크톱을 만들고 드라이버를 그 안에서 실행 |
| `Win.cs` | Win32 선언 |
| `Npp.cs` | Notepad++ 를 창 메시지로 조작하는 도우미 |
| `Full.cs`, `Stress.cs`, `Showcase.cs` | 모드별 검증과 캡처 |

# Etterna Journal — 설계 문서

Etterna fork (`etterna-journal`) 에 추가되는 기능 세트: 팩별 Obsidian 마크다운 자동 생성 · 날짜별 메모 hotkey · 곡 태그 라벨링 · 스크린샷 자동 첨부.
상위 맥락: 메인 워크스페이스 `TODO.md` query 7, 환경 가이드는 `Content/Tools/etterna_fork_workflow.md`.

## 1. 구현 전략

**Lua-first.** Themes/Rebirth 내에서 Scripts 모듈 + BGAnimations overlay 로 구현하여 C++ 수정 최소화 → upstream merge 충돌 최소.
프레임워크 재사용:
- `DFRStarted` / `DFRFinished` 이벤트 (`src/Etterna/Singletons/SongManager.cpp:222, 233`) → 팩 리로드 hook
- `create_setting(...)` 헬퍼 (`Themes/Rebirth/Scripts/02 ThemePrefs.lua`) → 저널 설정 저장
- 기존 `TAGMAN` 확장 (`Themes/Rebirth/Scripts/01 TagManager.lua`) → 곡별 태그 관리
- `ScreenTextEntry` → 인게임 메모 입력
- `FILEMAN` (Lua) · `RageFileManager` → 파일 I/O
- `SaveScreenshot` 반환 경로 (`src/Etterna/Globals/StepMania.cpp:1201`) → 스크린샷 첨부 hook

## 2. 디렉토리 레이아웃 (사용자 Preference)

```
<JournalRoot>/                      # 사용자 Preference "JournalOutputDir"
  <PackName>.md                     # 팩별 마크다운 (옵시디언 vault 에 직접 맵핑)
  banners/                          # 모든 팩 배너 이미지 집합
    <PackName>__<SongDir>.<ext>
  screenshots/                      # 모든 스크린샷
    <PackName>__<SongDir>__<UTC>.<ext>
  tags.json                         # 사용자 정의 태그 리스트 + 곡별 할당
  .journal-cache.json               # 내부 상태 (banner 해시, 최근 entry 위치 등)
```

**이름 충돌 방지 규칙**
- 배너: `{pack}__{songdir}.{ext}` — 동일 팩 내 곡 디렉토리가 unique 하므로 충돌 없음. 팩 경계를 넘어도 prefix 로 분리.
- 스크린샷: `{pack}__{songdir}__{YYYYMMDD_HHMMSS}.{ext}` — 타임스탬프까지 포함하여 유일성 보장.
- 공백·특수문자는 `_` 로 치환.

## 3. 팩 마크다운 포맷

곡 섹션 단위. 한 팩 = 한 .md.

```markdown
# <Song Title>

![[banners/<PackName>__<SongDir>.png|128]]

#Etterna/4KEY/Jumpstream #Etterna/4KEY/Handstream

* 2026-04-23 0.85 레이트 후살 연습, 거의 안정
* 2026-04-24 0.9 시도, 최후살 실수 2회
  [[screenshots/<PackName>__<SongDir>__20260424_213012.png]]
```

각 섹션 구성 요소:
1. **헤딩** `# <타이틀>` — 곡 구분자. 병합 기준.
2. **배너 라인** `![[banners/...|128]]` — 자동 관리 (팩 리로드 때 갱신).
3. **태그 라인** `#tag1 #tag2 ...` — 자동 관리 (태그 UI 로 갱신).
4. **메모 블록** `* YYYY-MM-DD ...` — 사용자 작성, 자동 건드리지 않음. 스크린샷은 그날 메모 아래 이어붙임.

## 4. 병합 전략 (사용자 메모 보존)

팩 리로드 시 .md 재생성 로직:

```
for each song in pack:
  if section "# <title>" exists in current .md:
    # 기존 섹션 유지. 관리 라인만 sync.
    update banner line (2번째 content line if match pattern, else insert)
    update tag line  (3번째 content line if match pattern, else insert)
    leave memo bullets + screenshot attachments untouched
  else:
    # 신규 곡: 템플릿 섹션 append
    append fresh section with heading + banner + empty tag line
```

섹션 파싱 규칙: `^# ` (heading) 부터 다음 `^# ` 직전까지를 "한 섹션" 으로 취급. 하위 heading (`##`, `###`) 은 섹션 내부로 귀속.

**보존 보장**
- 사용자가 추가한 heading 간 설명 블록, 링크, 추가 태그 라인, memo bullet 은 절대 삭제하지 않음.
- 자동 관리 라인 판별은 pattern match (배너는 `![[banners/...]]`, 태그는 `^#` 으로 시작하고 `Etterna/` prefix) — user-authored 라인과 구분.

## 5. In-game Hotkey

곡 선택 스크린 (`ScreenSelectMusic`) 에서만 동작.

| Hotkey | 동작 |
|---|---|
| `Ctrl+Shift+C` | 현재 곡에 오늘자 메모 추가. `ScreenTextEntry` 로 텍스트 입력 → `* YYYY-MM-DD <text>` append. |
| `Ctrl+Shift+T` | 태그 토글 UI 팝업. `tags.json` 의 정의된 태그 리스트 표시 → 토글 → 해당 곡 섹션의 태그 라인 갱신. |

Rebirth 테마 기존 단축키와 충돌 없음을 확인 (`Themes/Rebirth/**/*.lua` grep 결과 `Ctrl+Shift+*` 조합 부재).

## 6. 태그 관리

`tags.json` 스키마:
```json
{
  "defined": [
    "Etterna/4KEY/Jumpstream",
    "Etterna/4KEY/Handstream",
    "Etterna/4KEY/Chordjack",
    "Etterna/4KEY/Stream",
    "Etterna/4KEY/Technical"
  ],
  "assigned": {
    "<PackName>/<SongDir>": ["Etterna/4KEY/Jumpstream", "Etterna/4KEY/OH_Trill"]
  }
}
```

`defined` 는 사용자가 직접 편집 또는 태그 UI 에서 "Add new" 로 추가. `assigned` 는 UI 토글로 관리. 마크다운의 태그 라인은 `assigned` 를 매번 재생성.

## 7. 스크린샷 자동 첨부

`SaveScreenshot` 호출 → 반환 경로 X → `screenshots/` 폴더로 복사 · rename (`{pack}__{song}__{ts}.ext`) → 현재 곡의 .md 섹션에서 **오늘자 entry** 찾아서 그 직후 라인에 `  [[screenshots/...]]` append (들여쓰기 2 space, embed 아닌 link 형태).

오늘자 entry 부재 시: `* YYYY-MM-DD` 라인을 먼저 생성 후 아래 첨부.

원본 스크린샷은 Etterna 기본 경로 (`Save/Screenshots/`) 에도 그대로 두어 기존 기능 유지.

## 8. Preferences

새 Preferences (Etterna `Preference<T>` 시스템, `Save/Preferences.ini`):

| Key | Type | Default | 설명 |
|---|---|---|---|
| `JournalEnabled` | bool | `false` | 저널 기능 전체 on/off. 기본 off → 비활성 사용자에게 영향 없음. |
| `JournalOutputDir` | string | `Save/Journal/` | 마크다운 · 배너 · 스크린샷 루트. 옵시디언 vault 경로 지정 권장. |

Preference 등록 위치: `src/Etterna/Singletons/PrefsManager.cpp` (소량 C++ 변경) — 또는 Lua `ThemePrefs` 만으로도 가능하지만 Preference 시스템이 더 표준적이라 전자 권장. Upstream 충돌 risk 최소 (끝에 append).

## 9. 파일 구조 (구현 산출물)

Upstream 충돌 최소화를 위해 **신규 파일 위주**로 작성:

```
Themes/Rebirth/Scripts/
  50 JournalCore.lua         # 경로 · 파일 I/O · 공통 유틸
  50 JournalMarkdown.lua     # .md 파싱 · 병합 · 쓰기
  50 JournalBanner.lua       # 배너 copy · rename
  50 JournalScreenshot.lua   # 스크린샷 hook · 첨부
  50 JournalTags.lua         # tags.json 로드 · 저장 · 갱신
  50 JournalInput.lua        # ScreenSelectMusic hotkey binding
Themes/Rebirth/BGAnimations/
  ScreenSelectMusic decorations/journal.lua   # overlay (UI 팝업 · input listener)
```

기존 파일 수정 (최소):
- `src/Etterna/Singletons/PrefsManager.cpp` — 2개 Preference 추가 (파일 끝에 append)
- `Themes/Rebirth/BGAnimations/ScreenSelectMusic decorations.lua` — journal.lua 로드 1 줄 추가

## 10. 구현 순서 (Phases)

| Phase | 산출 | 검증 |
|---|---|---|
| A | Preferences + Lua 모듈 스캐폴드 | Linux 빌드 통과, 프리퍼런스 나타나는지 로그 |
| B | 팩 마크다운 생성 + 배너 복사 | `DFRFinished` 후 .md · banners/ 생성 확인 |
| C | 메모 hotkey (`Ctrl+Shift+C`) | 인게임 입력 → .md 에 날짜 entry 추가 (Windows) |
| D | 태그 UI (`Ctrl+Shift+T`) | 토글 → tags.json 업데이트 → .md 태그 라인 갱신 (Windows) |
| E | 스크린샷 자동 첨부 | F12 등 기본 스크린샷 키 → .md 에 링크 append (Windows) |

Phase A-B 는 devcontainer Linux 빌드로 Lua syntax · 빌드 성공 확인 가능.
Phase C-E 는 실제 인게임 동작이 필요하므로 Windows 빌드에서 사용자 검증.

## 11. 비고

- 모든 파일 쓰기는 원자적(쓰기 실패 시 기존 파일 손상 방지) — `tmp 파일 작성 → rename` 패턴 사용.
- `.journal-cache.json` 은 지능적 skip 용 (배너 해시 같아도 복사 다시 안 하기, 섹션 위치 캐싱). 분실되어도 재생성 가능.
- upstream merge 시 `Themes/Rebirth/` 는 본가가 빈번히 수정 → 충돌 가능성 존재. 최악의 경우 테마 파일은 수동 rebase.

---
title: Etterna Journal — Windows 검증 체크리스트
---

# Etterna Journal — Windows 검증 체크리스트

[[TODO]] query 7-2 구현 검증용. devcontainer (Linux) 에서 GUI 테스트 불가능하므로 실제 동작은 Windows 로컬 빌드로 확인해야 합니다.
설계: [[Docs/Journal]]. 구현은 `Themes/Rebirth/Scripts/50 Journal*.lua` + overlay + StepMania.cpp (+5줄).

## 0. 준비

### 0.1 Windows 에 etterna-journal 빌드
- `git pull` (또는 최초라면 `git clone https://github.com/ATTANGHUB/etterna-journal.git`).
- `Docs/Building.md` 절차대로 Visual Studio + CMake + vcpkg 로 빌드.
- 빌드 성공하면 `Etterna.exe` 가 생성됨.

### 0.2 최초 실행 (설정 파일 생성)
- `Etterna.exe` 실행 → 곡 선택 화면까지 진입.
- **기대**: `Save/Rebirth/journal.lua` 파일이 생성됨 (기본값으로).
- 게임 종료.

### 0.3 설정 편집
- `Save/Rebirth/journal.lua` 를 텍스트 에디터로 열기.
- 다음처럼 수정:
  ```lua
  return {
      enabled = true,
      outputDir = "C:/Users/<you>/ObsidianVault/Etterna",
      bannerDir = "banners",
      screenshotDir = "screenshots",
      tagsFile = "tags.json",
      cacheFile = ".journal-cache.json",
  }
  ```
  - `outputDir` 은 **절대 경로**, **forward-slash** 추천 (백슬래시도 동작하지만 이스케이프 필요).
  - 이 폴더는 자동 생성됨 (mkdir) — 미리 만들 필요 없음.

## 1. Phase B — 팩 마크다운 + 배너 복사

### 1.1 기본 생성
1. `Etterna.exe` 재실행.
2. 곡 선택 화면 진입 후 1~2초 대기.
3. **확인**:
   - [ ] `<outputDir>/` 에 각 팩마다 `<팩이름>.md` 생성됨.
   - [ ] `<outputDir>/banners/` 폴더에 배너 이미지들이 `<팩>__<곡디렉토리>.<ext>` 형태로 복사됨.
   - [ ] `<outputDir>/tags.json` 생성됨 (defined 태그 8개 기본 포함).
   - [ ] .md 파일 안에 각 곡마다 다음 섹션이 있음:
     ```
     # <곡 제목>
     
     ![[banners/<팩>__<곡디렉토리>.png|128]]
     ```

### 1.2 메모 보존 검증 (중요)
1. 아무 .md 파일에서 한 곡 섹션을 찾아, 배너 라인 아래에 수동으로 추가:
   ```
   * 2026-04-23 테스트 메모. 삭제되지 않아야 함.
   ```
2. 게임에서 **Ctrl+Q** 로 차트 리로드.
3. **확인**:
   - [ ] 방금 추가한 `* 2026-04-23 테스트 메모...` 라인이 그대로 남아있음.
   - [ ] 배너 라인은 정상 (바뀌지 않았거나 동일한 내용으로 유지).
   - [ ] 새 곡이 팩에 추가된 경우에만 신규 섹션이 append 됨.

### 1.3 배너 이름 충돌 방지
- [ ] 같은 곡을 여러 번 리로드해도 `banners/` 폴더에 중복 파일이 생기지 않음 (이미 존재 시 skip).
- [ ] 다른 팩에 같은 이름 곡이 있어도 `<팩>__<곡>` prefix 로 분리됨.

## 2. Phase C — 메모 hotkey (Ctrl+Shift+C)

1. 곡 선택 화면에서 아무 곡 선택.
2. **Ctrl+Shift+C** 누름.
3. **확인**:
   - [ ] 텍스트 입력창 (`ScreenTextEntry`) 이 뜨고 "Memo: <곡 제목>" 프롬프트 표시.
4. 메모 입력 (예: `0.9레이트 연습, 후살 2회 실수`) → Enter.
5. **확인**:
   - [ ] 화면 우상단에 "[Journal] memo saved to ..." 메시지 표시.
   - [ ] 해당 팩의 .md 파일에서 해당 곡 섹션 끝에 다음 라인 추가:
     ```
     * 2026-04-23 0.9레이트 연습, 후살 2회 실수
     ```
6. Esc 로 입력 취소 시:
   - [ ] .md 변경 없음, 아무 메시지 없음.

## 3. Phase D v1 — 태그 순환 토글 (Ctrl+Shift+T)

1. 곡 선택.
2. **Ctrl+Shift+T** 를 여러 번 누름 (예: 3회).
3. **확인**:
   - [ ] 누를 때마다 화면 우상단에 `[Journal] +Etterna/4KEY/Jumpstream` 같은 메시지 표시.
   - [ ] `tags.json` 의 `assigned` 섹션에 `"<팩>/<곡>"` 키로 태그 리스트 저장.
   - [ ] 해당 곡 섹션의 배너 라인 아래 (또는 헤딩 바로 아래) 태그 라인이 업데이트됨:
     ```
     #Etterna/4KEY/Jumpstream #Etterna/4KEY/Handstream
     ```
4. 같은 태그가 이미 있을 때 다시 누르면 `-` 로 제거 표시.

### 3.1 제한 사항
- 현재는 `defined` 리스트를 **순차 순환** 하며 토글하는 placeholder. 특정 태그만 고르려면 여러 번 눌러야 함.
- `defined` 리스트를 바꾸려면 `tags.json` 을 직접 편집 후 게임 재시작.
- 제대로 된 multiselect UI 는 후속 iteration (Phase D-2).

## 4. Phase E — 스크린샷 자동 첨부

1. 곡 선택 화면에서 아무 곡 선택.
2. **PrintScreen** 누름 (Shift+PrintScreen 은 압축 저장).
3. **확인**:
   - [ ] `Save/Screenshots/` 에 원본 스크린샷 생성 (기존 동작 유지).
   - [ ] `<outputDir>/screenshots/<팩>__<곡>__20260423_HHMMSS.png` 로 복사본 생성.
   - [ ] .md 파일의 해당 곡 섹션에서 **오늘자 메모 아래** 다음 라인이 추가됨:
     ```
       [[screenshots/<팩>__<곡>__20260423_HHMMSS.png]]
     ```
     (2-space 들여쓰기, `![[...]]` 가 아닌 `[[...]]` — 옵시디언에서 embed 아닌 link).
4. 오늘자 메모가 아직 없을 때 PrintScreen 누름 → `* 2026-04-23` 라인이 먼저 생성되고 그 아래에 첨부.

## 5. Obsidian 연동 확인

1. `outputDir` 을 옵시디언 vault 내 경로로 지정했다면, Obsidian 에서 vault 열기.
2. **확인**:
   - [ ] 팩 .md 들이 노트로 나타남.
   - [ ] `![[banners/...|128]]` 가 128px 이미지로 렌더.
   - [ ] `#Etterna/4KEY/Jumpstream` 가 옵시디언 태그로 인식 (왼쪽 panel 태그 뷰).
   - [ ] 스크린샷 `[[screenshots/...]]` 이 클릭 가능한 링크로.

## 6. 트러블슈팅

| 증상 | 원인 후보 | 대처 |
|---|---|---|
| `journal.lua` 가 생성되지 않음 | 테마 로딩 전 에러, 또는 Rebirth 가 아님 | Themes 설정 확인, 로그에서 `[Journal]` 검색 |
| .md 가 생성되지 않음 | `enabled=false` 또는 `outputDir` 비어있음 | `Save/Rebirth/journal.lua` 재확인 |
| `mkdir` 실패 | `outputDir` 에 쓰기 권한 없음 | 다른 경로로 이동 |
| 한글 곡명 깨짐 | .md 저장 encoding (Lua `io` 는 바이트 그대로 씀) | 옵시디언에서 UTF-8 기대 — 곡명 원본이 UTF-8 이면 정상 |
| Ctrl+Shift+C 반응 없음 | SelectMusic overlay 미로드 | 로그에서 `JournalOverlay` 검색, `default.lua` 수정 반영 확인 |
| 스크린샷이 .md 에 붙지 않음 | `GAMESTATE:GetCurrentSong()` 이 nil (곡 선택 전) | 곡 선택 후 찍기 |

## 7. 로그 확인 (게임 내 또는 log.txt)

모든 Journal 메시지는 `[Journal]` prefix 로 시작. Etterna 의 로그 파일 (`Logs/log.txt` 또는 게임 내 `~` 콘솔) 에서 grep:
- `[Journal] regenerateAll: N packs, +X new sections, Y updated (Zs)` — 정상 동작 신호.
- `[Journal] ensureDir failed` — 경로 문제.
- `[Journal] banner copy failed` — 원본 배너 파일 없음 (일부 곡은 배너 없을 수 있음, 정상).
- `[Journal] screenshot source not found` — 저장 경로 해석 실패 (버그 가능성).

## 8. 완료 후

- 이 문서의 checkbox 를 다 확인한 후 문제 없으면 [[TODO]] 7-2 를 `[x]` 로 resolve.
- 문제 발견 시 이슈 요약 + 재현 방법 을 TODO 새 쿼리로 등록.
- Phase D-2 (multiselect 태그 UI) 는 별도 쿼리로 후속 iteration.

# Raising Fish SDL2 - C프로그래밍 13주차 과제

SDL2 기반 물고기 키우기 게임 확장 과제입니다.

## 구현 기능

| 구분 | 기능 |
|------|------|
| 필수 1 | 물고기 종류 추가 (Normal / Fast / Big) |
| 필수 2 | 종류별 물 소비량 및 성장 속도 차이 |
| 선택 1 | 물 양에 따른 물고기 이미지 색상 변경 |
| 선택 2 | 상황별 효과음 (물 주기 / 죽음 / 레벨업) |

## 물고기 종류

| 종류 | 인디케이터 색 | 성장 배율 | 물 소비 배율 |
|------|-------------|----------|------------|
| Normal | 초록 | ×1 | ×1 |
| Fast | 파랑 | ×2 | ×1 |
| Big | 빨강 | ×1 | ×2 |

## 조작 방법

- `J` : 왼쪽 어항 선택
- `L` : 오른쪽 어항 선택
- `K` : 선택된 어항에 물 주기 (+5)
- `ESC` : 게임 종료

## 파일 구성

- `rasingFish_final.c` — 게임 소스 코드
- `fish.bmp` — 물고기 이미지
- `water.wav` — 물 주기 효과음
- `과제_보고서.docx` — 과제 내용 설명

## 빌드 방법

<img width="1002" height="790" alt="image" src="https://github.com/user-attachments/assets/1bdd4831-217b-4e1b-bf24-3c96db0fce2b" />
- 첫 실행화면
<img width="1002" height="790" alt="image" src="https://github.com/user-attachments/assets/2e4c382a-cc6d-487a-95ee-41c36f307804" />
- 플레이 중
<img width="1002" height="790" alt="image" src="https://github.com/user-attachments/assets/85936a33-dd54-4958-9e50-3bf37afc801a" />
- 죽기 직전
<img width="1002" height="790" alt="image" src="https://github.com/user-attachments/assets/931f7fb6-4d25-4518-ac26-5d78a09b6400" />
- 게임오버

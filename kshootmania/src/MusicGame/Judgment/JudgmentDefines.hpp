#pragma once

namespace MusicGame::Judgment
{
    // Supplemental input accuracy only; score/gauge judgments remain unchanged.
    inline constexpr double kPerfectCriticalWindowMs = 21.0;

	enum class JudgmentResult
	{
		kUnspecified,
		kError,
		kNearFast,
		kNearSlow,
		kCritical,
	};

	enum class KeyBeamType : int32
	{
		kDefault = 0,
		kNear,
		kCritical,
		kNearSlow,
		kNearFast,
		kErrorFast,
		kPerfectCritical,

		kNumBeamTypes,
	};

	enum class ChipAnimType
	{
		kUnspecified,
		kError,
		kNear,
		kNearFast,
		kCritical,
		kPerfectCritical,
	};

	namespace TimingWindow
	{
		namespace ChipNote
		{
			constexpr double kWindowSecCritical = 0.044;
			constexpr double kWindowSecNear = 0.114;
			constexpr double kWindowSecError = 0.260;
			constexpr double kWindowSecErrorEasy = 0.160;

			// 遅ERROR判定開始タイミング(ノート時刻からの経過時間)
			// 本来見逃しERRORになる時間はNEAR幅と同じ+114msだが、v1の実装を再現して+110msとする
			// そのため、NEAR判定幅は早NEARは-114ms~-42ms、遅NEARは+42ms~+110msと非対称になっている
			constexpr double kWindowSecLateErrorBegin = 0.110;
		}

		namespace LongNote
		{
			constexpr double kWindowSecPreHold = 0.15;
		}

		namespace LaserNote
		{
			constexpr double kWindowSecSlam = 0.06;
		}
	}

	constexpr int32 kScoreValueNear = 1;
	constexpr int32 kScoreValueCritical = 2;

	constexpr double kHalveComboBPMThreshold = 256.0;

	// キーボード入力での1秒あたりのLASERカーソル移動量
	constexpr double kLaserKeyboardCursorXPerSec = 5.0;

	// LASERカーソル位置を理想カーソル位置と同一とみなす範囲
	// (この範囲に入ったら理想位置に吸い付かせて表示し、カーソルは移動しない)
	constexpr double kLaserAutoFitMaxDeltaCursorX = 0.05;

	constexpr bool IsLaserCursorInAutoFitRange(double cursorX, double noteCursorX)
	{
		return Abs(cursorX - noteCursorX) < kLaserAutoFitMaxDeltaCursorX;
	}

	// LASERカーソル位置をCRITICAL判定とする許容誤差
	constexpr double kLaserCriticalMaxDeltaCursorX = 0.1;

	constexpr bool IsLaserCursorInCriticalJudgmentRange(double cursorX, double noteCursorX)
	{
		return Abs(cursorX - noteCursorX) < kLaserCriticalMaxDeltaCursorX;
	}

	// LASERカーソルの増幅移動量の倍率
	// (LASERカーソル移動がLASERノーツと同方向の場合はこの倍率で入力があったものと見立てた"増幅移動量"を計算し、
	//  "増幅移動量"での移動幅が理想位置を超えている場合は理想位置にカーソルを吸い付かせる)
	constexpr double kLaserCursorInputOvershootScale = 20.0;

	// LASER入力の移動量増幅での吸着判定間隔(60fps基準)
	constexpr double kLaserInputAmplificationCheckIntervalSec = 1.0 / 60.0;

	// 直角LASERをCRITICAL判定するのに必要な累計カーソル移動量
	constexpr double kLaserSlamCriticalDeltaCursorXThreshold = 0.0005;

	// 直角LASER判定後の判定補正時間(秒)
	constexpr double kLaserAutoSecAfterSlamJudgment = 0.075;

	// LASER成功判定後の判定補正時間(秒)
	constexpr double kLaserAutoSecAfterCorrectMovement = 0.025;

	// LASER折り返し前後での判定補正時間(秒)
	constexpr double kLaserAutoSecBeforeLineDirectionChange = 0.05;
	constexpr double kLaserAutoSecBeforeLineDirectionChangeByCorrectMovement = 0.2;
	constexpr double kLaserAutoSecAfterLineDirectionChange = 0.05;

	class ButtonLaneJudgment;
	class LaserLaneJudgment;

	using BTLaneJudgments = std::array<ButtonLaneJudgment, kson::kNumBTLanesSZ>;
	using FXLaneJudgments = std::array<ButtonLaneJudgment, kson::kNumFXLanesSZ>;
	using LaserLaneJudgments = std::array<LaserLaneJudgment, kson::kNumLaserLanesSZ>;
}

#pragma once

// Generates font glyphs ahead of time, a little per frame.
//
// The UI fonts are distance-field fonts, so the first time a character is drawn its glyph is generated
// on the spot (~2 ms per CJK character as SDF, ~9 ms as MSDF). Scrolling song select onto titles with new
// characters (Japanese titles especially) therefore stalled 70-400 ms per step. Queue the text that is
// about to appear and call Update() every frame to spread that work out instead.
namespace GlyphPrewarmer
{
	/// High priority: text near the cursor. Replaces the previous urgent queue.
	void SetUrgent(const Array<String>& texts);

	/// Low priority: text of the whole open folder. Replaces the previous background queue.
	void SetBackground(const Array<String>& texts);

	/// Generates queued glyphs until budgetMs has been spent.
	void Update(double budgetMs);
}

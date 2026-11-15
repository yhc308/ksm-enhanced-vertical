#include "GlyphPrewarmer.hpp"
#include <chrono>

namespace GlyphPrewarmer
{
	namespace
	{
		HashSet<char32> g_done;
		Array<char32> g_urgent, g_background;
		size_t g_urgentIndex = 0, g_backgroundIndex = 0;

		Array<char32> Pending(const Array<String>& texts)
		{
			Array<char32> chars;
			HashSet<char32> seen;
			for (const auto& text : texts)
			{
				for (const char32 ch : text)
				{
					if (ch < U' ' || g_done.contains(ch) || !seen.insert(ch).second)
						continue;
					chars.push_back(ch);
				}
			}
			return chars;
		}

		Optional<char32> Next()
		{
			while (g_urgentIndex < g_urgent.size())
			{
				const char32 ch = g_urgent[g_urgentIndex++];
				if (!g_done.contains(ch))
					return ch;
			}
			while (g_backgroundIndex < g_background.size())
			{
				const char32 ch = g_background[g_backgroundIndex++];
				if (!g_done.contains(ch))
					return ch;
			}
			return none;
		}
	}

	void SetUrgent(const Array<String>& texts)
	{
		g_urgent = Pending(texts);
		g_urgentIndex = 0;
	}

	void SetBackground(const Array<String>& texts)
	{
		g_background = Pending(texts);
		g_backgroundIndex = 0;
	}

	void Update(double budgetMs)
	{
		if (g_urgentIndex >= g_urgent.size() && g_backgroundIndex >= g_background.size())
			return;
		// Near-cursor text every frame; the rest of the folder only every 4th frame, so browsing a big
		// folder does not keep every frame busy.
		static uint64 frame = 0;
		if (g_urgentIndex >= g_urgent.size() && (++frame % 4) != 0)
			return;
		const auto start = std::chrono::steady_clock::now();
		const Font regular = AssetManagement::SystemFont();
		const Font bold = AssetManagement::SystemFontBold();
		while (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() < budgetMs)
		{
			const auto ch = Next();
			if (!ch)
				break;
			const String text(1, *ch);
			regular.preload(text);
			bold.preload(text);
			g_done.insert(*ch);
		}
	}
}

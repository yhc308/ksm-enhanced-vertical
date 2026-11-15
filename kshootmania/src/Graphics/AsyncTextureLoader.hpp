#pragma once

// Decodes external images (song jackets, which can be full 1920x1080 osu! backgrounds) on worker
// threads instead of inside the frame that first draws them. Decoding one took 10-55 ms on the main
// thread and was the largest remaining song select stall after the font fix.
//
// The UI texture loader asks TryGet(); until the image is decoded it gets an empty texture, which the
// UI caches under that key. Update() (called once per frame) evicts those placeholders when their
// image is ready, so the next draw loads the real texture: the jacket appears a few frames later
// instead of freezing a frame.
namespace AsyncTextureLoader
{
	/// The texture if its image has been decoded; otherwise queues the decode and returns none.
	[[nodiscard]]
	Optional<Texture> TryGet(FilePathView cacheKey, FilePathView fullPath, TextureDesc desc);

	/// Evicts UI placeholders whose images are ready. Call once per frame on the main thread.
	void Update();

	/// Stops the worker threads (call before exit).
	void Shutdown();
}

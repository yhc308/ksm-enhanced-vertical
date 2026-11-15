#include "AsyncTextureLoader.hpp"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace AsyncTextureLoader
{
	namespace
	{
		// Jackets are shown at a few hundred pixels at most; larger sources are shrunk on the worker.
		constexpr int32 kMaxSide = 1024;
		constexpr size_t kWorkerCount = 2;

		struct Request
		{
			FilePath cacheKey;
			FilePath fullPath;
		};

		std::mutex g_mutex;
		std::condition_variable g_wake;
		std::deque<Request> g_queue;
		HashSet<FilePath> g_pending;			   // full paths queued or decoding
		HashTable<FilePath, Image> g_ready;		   // decoded, waiting to become a texture
		Array<FilePath> g_readyKeys;			   // cache keys to evict from the UI cache
		Array<std::thread> g_workers;
		bool g_stopping = false;

		void WorkerLoop()
		{
			for (;;)
			{
				Request request;
				{
					std::unique_lock lock{ g_mutex };
					g_wake.wait(lock, []
								{
									return g_stopping || !g_queue.empty();
								});
					if (g_stopping)
						return;
					request = std::move(g_queue.front());
					g_queue.pop_front();
				}
				Image image{ request.fullPath };
				if (image && (image.width() > kMaxSide || image.height() > kMaxSide))
				{
					const double scale = static_cast<double>(kMaxSide) / Max(image.width(), image.height());
					image = image.scaled(Max(1, static_cast<int32>(image.width() * scale)), Max(1, static_cast<int32>(image.height() * scale)), InterpolationAlgorithm::Area);
				}
				std::lock_guard lock{ g_mutex };
				g_ready[request.fullPath] = std::move(image); // an empty Image marks a failed decode
				g_readyKeys.push_back(request.cacheKey);
			}
		}

		void EnsureWorkers()
		{
			if (!g_workers.isEmpty())
				return;
			for (size_t i = 0; i < kWorkerCount; ++i)
				g_workers.emplace_back(WorkerLoop);
		}
	}

	Optional<Texture> TryGet(FilePathView cacheKey, FilePathView fullPath, TextureDesc desc)
	{
		std::unique_lock lock{ g_mutex };
		if (auto it = g_ready.find(fullPath); it != g_ready.end())
		{
			Image image = std::move(it->second);
			g_ready.erase(it);
			g_pending.erase(FilePath{ fullPath });
			lock.unlock();
			return image ? Texture{ image, desc } : Texture{};
		}
		if (!g_pending.contains(fullPath))
		{
			g_pending.insert(FilePath{ fullPath });
			g_queue.push_back({ FilePath{ cacheKey }, FilePath{ fullPath } });
			lock.unlock();
			EnsureWorkers();
			g_wake.notify_one();
		}
		return none;
	}

	void Update()
	{
		Array<FilePath> keys;
		{
			std::lock_guard lock{ g_mutex };
			if (g_readyKeys.isEmpty())
				return;
			keys.swap(g_readyKeys);
		}
		for (const auto& key : keys)
			noco::Asset::UnloadTexture(key); // drop the empty placeholder so the next draw loads the real texture
	}

	void Shutdown()
	{
		{
			std::lock_guard lock{ g_mutex };
			g_stopping = true;
		}
		g_wake.notify_all();
		for (auto& worker : g_workers)
		{
			if (worker.joinable())
				worker.join();
		}
		g_workers.clear();
	}
}

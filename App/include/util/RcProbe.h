#pragma once

// Phase 4: gate noisy /tmp/rc_probe.log writes.
// Enable with: RC_PROBE=1 (or any non-empty value other than "0")
//
// Usage:
//   RC_PROBE("startLocalPlay: begin\n");
//   RC_PROBE("userId=%d\n", id);

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

inline bool rcProbeEnabled()
{
	static int enabled = -1;
	if (enabled < 0)
	{
		const char* e = std::getenv("RC_PROBE");
		enabled = (e && e[0] != '\0' && !(e[0] == '0' && e[1] == '\0')) ? 1 : 0;
	}
	return enabled != 0;
}

inline void rcProbe(const char* fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
	__attribute__((format(printf, 1, 2)))
#endif
{
	if (!rcProbeEnabled())
		return;
	FILE* f = std::fopen("/tmp/rc_probe.log", "a");
	if (!f)
		return;
	va_list ap;
	va_start(ap, fmt);
	std::vfprintf(f, fmt, ap);
	va_end(ap);
	std::fclose(f);
}

// Convenience macro matching prior one-liner style.
#define RC_PROBE(...) ::rcProbe(__VA_ARGS__)

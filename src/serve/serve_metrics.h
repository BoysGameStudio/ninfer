#pragma once

// Cumulative counters behind GET /metrics, in the flat `name value` subset of the Prometheus
// text format. The token/seconds counters are sourced from the Engine's live per-unit totals,
// so they advance during a request rather than only at its completion; prompt tokens count
// only computed prefill, prefix-cache hits excluded. The completion-based sums this class
// accumulates back the request, reuse, and speculative series.

#include "serve/generation_service.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace ninfer::serve {

class ServeMetrics {
public:
    // Accumulates one completed request. Called from the same funnel as the request-done log
    // line, so every protocol and both streaming modes count.
    void record(const GenerationOutcome& outcome);

    // One complete Prometheus text body, without HTTP framing. The supplied request-lifetime
    // count begins before preparation/submission and survives through response release, so
    // every accepted request remains visible.
    [[nodiscard]] std::string render(std::uint32_t max_concurrency,
                                     const ninfer::RuntimeStats& live,
                                     std::size_t active_requests) const;

private:
    mutable std::mutex mutex_;
    std::uint64_t requests_total_                    = 0;
    std::uint64_t prefix_cache_hit_tokens_total_     = 0;
    std::uint64_t speculative_draft_tokens_total_    = 0;
    std::uint64_t speculative_accepted_tokens_total_ = 0;
};

} // namespace ninfer::serve

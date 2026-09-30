#include "serve/serve_metrics.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace {

using namespace ninfer::serve;

int check(bool condition, const char* message) {
    if (condition) { return 0; }
    std::cerr << message << '\n';
    return 1;
}

bool has_line(const std::string& body, const std::string& line) {
    return body.find(line + "\n") != std::string::npos;
}

GenerationOutcome make_outcome(std::uint64_t cache_hits, std::uint64_t drafted,
                               std::uint64_t accepted) {
    GenerationOutcome outcome;
    outcome.metrics.prefix_cache_hit_tokens     = cache_hits;
    outcome.metrics.speculative_draft_tokens    = drafted;
    outcome.metrics.speculative_accepted_tokens = accepted;
    return outcome;
}

} // namespace

int main() {
    int failures = 0;

    ServeMetrics metrics;

    {
        const std::string body = metrics.render(2, ninfer::RuntimeStats{}, 0);
        failures += check(has_line(body, "ninfer:prompt_tokens_total 0"),
                          "empty render missed prompt_tokens_total");
        failures += check(has_line(body, "ninfer:prompt_seconds_total 0.000000"),
                          "empty render missed prompt_seconds_total");
        failures += check(has_line(body, "ninfer:tokens_predicted_total 0"),
                          "empty render missed tokens_predicted_total");
        failures += check(has_line(body, "ninfer:tokens_predicted_seconds_total 0.000000"),
                          "empty render missed tokens_predicted_seconds_total");
        failures += check(has_line(body, "ninfer:requests_processing 0"),
                          "empty render missed requests_processing");
        failures += check(has_line(body, "ninfer:requests_deferred 0"),
                          "empty render missed requests_deferred");
        failures += check(has_line(body, "ninfer:requests_total 0"),
                          "empty render missed requests_total");
        failures += check(has_line(body, "ninfer:prefix_cache_hit_tokens_total 0"),
                          "empty render missed prefix_cache_hit_tokens_total");
        failures += check(has_line(body, "ninfer:draft_tokens_total 0"),
                          "empty render missed draft_tokens_total");
        failures += check(has_line(body, "ninfer:draft_accepted_tokens_total 0"),
                          "empty render missed draft_accepted_tokens_total");
    }

    {
        ninfer::RuntimeStats live;
        live.computed_prefill_tokens = 12345;
        live.committed_decode_tokens = 678;
        live.prefill_seconds_total   = 4.5;
        live.decode_seconds_total    = 12.25;
        const std::string body       = metrics.render(2, live, 5);
        failures += check(has_line(body, "ninfer:prompt_tokens_total 12345"),
                          "live prefill tokens not rendered");
        failures += check(has_line(body, "ninfer:tokens_predicted_total 678"),
                          "live decode tokens not rendered");
        failures += check(has_line(body, "ninfer:prompt_seconds_total 4.500000"),
                          "live prefill seconds not rendered");
        failures += check(has_line(body, "ninfer:tokens_predicted_seconds_total 12.250000"),
                          "live decode seconds not rendered");
        failures += check(has_line(body, "ninfer:decode_rounds_total 0"),
                          "empty render missed decode_rounds_total");
        // Occupancy splits into processing lanes and the deferred queue.
        failures += check(has_line(body, "ninfer:requests_processing 2"),
                          "processing not capped at max_concurrency");
        failures += check(has_line(body, "ninfer:requests_deferred 3"),
                          "deferred did not take the occupancy overflow");
    }

    metrics.record(make_outcome(100, 9, 7));
    metrics.record(make_outcome(250, 6, 5));

    {
        const std::string body = metrics.render(1, ninfer::RuntimeStats{}, 0);
        failures += check(has_line(body, "ninfer:requests_total 2"),
                          "completed requests not accumulated");
        failures += check(has_line(body, "ninfer:prefix_cache_hit_tokens_total 350"),
                          "prefix cache hits not accumulated");
        failures += check(has_line(body, "ninfer:draft_tokens_total 15"),
                          "draft tokens not accumulated");
        failures += check(has_line(body, "ninfer:draft_accepted_tokens_total 12"),
                          "accepted draft tokens not accumulated");
    }

    return failures == 0 ? 0 : 1;
}
